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

#ifndef SURROGATE_RUNTIME_H
#define SURROGATE_RUNTIME_H

#include <atomic>

#include <atlcomcli.h>
#include <windows.h>

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include "class_spec.h"
#include "ready_event.h"
#include "surrogate.h"

class HostSurrogateRuntime : public QObject {
  Q_OBJECT

protected:
  inline static HostSurrogateRuntime *s_instance = nullptr;

  CComPtr<HostSurrogate> m_surrogate;

  ulong m_freeUnusedLibrariesInterval = 60000;
  QElapsedTimer m_freeUnusedLibrariesTimer;

  ulong m_checkForExitTimeout = 60000;
  QTimer m_checkForExitTimer;

  std::atomic<ulong> m_acquisitionCount{0};

  bool m_warnedIdle = false;
  bool m_hasMultipleUse = false;

protected:
  void InitializeStaticInstance();
  void StartUnusedLibraryCleaner();
  void StartExitConditionChecker();

public:
  HostSurrogateRuntime(QObject *parent = nullptr);
  virtual ~HostSurrogateRuntime();

  static HostSurrogateRuntime *instance();

protected slots:
  void CheckForExit();
  void CheckForExitLater();

  void FreeUnusedLibraries();
  void OnAboutToBlock();

public slots:
  virtual void AddServerReference();
  virtual void ReleaseServerReference();
};

class RunAsSurrogate : public HostSurrogateRuntime {
  Q_OBJECT

public:
  RunAsSurrogate(const QUuid &clsid, bool embedding, QObject *parent = nullptr);
  virtual ~RunAsSurrogate() override;
};

class RunAsStandalone : public HostSurrogateRuntime {
  Q_OBJECT

private:
  HostReadyEvent m_readyEvent;

private:
  void CheckMultipleUse(QList<ClassSpec> &specs, DWORD regcls);

public:
  RunAsStandalone(
      QList<ClassSpec> &specs, const QString &readyEvent, DWORD regcls,
      QObject *parent = nullptr
  );
  virtual ~RunAsStandalone() override;

public slots:
  virtual void AddServerReference() override;
  virtual void ReleaseServerReference() override;
};

#endif // SURROGATE_RUNTIME_H
