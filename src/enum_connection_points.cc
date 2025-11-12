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

#include "enum_connection_points.h"

HostEnumConnectionPoints::HostEnumConnectionPoints(
    IEnumConnectionPoints *underlying, HostConnectionPointContainer *container
)
    : m_underlying(underlying),
      m_container(container) {}

HRESULT STDMETHODCALLTYPE HostEnumConnectionPoints::Next(
    ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFetched
) {
  if (!ppCP)
    return E_POINTER;
  if (cConnections > 1 && !pcFetched)
    return E_POINTER;
  if (!m_underlying)
    return E_UNEXPECTED;
  ULONG fetched = 0;
  HRESULT hr = m_underlying->Next(cConnections, ppCP, &fetched);
  if (pcFetched) {
    *pcFetched = fetched;
  }
  for (ULONG i = 0; i < fetched; ++i) {
    CComPtr<IConnectionPoint> underlyingConcrete = ppCP[i];
    CComPtr<IUnknown> underlying;
    HRESULT qry =
        underlyingConcrete->QueryInterface(IID_IUnknown, (void **)&underlying);
    if (FAILED(qry))
      return qry;
    if (!underlying)
      return E_UNEXPECTED;
    CComPtr<IConnectionPoint> proxy;
    HRESULT hr = m_container->GetProxyConnectionPoint(underlying, &proxy);
    if (FAILED(hr))
      continue;
    if (!proxy)
      continue;
    ppCP[i]->Release();
    ppCP[i] = proxy.Detach();
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE HostEnumConnectionPoints::Skip(ULONG cConnections) {
  if (!m_underlying)
    return E_UNEXPECTED;
  return m_underlying->Skip(cConnections);
}

HRESULT STDMETHODCALLTYPE HostEnumConnectionPoints::Reset() {
  if (!m_underlying)
    return E_UNEXPECTED;
  return m_underlying->Reset();
}

HRESULT STDMETHODCALLTYPE
HostEnumConnectionPoints::Clone(IEnumConnectionPoints **ppEnum) {
  if (!ppEnum)
    return E_POINTER;
  *ppEnum = nullptr;
  if (!m_underlying)
    return E_UNEXPECTED;
  CComPtr<IEnumConnectionPoints> underlyingClone;
  HRESULT hr = m_underlying->Clone(&underlyingClone);
  if (FAILED(hr))
    return hr;
  if (!underlyingClone)
    return E_UNEXPECTED;
  if (!m_container)
    return E_UNEXPECTED;
  CComPtr<HostEnumConnectionPoints> cloneConcrete =
      new HostEnumConnectionPoints(underlyingClone, m_container);
  if (!cloneConcrete)
    return E_OUTOFMEMORY;
  CComQIPtr<IEnumConnectionPoints> clone = cloneConcrete.p;
  if (!clone)
    return E_UNEXPECTED;
  *ppEnum = clone.Detach();
  return hr;
}
