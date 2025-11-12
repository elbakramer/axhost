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

#ifndef CONNECTION_POINT_H
#define CONNECTION_POINT_H

#include <unordered_map>

#include <atlcomcli.h>

#include "connection_point_container.h"
#include "unknown_impl.h"

class HostConnectionPointContainer;

class HostConnectionPoint : public CUnknownImpl<IConnectionPoint> {
private:
  CComPtr<IConnectionPoint> m_underlying;
  CComPtr<HostConnectionPointContainer> m_container;

  std::unordered_map<DWORD, CComPtr<IUnknown>> m_connections;
  std::unordered_map<DWORD, CComPtr<IUnknown>> m_underlyingConnections;

public:
  HostConnectionPoint(
      IConnectionPoint *underlying, HostConnectionPointContainer *container
  );

public:
  HRESULT GetUnderlyingSink(DWORD dwCookie, IUnknown **ppUnk);

public:
  HRESULT STDMETHODCALLTYPE GetConnectionInterface(IID *pIID) override;
  HRESULT STDMETHODCALLTYPE
  GetConnectionPointContainer(IConnectionPointContainer **ppCPC) override;
  HRESULT STDMETHODCALLTYPE
  Advise(IUnknown *pUnkSink, DWORD *pdwCookie) override;
  HRESULT STDMETHODCALLTYPE Unadvise(DWORD dwCookie) override;
  HRESULT STDMETHODCALLTYPE EnumConnections(IEnumConnections **ppEnum) override;
};

#endif // CONNECTION_POINT_H
