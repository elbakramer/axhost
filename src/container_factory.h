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

#ifndef CONTAINER_FACTORY_H
#define CONTAINER_FACTORY_H

#include <atomic>

#include <atlcomcli.h>
#include <windows.h>

#include <QUuid>

class HostContainerFactory : public IClassFactory, public IMarshal {
private:
  std::atomic<ULONG> m_ref{0};

  QUuid m_classId;
  DWORD m_classContext;

  CComPtr<IClassFactory> m_underlying;

  IUnknown *m_unk;
  IUnknown *m_underlyingUnk;

  IUnknown *GetInterfaceToBeMarshaled(REFIID riid);

public:
  HostContainerFactory(REFCLSID clsid, DWORD clsctx = CLSCTX_SERVER);
  ~HostContainerFactory();

  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override;

  HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;
  HRESULT STDMETHODCALLTYPE
  CreateInstance(IUnknown *outer, REFIID riid, void **ppv) override;

  HRESULT STDMETHODCALLTYPE GetUnmarshalClass(
      REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext,
      DWORD mshlflags, CLSID *pCid
  ) override;
  HRESULT STDMETHODCALLTYPE GetMarshalSizeMax(
      REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext,
      DWORD mshlflags, DWORD *pSize
  ) override;
  HRESULT STDMETHODCALLTYPE MarshalInterface(
      IStream *pStm, REFIID riid, void *pv, DWORD dwDestContext,
      void *pvDestContext, DWORD mshlflags
  ) override;
  HRESULT STDMETHODCALLTYPE
  UnmarshalInterface(IStream *pStm, REFIID riid, void **ppv) override;
  HRESULT STDMETHODCALLTYPE ReleaseMarshalData(IStream *pStm) override;
  HRESULT STDMETHODCALLTYPE DisconnectObject(DWORD dwReserved) override;
};

#endif // CONTAINER_FACTORY_H
