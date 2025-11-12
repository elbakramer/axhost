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

#include "container_factory_registry.h"

#include <atlcomcli.h>
#include <windows.h>

#include <QCoreApplication>
#include <QList>
#include <QMessageBox>

#include "class_spec.h"
#include "container_factory.h"
#include "utils.h"

HostContainerFactoryRegistry::HostContainerFactoryRegistry(
    const QList<ClassSpec> &specs
) {
  HRESULT sus = CoSuspendClassObjects();
  if (FAILED(sus)) {
    return;
  }
  int success = 0;
  for (const ClassSpec &spec : specs) {
    CLSID classId;
    HRESULT conv = CLSIDFromString(spec.alias.toStdWString().c_str(), &classId);
    if (FAILED(conv)) {
      QString text = QString(R"(
Error: CLSID Parsing Failed

Invalid CLSID: '%1'
)")
                         .arg(spec.alias)
                         .trimmed();
      QMessageBox::warning(nullptr, QCoreApplication::applicationName(), text);
      continue;
    }
    CComPtr<IClassFactory> factory = new HostContainerFactory(spec);
    DWORD cookie = 0;
    HRESULT reg = CoRegisterClassObject(
        classId, factory, spec.clsctx_register, spec.regcls, &cookie
    );
    if (SUCCEEDED(reg)) {
      m_registry.push_back(cookie);
      ++success;
    } else {
      DWORD err = GetLastError();
      QString msg = GetLastErrorMessage(err);
      QString text = QString(R"(
Error: Class Registration Failed

CoRegisterClassObject failed.

CLSID: '%1'
CLSCTX: 0x%2

Error message:
%3)")
                         .arg(spec.alias)
                         .arg(QString::number(spec.clsctx_register, 16))
                         .arg(msg)
                         .trimmed();
      QMessageBox::warning(nullptr, QCoreApplication::applicationName(), text);
    }
  }
  if (SUCCEEDED(sus)) {
    HRESULT res = CoResumeClassObjects();
  }
  if (success == 0) {
    QString text = QString(R"(
Error: No Class Registered

No class factory could be registered. Exiting.
)")
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    DWORD err = GetLastError();
    QCoreApplication::exit(err);
  }
}

HostContainerFactoryRegistry::~HostContainerFactoryRegistry() {
  HRESULT sus = CoSuspendClassObjects();
  for (DWORD cookie : m_registry) {
    HRESULT hr = CoRevokeClassObject(cookie);
  }
  if (SUCCEEDED(sus)) {
    HRESULT res = CoResumeClassObjects();
  }
}
