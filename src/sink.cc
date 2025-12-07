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

#include "sink.h"

#include <wil/resource.h>

#include <QtConcurrent>

#include <QFuture>

QSharedPointer<QThreadPool> HostEventSink::g_threadPool;
CComPtr<IGlobalInterfaceTable> HostEventSink::g_git;
QThreadStorage<QSharedPointer<ComInitializeContext>> HostEventSink::g_tls;

QSharedPointer<QThreadPool> &HostEventSink::GetThreadPool() {
  if (!g_threadPool) {
    g_threadPool = QSharedPointer<QThreadPool>::create();
  }
  return g_threadPool;
}

CComPtr<IGlobalInterfaceTable> &HostEventSink::GetGlobalInterfaceTable() {
  if (!g_git) {
    HRESULT hr = CoCreateInstance(
        CLSID_StdGlobalInterfaceTable, nullptr, CLSCTX_INPROC_SERVER,
        IID_IGlobalInterfaceTable, (void **)&g_git
    );
  }
  return g_git;
}

QThreadStorage<QSharedPointer<ComInitializeContext>> &
HostEventSink::GetThreadStorage() {
  return g_tls;
}

CComPtr<IDispatch> HostEventSink::CreateGlobalSinkInThread(DWORD cookie) {
  if (!g_tls.hasLocalData()) {
    g_tls.setLocalData(
        QSharedPointer<ComInitializeContext>::create(COINIT_MULTITHREADED)
    );
  }
  CComPtr<IDispatch> sink;
  HRESULT hr = GetGlobalInterfaceTable()->GetInterfaceFromGlobal(
      cookie, IID_IDispatch, (void **)&sink
  );
  return sink;
}

HRESULT HostEventSink::InvokeInThread(
    DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
    DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
    UINT *puArgErr, DWORD cookie, HANDLE hEvent
) {
  if (!g_tls.hasLocalData()) {
    g_tls.setLocalData(
        QSharedPointer<ComInitializeContext>::create(COINIT_MULTITHREADED)
    );
  }
  CComPtr<IGlobalInterfaceTable> &git = GetGlobalInterfaceTable();
  CComPtr<IDispatch> sink;
  HRESULT hr =
      git->GetInterfaceFromGlobal(cookie, IID_IDispatch, (void **)&sink);
  if (FAILED(hr))
    return hr;
  if (!sink)
    return E_UNEXPECTED;
  hr = sink->Invoke(
      dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo,
      puArgErr
  );
  SetEvent(hEvent);
  return hr;
}

HRESULT
HostEventSink::CreateGlobalSink(IUnknown *pUnkSink) {
  if (!pUnkSink)
    return E_INVALIDARG;
  CComPtr<IDispatch> sink;
  HRESULT hr = pUnkSink->QueryInterface(IID_IDispatch, (void **)&sink);
  if (FAILED(hr))
    return hr;
  if (!sink)
    return E_UNEXPECTED;
  CComPtr<IGlobalInterfaceTable> &git = GetGlobalInterfaceTable();
  if (!git)
    return E_UNEXPECTED;
  return git->RegisterInterfaceInGlobal(sink, IID_IDispatch, &m_cookie);
}

HRESULT HostEventSink::RemoveGlobalSink() {
  if (m_cookie) {
    CComPtr<IGlobalInterfaceTable> &git = GetGlobalInterfaceTable();
    if (!git)
      return E_UNEXPECTED;
    HRESULT hr = git->RevokeInterfaceFromGlobal(m_cookie);
    m_cookie = 0;
    return hr;
  }
  return S_OK;
}

HostEventSink::HostEventSink(IUnknown *underlying)
    : m_underlying(underlying) {
  m_underlyingDispatch = m_underlying;
  HRESULT hr = CreateGlobalSink(m_underlying);
}

HostEventSink::~HostEventSink() { HRESULT hr = RemoveGlobalSink(); }

HRESULT STDMETHODCALLTYPE HostEventSink::GetTypeInfoCount(UINT *pctinfo) {
  if (pctinfo)
    *pctinfo = 0;
  return S_OK;
}
HRESULT STDMETHODCALLTYPE HostEventSink::GetTypeInfo(UINT, LCID, ITypeInfo **) {
  return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE
HostEventSink::GetIDsOfNames(REFIID, LPOLESTR *, UINT, LCID, DISPID *) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE HostEventSink::Invoke(
    DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
    DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
    UINT *puArgErr
) {
  if (riid != IID_NULL)
    return DISP_E_UNKNOWNINTERFACE;
  if (!m_cookie)
    return E_UNEXPECTED;
  wil::unique_event hEvent;
  hEvent.create(wil::EventOptions::None);
  if (!hEvent)
    return E_FAIL;
  HANDLE hEventRaw = hEvent.get();
  QFuture<HRESULT> res = QtConcurrent::run(
      GetThreadPool().data(), InvokeInThread, dispIdMember, riid, lcid, wFlags,
      pDispParams, pVarResult, pExcepInfo, puArgErr, m_cookie, hEventRaw
  );
  DWORD index = 0;
  HRESULT hr;
  while (true) {
    hr = CoWaitForMultipleHandles(
        COWAIT_INPUTAVAILABLE | COWAIT_DISPATCH_CALLS, INFINITE, 1, &hEventRaw,
        &index
    );
    if (FAILED(hr)) {
      return hr;
    }
    if (index == WAIT_OBJECT_0)
      break;
  }
  hr = res.result();
  return hr;
}
