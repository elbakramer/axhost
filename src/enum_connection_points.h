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

#ifndef ENUM_CONNECTION_POINTS_H
#define ENUM_CONNECTION_POINTS_H

#include <atlcomcli.h>

#include "connection_point_container.h"
#include "unknown_impl.h"

class HostConnectionPointContainer;

class HostEnumConnectionPoints : public CUnknownImpl<IEnumConnectionPoints> {
private:
  CComPtr<IEnumConnectionPoints> m_underlying;
  CComPtr<HostConnectionPointContainer> m_container;

public:
  HostEnumConnectionPoints(
      IEnumConnectionPoints *underlying, HostConnectionPointContainer *container
  );

public:
  HRESULT STDMETHODCALLTYPE
  Next(ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFetched) override;
  HRESULT STDMETHODCALLTYPE Skip(ULONG cConnections) override;
  HRESULT STDMETHODCALLTYPE Reset() override;
  HRESULT STDMETHODCALLTYPE Clone(IEnumConnectionPoints **ppEnum) override;
};

#endif // ENUM_CONNECTION_POINTS_H
