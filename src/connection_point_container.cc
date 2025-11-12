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

#include "connection_point_container.h"

#include "connection_point.h"
#include "enum_connection_points.h"

HostConnectionPointContainer::HostConnectionPointContainer(
    IConnectionPointContainer *underlying
)
    : m_underlying(underlying) {}

HRESULT HostConnectionPointContainer::GetProxyConnectionPoint(
    IUnknown *pCP, IConnectionPoint **ppCP
) {
  if (!ppCP)
    return E_POINTER;
  if (!pCP)
    return E_INVALIDARG;
  auto search = m_proxyConnectionPoints.find(pCP);
  CComPtr<IConnectionPoint> proxy;
  if (search != m_proxyConnectionPoints.end()) {
    CComPtr<IConnectionPoint> proxyInner = search->second;
    proxy = proxyInner;
  } else {
    CComQIPtr<IConnectionPoint> underlying = pCP;
    if (!underlying)
      return E_UNEXPECTED;
    CComPtr<HostConnectionPoint> proxyConcrete =
        new HostConnectionPoint(underlying, this);
    if (!proxyConcrete)
      return E_OUTOFMEMORY;
    CComQIPtr<IConnectionPoint> proxyInner = proxyConcrete.p;
    if (!proxyInner)
      return E_UNEXPECTED;
    m_proxyConnectionPoints.emplace(pCP, proxyInner.p);
    proxy = proxyInner;
  }
  if (!proxy)
    return E_UNEXPECTED;
  *ppCP = proxy.Detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE HostConnectionPointContainer::EnumConnectionPoints(
    IEnumConnectionPoints **ppEnum
) {
  if (!ppEnum)
    return E_POINTER;
  if (!m_underlying)
    return E_UNEXPECTED;
  CComPtr<IEnumConnectionPoints> underlying;
  HRESULT hr = m_underlying->EnumConnectionPoints(&underlying);
  if (FAILED(hr))
    return hr;
  if (!underlying)
    return E_UNEXPECTED;
  CComPtr<HostEnumConnectionPoints> proxyConcrete =
      new HostEnumConnectionPoints(underlying, this);
  if (!proxyConcrete)
    return E_OUTOFMEMORY;
  CComQIPtr<IEnumConnectionPoints> proxy = proxyConcrete.p;
  if (!proxy)
    return E_UNEXPECTED;
  *ppEnum = proxy.Detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE HostConnectionPointContainer::FindConnectionPoint(
    REFIID riid, IConnectionPoint **ppCP
) {
  if (!ppCP)
    return E_POINTER;
  if (!m_underlying)
    return E_UNEXPECTED;
  CComPtr<IConnectionPoint> underlying;
  HRESULT hr = m_underlying->FindConnectionPoint(riid, &underlying);
  if (FAILED(hr))
    return hr;
  if (!underlying)
    return E_UNEXPECTED;
  CComPtr<IUnknown> underlyingUnk;
  HRESULT qry =
      underlying->QueryInterface(IID_IUnknown, (void **)&underlyingUnk);
  if (FAILED(qry))
    return qry;
  if (!underlyingUnk)
    return E_UNEXPECTED;
  CComPtr<IConnectionPoint> proxy;
  hr = GetProxyConnectionPoint(underlyingUnk, &proxy);
  if (FAILED(hr))
    return hr;
  if (!proxy)
    return E_UNEXPECTED;
  *ppCP = proxy.Detach();
  return hr;
}
