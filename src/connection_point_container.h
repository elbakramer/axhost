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

#ifndef CONNECTION_POINT_CONTAINER_H
#define CONNECTION_POINT_CONTAINER_H

#include <unordered_map>

#include <atlcomcli.h>

#include "unknown_impl.h"

class HostConnectionPointContainer
    : public CUnknownImpl<IConnectionPointContainer> {
private:
  CComPtr<IConnectionPointContainer> m_underlying;
  std::unordered_map<IUnknown *, CComPtr<IConnectionPoint>>
      m_proxyConnectionPoints;

public:
  HostConnectionPointContainer(IConnectionPointContainer *underlying);

public:
  HRESULT GetProxyConnectionPoint(IUnknown *pCP, IConnectionPoint **ppCP);

public:
  HRESULT STDMETHODCALLTYPE
  EnumConnectionPoints(IEnumConnectionPoints **ppEnum) override;
  HRESULT STDMETHODCALLTYPE
  FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP) override;
};

#endif // CONNECTION_POINT_CONTAINER_H
