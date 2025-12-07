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

#include "connection_point.h"

#include <QtConcurrent>

#include <QFuture>
#include <QSharedPointer>
#include <QThreadPool>
#include <QThreadStorage>

#include "com_initialize_context.h"
#include "enum_connections.h"
#include "sink.h"

static QThreadStorage<QSharedPointer<ComInitializeContext>> g_tls;
static QThreadPool g_threadPool;

HostConnectionPoint::HostConnectionPoint(
    IConnectionPoint *underlying, HostConnectionPointContainer *container
)
    : m_underlying(underlying),
      m_container(container) {}

HRESULT
HostConnectionPoint::GetUnderlyingSink(DWORD dwCookie, IUnknown **ppUnk) {
  if (!ppUnk)
    return E_POINTER;
  auto search = m_underlyingConnections.find(dwCookie);
  if (search == m_underlyingConnections.end())
    return E_UNEXPECTED;
  CComPtr<IUnknown> underlying = search->second;
  if (!underlying)
    return E_UNEXPECTED;
  *ppUnk = underlying.Detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
HostConnectionPoint::GetConnectionInterface(IID *pIID) {
  if (!pIID)
    return E_POINTER;
  if (!m_underlying)
    return E_UNEXPECTED;
  return m_underlying->GetConnectionInterface(pIID);
}

HRESULT STDMETHODCALLTYPE HostConnectionPoint::GetConnectionPointContainer(
    IConnectionPointContainer **ppCPC
) {
  if (!ppCPC)
    return E_POINTER;
  if (!m_container)
    return E_UNEXPECTED;
  CComQIPtr<IConnectionPointContainer> proxy = m_container.p;
  *ppCPC = proxy.Detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
HostConnectionPoint::Advise(IUnknown *pUnkSink, DWORD *pdwCookie) {
  if (!pUnkSink || !pdwCookie)
    return E_INVALIDARG;
  CComPtr<HostEventSink> proxyConcrete = new HostEventSink(pUnkSink);
  if (!proxyConcrete) {
    return E_OUTOFMEMORY;
  }
  CComPtr<IUnknown> proxy;
  HRESULT qry = proxyConcrete->QueryInterface(IID_IUnknown, (void **)&proxy);
  if (FAILED(qry))
    return qry;
  if (!proxy)
    return E_UNEXPECTED;
  HRESULT hr = m_underlying->Advise(proxy, pdwCookie);
  if (SUCCEEDED(hr)) {
    DWORD dwCookie = *pdwCookie;
    m_connections.emplace(dwCookie, proxy);
    m_underlyingConnections.emplace(dwCookie, pUnkSink);
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE HostConnectionPoint::Unadvise(DWORD dwCookie) {
  HRESULT hr = m_underlying->Unadvise(dwCookie);
  if (SUCCEEDED(hr)) {
    m_connections.erase(dwCookie);
    m_underlyingConnections.erase(dwCookie);
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE
HostConnectionPoint::EnumConnections(IEnumConnections **ppEnum) {
  if (!ppEnum)
    return E_POINTER;
  CComPtr<IEnumConnections> underlying;
  HRESULT hr = m_underlying->EnumConnections(&underlying);
  if (FAILED(hr))
    return hr;
  if (!underlying)
    return E_UNEXPECTED;
  CComPtr<HostEnumConnections> proxyConcrete =
      new HostEnumConnections(underlying, this);
  if (!proxyConcrete)
    return E_OUTOFMEMORY;
  CComQIPtr<IEnumConnections> proxy = proxyConcrete.p;
  if (!proxy)
    return E_UNEXPECTED;
  *ppEnum = proxy.Detach();
  return hr;
}
