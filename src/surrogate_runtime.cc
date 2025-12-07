// Copyright 2025 Yunseong Hwang
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-FileCopyrightText: 2025 Yunseong Hwang
//
// SPDX-License-Identifier: Apache-2.0

#include "surrogate_runtime.h"

#include <QMessageBox>

#include "utils.h"

// HostSurrogateRuntime implementation

HostSurrogateRuntime::HostSurrogateRuntime(QObject *parent)
    : QObject(parent),
      m_surrogate(new HostSurrogate()) {
  InitializeStaticInstance();
  StartUnusedLibraryCleaner();
  StartExitConditionChecker();
}

HostSurrogateRuntime::~HostSurrogateRuntime() {
  if (s_instance == this) {
    s_instance = nullptr;
  }
}

HostSurrogateRuntime *HostSurrogateRuntime::instance() { return s_instance; }

void HostSurrogateRuntime::InitializeStaticInstance() {
  if (s_instance != nullptr) {
    QString text = QString(R"(
Error: Multiple Runtime Instances

A HostSurrogateRuntime instance already exists.
Cannot create multiple instances.
)")
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    ExitApplicationLater();
    return;
  }
  s_instance = this;
}

void HostSurrogateRuntime::StartUnusedLibraryCleaner() {
  QCoreApplication *app = QCoreApplication::instance();
  if (!app) {
    return;
  }
  QAbstractEventDispatcher *dispatcher = app->eventDispatcher();
  if (!dispatcher) {
    return;
  }
  connect(
      dispatcher, &QAbstractEventDispatcher::aboutToBlock, this,
      &HostSurrogateRuntime::OnAboutToBlock
  );
}

void HostSurrogateRuntime::StartExitConditionChecker() {
  m_checkForExitTimer.setSingleShot(true);
  m_checkForExitTimer.setInterval(m_checkForExitTimeout);
  connect(
      &m_checkForExitTimer, &QTimer::timeout, this,
      &HostSurrogateRuntime::CheckForExit
  );
  m_checkForExitTimer.start(m_checkForExitTimeout);
}

void HostSurrogateRuntime::CheckForExit() {
  ULONG a = CoAddRefServerProcess();
  ULONG r = CoReleaseServerProcess();
  if (r == 0) {
    if (m_acquisitionCount == 0 && !m_warnedIdle) {
      m_warnedIdle = true;
      QString text = QString(R"(
Warning: Timed Out For No Interaction

No COM interaction has occurred for the timeout period (%1 ms).
Server process will now terminate.
)")
                         .arg(m_checkForExitTimeout)
                         .trimmed();
      QMessageBox::warning(nullptr, QCoreApplication::applicationName(), text);
    }
    ExitApplicationLater();
  } else {
    CheckForExitLater();
  }
}

void HostSurrogateRuntime::CheckForExitLater() {
  m_checkForExitTimer.start(m_checkForExitTimeout);
}

void HostSurrogateRuntime::FreeUnusedLibraries() {
  if (!m_freeUnusedLibrariesTimer.isValid() ||
      m_freeUnusedLibrariesTimer.hasExpired(m_freeUnusedLibrariesInterval)) {
    m_freeUnusedLibrariesTimer.restart();
    ::CoFreeUnusedLibraries();
  }
}

void HostSurrogateRuntime::OnAboutToBlock() { FreeUnusedLibraries(); }

void HostSurrogateRuntime::AddServerReference() {}

void HostSurrogateRuntime::ReleaseServerReference() {}

// RunAsSurrogate implementation

RunAsSurrogate::RunAsSurrogate(
    const QUuid &clsid, bool embedding, QObject *parent
)
    : HostSurrogateRuntime(parent) {
  THROW_IF_FAILED(CoRegisterSurrogate(m_surrogate));
  m_surrogate->LoadDllServer(clsid);
}

RunAsSurrogate::~RunAsSurrogate() {}

// RunAsStandalone implementation

RunAsStandalone::RunAsStandalone(
    QList<ClassSpec> &specs, const QString &readyEvent, DWORD regcls,
    QObject *parent
)
    : HostSurrogateRuntime(parent),
      m_readyEvent(readyEvent) {
  CheckMultipleUse(specs, regcls);
  {
    HRESULT sus = CoSuspendClassObjects();
    if (FAILED(sus)) {
    }
    BOOL suspend = SUCCEEDED(sus);
    HRESULT hr = m_surrogate->RegisterAllClassFactories(specs, suspend, regcls);
    if (!m_surrogate->HasRegisteredClass()) {
      QString text = QString(R"(
Error: No Class Registered

No class factory could be registered. Exiting.
)")
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      DWORD err = GetLastError();
      ExitApplicationLater();
    }
    if (SUCCEEDED(sus) && m_surrogate->HasRegisteredClass()) {
      HRESULT res = CoResumeClassObjects();
      if (FAILED(res)) {
      }
    }
    if (SUCCEEDED(hr)) {
      m_readyEvent.SetEvent();
    }
  }
}

RunAsStandalone::~RunAsStandalone() {
  HRESULT sus = CoSuspendClassObjects();
  if (FAILED(sus)) {
  }
  HRESULT hr = m_surrogate->RevokeAllClassFactories();
  if (SUCCEEDED(sus) && m_surrogate->HasRegisteredClass()) {
    HRESULT res = CoResumeClassObjects();
    if (FAILED(res)) {
    }
  }
}

void RunAsStandalone::CheckMultipleUse(QList<ClassSpec> &specs, DWORD regcls) {
  if (regcls & REGCLS_MULTIPLEUSE) {
    m_hasMultipleUse = true;
  } else {
    for (const ClassSpec &spec : specs) {
      if (spec.regcls & REGCLS_MULTIPLEUSE) {
        m_hasMultipleUse = true;
        break;
      }
    }
  }
}

void RunAsStandalone::AddServerReference() {
  CoAddRefServerProcess();
  ++m_acquisitionCount;
}

void RunAsStandalone::ReleaseServerReference() {
  bool shouldExit = CoReleaseServerProcess() == 0;
  if (shouldExit) {
    if (m_hasMultipleUse) {
      CheckForExitLater();
    } else {
      ExitApplicationLater();
    }
  }
}
