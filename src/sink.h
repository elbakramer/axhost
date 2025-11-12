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

#ifndef SINK_H
#define SINK_H

#include <atlcomcli.h>

#include <QSharedPointer>
#include <QThreadPool>
#include <QThreadStorage>

#include "com_initialize_context.h"
#include "unknown_impl.h"

class HostEventSink : public CUnknownImpl<IDispatch> {
private:
  CComPtr<IUnknown> m_underlying;
  CComQIPtr<IDispatch> m_underlyingDispatch;
  CComPtr<IDispatch> m_underlyingGlobalDispatch;
  DWORD m_cookie;

private:
  static QSharedPointer<QThreadPool> g_threadPool;
  static CComPtr<IGlobalInterfaceTable> g_git;
  static QThreadStorage<QSharedPointer<ComInitializeContext>> g_tls;

  static QSharedPointer<QThreadPool> &GetThreadPool();
  static CComPtr<IGlobalInterfaceTable> &GetGlobalInterfaceTable();
  static QThreadStorage<QSharedPointer<ComInitializeContext>> &
  GetThreadStorage();

  static CComPtr<IDispatch> CreateGlobalSinkInThread(DWORD cookie);
  static HRESULT InvokeInThread(
      DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
      DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
      UINT *puArgErr, DWORD cookie, HANDLE hEvent
  );

  HRESULT
  CreateGlobalSink(IUnknown *pUnkSink);
  HRESULT RemoveGlobalSink();

public:
  HostEventSink(IUnknown *underlying);
  ~HostEventSink();

public:
  HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo) override;
  HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo **) override;
  HRESULT STDMETHODCALLTYPE
  GetIDsOfNames(REFIID, LPOLESTR *, UINT, LCID, DISPID *) override;
  HRESULT STDMETHODCALLTYPE Invoke(
      DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
      DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
      UINT *puArgErr
  );
};

#endif // SINK_H
