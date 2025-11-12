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

#ifndef EXTERNAL_CONNECTION_H
#define EXTERNAL_CONNECTION_H

#include <atomic>

#include <atlcomcli.h>
#include <objidl.h>
#include <windows.h>

#include "unknown_impl.h"

class HostExternalConnection : public CUnknownImpl<IExternalConnection> {
private:
  std::atomic<ULONG> m_con{0};
  CComPtr<IExternalConnection> m_underlying;

public:
  HostExternalConnection(IExternalConnection *underlying);

public:
  DWORD STDMETHODCALLTYPE AddConnection(DWORD extconn, DWORD reserved) override;
  DWORD STDMETHODCALLTYPE ReleaseConnection(
      DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses
  ) override;
};

#endif // EXTERNAL_CONNECTION_H
