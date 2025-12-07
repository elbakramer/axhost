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

#ifndef SURROGATE_H
#define SURROGATE_H

#include <QList>
#include <QSet>

#include "class_spec.h"
#include "unknown_impl.h"

class HostSurrogate : public CUnknownImpl<ISurrogate> {
private:
  QSet<DWORD> m_cookies;

public:
  HRESULT LoadDllServerEx(
      REFCLSID clsid, REFCLSID alias,
      DWORD clsctx_create = CLSCTX_INPROC_SERVER,
      DWORD clsctx_register = CLSCTX_LOCAL_SERVER,
      DWORD regcls = REGCLS_SURROGATE | REGCLS_MULTI_SEPARATE
  );
  HRESULT LoadDllServerEx(
      REFCLSID clsid, DWORD clsctx = CLSCTX_LOCAL_SERVER,
      DWORD regcls = REGCLS_SURROGATE | REGCLS_MULTI_SEPARATE
  );
  HRESULT
  RegisterAllClassFactories(
      QList<ClassSpec> &specs, BOOL suspend = FALSE, DWORD regcls = 0
  );
  HRESULT RevokeAllClassFactories();
  ULONG GetRegisteredClassCount();
  BOOL HasRegisteredClass();

public:
  HRESULT STDMETHODCALLTYPE LoadDllServer(REFCLSID clsid) override;
  HRESULT STDMETHODCALLTYPE FreeSurrogate() override;
};

#endif // SURROGATE_H
