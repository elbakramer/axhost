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

#include "enum_connections.h"

HostEnumConnections::HostEnumConnections(
    IEnumConnections *underlying, HostConnectionPoint *connectionPoint
)
    : m_underlying(underlying),
      m_connectionPoint(connectionPoint) {}

HRESULT STDMETHODCALLTYPE HostEnumConnections::Next(
    ULONG cConnections, LPCONNECTDATA rgcd, ULONG *pcFetched
) {
  if (!rgcd)
    return E_POINTER;
  if (cConnections > 1 && !pcFetched)
    return E_POINTER;
  if (!m_underlying)
    return E_UNEXPECTED;
  ULONG fetched = 0;
  HRESULT hr = m_underlying->Next(cConnections, rgcd, &fetched);
  if (pcFetched) {
    *pcFetched = fetched;
  }
  for (ULONG i = 0; i < fetched; ++i) {
    DWORD dwCookie = rgcd[i].dwCookie;
    CComPtr<IUnknown> underlying;
    if (!m_connectionPoint)
      return E_UNEXPECTED;
    HRESULT hr = m_connectionPoint->GetUnderlyingSink(dwCookie, &underlying);
    if (FAILED(hr))
      continue;
    if (!underlying)
      continue;
    rgcd[i].pUnk->Release();
    rgcd[i].pUnk = underlying.Detach();
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE HostEnumConnections::Skip(ULONG cConnections) {
  if (!m_underlying)
    return E_UNEXPECTED;
  return m_underlying->Skip(cConnections);
}

HRESULT STDMETHODCALLTYPE HostEnumConnections::Reset() {
  if (!m_underlying)
    return E_UNEXPECTED;
  return m_underlying->Reset();
}

HRESULT STDMETHODCALLTYPE
HostEnumConnections::Clone(IEnumConnections **ppEnum) {
  if (!ppEnum)
    return E_POINTER;
  *ppEnum = nullptr;
  if (!m_underlying)
    return E_UNEXPECTED;
  CComPtr<IEnumConnections> underlyingClone;
  HRESULT hr = m_underlying->Clone(&underlyingClone);
  if (FAILED(hr))
    return hr;
  if (!m_connectionPoint)
    return E_UNEXPECTED;
  CComPtr<HostEnumConnections> cloneConcrete =
      new HostEnumConnections(underlyingClone, m_connectionPoint);
  if (!cloneConcrete)
    return E_OUTOFMEMORY;
  CComQIPtr<IEnumConnections> clone = cloneConcrete.p;
  if (!clone)
    return E_UNEXPECTED;
  *ppEnum = clone;
  return hr;
}
