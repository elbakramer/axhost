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

#include "surrogate.h"

#include <atlcomcli.h>
#include <windows.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QString>

#include "class_spec.h"
#include "container_factory.h"
#include "utils.h"

HRESULT
HostSurrogate::LoadDllServerEx(
    REFCLSID clsid, REFCLSID alias, DWORD clsctx_create, DWORD clsctx_register,
    DWORD regcls
) {
  DWORD cookie = 0;
  CComPtr<IClassFactory> original;
  CComPtr<IClassFactory> surrogate;
  bool does_auto_inprog_registration =
      (clsctx_register & CLSCTX_LOCAL_SERVER) && (regcls & REGCLS_MULTIPLEUSE);
  bool can_workaround_short_circuit = (clsctx_create & CLSCTX_INPROC_SERVER) &&
      !(clsctx_create & CLSCTX_LOCAL_SERVER);
  if (does_auto_inprog_registration && can_workaround_short_circuit) {
    HRESULT get_org = CoGetClassObject(
        clsid, clsctx_create, NULL, IID_IClassFactory, (void **)&original
    );
    if (FAILED(get_org) && alias == clsid) {
      QString text = QString(R"(
Warning: Original InProc Class Factory Not Available

Multiple-use mode with alias == source requires the original InProc DLL.
CoGetClassObject failed for CLSID: %1

Self-instantiation may occur. Consider using an explicit alias.
)")
                         .arg(QUuid(clsid).toString())
                         .trimmed();
      QMessageBox::warning(nullptr, QCoreApplication::applicationName(), text);
    }
  }
  surrogate = new HostContainerFactory(clsid, clsctx_create);
  HRESULT reg =
      CoRegisterClassObject(alias, surrogate, clsctx_register, regcls, &cookie);
  if (FAILED(reg)) {
    return reg;
  }
  m_cookies.insert(cookie);
  if (original) {
    HRESULT reg_cls = CoRegisterClassObject(
        clsid, original, clsctx_create,
        REGCLS_MULTIPLEUSE | REGCLS_MULTI_SEPARATE, &cookie
    );
    if (SUCCEEDED(reg_cls)) {
      m_cookies.insert(cookie);
    }
  }
  return reg;
}

HRESULT
HostSurrogate::LoadDllServerEx(REFCLSID clsid, DWORD clsctx, DWORD regcls) {
  return LoadDllServerEx(clsid, clsid, CLSCTX_INPROC_SERVER, clsctx, regcls);
}

HRESULT
HostSurrogate::RegisterAllClassFactories(
    QList<ClassSpec> &specs, BOOL suspend, DWORD regcls
) {
  HRESULT hrOverall = S_OK;
  if (suspend) {
    regcls |= REGCLS_SUSPENDED;
  }
  if (true) {
    regcls |= REGCLS_MULTI_SEPARATE;
  }
  for (ClassSpec &spec : specs) {
    spec.sanitize(regcls);
    spec.result = LoadDllServerEx(
        spec.clsid, spec.alias, spec.clsctx_create, spec.clsctx_register,
        spec.regcls
    );
    if (FAILED(spec.result)) {
      spec.error = GetLastError();
      {
        QString msg = GetLastErrorMessage(spec.error);
        QString text = QString(R"(
Error: Class Registration Failed

CoRegisterClassObject failed.

CLSID: '%1'
CLSCTX: 0x%2

Error message:
%3)")
                           .arg(spec.alias_input)
                           .arg(QString::number(spec.clsctx_register, 16))
                           .arg(msg)
                           .trimmed();
        QMessageBox::warning(
            nullptr, QCoreApplication::applicationName(), text
        );
      }
      if (SUCCEEDED(hrOverall)) {
        hrOverall = spec.result;
      }
      continue;
    }
  }
  return hrOverall;
}

HRESULT HostSurrogate::RevokeAllClassFactories() {
  HRESULT hrOverall = S_OK;
  for (auto it = m_cookies.begin(); it != m_cookies.end();) {
    DWORD cookie = *it;
    HRESULT hr = CoRevokeClassObject(cookie);
    if (SUCCEEDED(hr)) {
      it = m_cookies.erase(it);
    } else {
      if (SUCCEEDED(hrOverall)) {
        hrOverall = hr;
      }
      ++it;
    }
  }
  return hrOverall;
}

ULONG HostSurrogate::GetRegisteredClassCount() { return m_cookies.size(); }

BOOL HostSurrogate::HasRegisteredClass() { return !m_cookies.empty(); }

HRESULT STDMETHODCALLTYPE HostSurrogate::LoadDllServer(REFCLSID clsid) {
  return LoadDllServerEx(clsid);
}

HRESULT STDMETHODCALLTYPE HostSurrogate::FreeSurrogate() {
  HRESULT hr = RevokeAllClassFactories();
  bool invoked = ExitApplicationLater();
  if (SUCCEEDED(hr) && !invoked) {
    hr = E_UNEXPECTED;
  }
  return hr;
}
