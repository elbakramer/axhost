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

#include "container_factory.h"

#include <wil/result.h>
#include <windows.h>

#include "class_spec.h"
#include "container.h"
#include "surrogate_runtime.h"
#include "unknown_impl.h"

IUnknown *HostContainerFactory::GetInterfaceToBeMarshaled(REFIID riid) {
  if (riid == IID_IClassFactory || riid == IID_IUnknown) {
    return m_unk;
  } else if (m_underlyingUnk) {
    return m_underlyingUnk;
  } else {
    return m_unk;
  }
}

HostContainerFactory::HostContainerFactory(REFCLSID clsid, DWORD clsctx)
    : m_classId(clsid),
      m_classContext(clsctx) {
  m_unk = static_cast<IClassFactory *>(this);

  [&] {
    RETURN_IF_FAILED(CoGetClassObject(
        clsid, clsctx, nullptr, IID_IClassFactory, (void **)&m_underlying
    ));

    if (m_underlying) {
      RETURN_IF_FAILED(
          m_underlying->QueryInterface(IID_IUnknown, (void **)&m_underlyingUnk)
      );
      if (m_underlyingUnk) {
        m_underlyingUnk->Release();
      }
    }

    return S_OK;
  }();
}

HostContainerFactory::~HostContainerFactory() {}

ULONG STDMETHODCALLTYPE HostContainerFactory::AddRef() { return ++m_ref; }

ULONG STDMETHODCALLTYPE HostContainerFactory::Release() {
  ULONG n = --m_ref;
  if (n == 0)
    delete this;
  return n;
}

HRESULT STDMETHODCALLTYPE
HostContainerFactory::QueryInterface(REFIID riid, void **ppv) {
  if (!ppv)
    return E_POINTER;
  *ppv = nullptr;
  if (riid == IID_IUnknown) {
    *ppv = static_cast<IClassFactory *>(this);
  } else if (riid == IID_IClassFactory) {
    *ppv = static_cast<IClassFactory *>(this);
  } else if (riid == IID_IMarshal) {
    *ppv = static_cast<IMarshal *>(this);
  } else {
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE HostContainerFactory::LockServer(BOOL fLock) {
  if (auto *runtime = HostSurrogateRuntime::instance()) {
    if (fLock) {
      runtime->AddServerReference();
    } else {
      runtime->ReleaseServerReference();
    }
  }
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
HostContainerFactory::CreateInstance(IUnknown *outer, REFIID riid, void **ppv) {
  if (!ppv)
    return E_POINTER;
  *ppv = nullptr;
  if (outer)
    return CLASS_E_NOAGGREGATION;
  CComPtr<HostContainer> container =
      new HostContainer(m_classId, m_classContext);
  if (!container)
    return E_OUTOFMEMORY;
  if (!container->IsInitialized())
    return CLASS_E_CLASSNOTAVAILABLE;
  return container->QueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE HostContainerFactory::GetUnmarshalClass(
    REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext,
    DWORD mshlflags, CLSID *pCid
) {
  CComPtr<IMarshal> marshal;
  RETURN_IF_FAILED(CoGetStandardMarshal(
      riid, GetInterfaceToBeMarshaled(riid), dwDestContext, pvDestContext,
      mshlflags, &marshal
  ));
  return marshal->GetUnmarshalClass(
      riid, pv, dwDestContext, pvDestContext, mshlflags, pCid
  );
}

HRESULT STDMETHODCALLTYPE HostContainerFactory::GetMarshalSizeMax(
    REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext,
    DWORD mshlflags, DWORD *pSize
) {
  CComPtr<IMarshal> marshal;
  RETURN_IF_FAILED(CoGetStandardMarshal(
      riid, GetInterfaceToBeMarshaled(riid), dwDestContext, pvDestContext,
      mshlflags, &marshal
  ));
  return marshal->GetMarshalSizeMax(
      riid, pv, dwDestContext, pvDestContext, mshlflags, pSize
  );
}

HRESULT STDMETHODCALLTYPE HostContainerFactory::MarshalInterface(
    IStream *pStm, REFIID riid, void *pv, DWORD dwDestContext,
    void *pvDestContext, DWORD mshlflags
) {
  CComPtr<IMarshal> marshal;
  RETURN_IF_FAILED(CoGetStandardMarshal(
      riid, GetInterfaceToBeMarshaled(riid), dwDestContext, pvDestContext,
      mshlflags, &marshal
  ));
  return marshal->MarshalInterface(
      pStm, riid, pv, dwDestContext, pvDestContext, mshlflags
  );
}

HRESULT STDMETHODCALLTYPE HostContainerFactory::UnmarshalInterface(
    IStream *pStm, REFIID riid, void **ppv
) {
  CComPtr<IMarshal> marshal;
  RETURN_IF_FAILED(CoGetStandardMarshal(
      riid, GetInterfaceToBeMarshaled(riid), MSHCTX_LOCAL, nullptr,
      MSHLFLAGS_NORMAL, &marshal
  ));
  return marshal->UnmarshalInterface(pStm, riid, ppv);
}

HRESULT STDMETHODCALLTYPE
HostContainerFactory::ReleaseMarshalData(IStream *pStm) {
  CComPtr<IMarshal> marshal;
  RETURN_IF_FAILED(CoGetStandardMarshal(
      IID_IUnknown, GetInterfaceToBeMarshaled(IID_IUnknown), MSHCTX_LOCAL,
      nullptr, MSHLFLAGS_NORMAL, &marshal
  ));
  return marshal->ReleaseMarshalData(pStm);
}

HRESULT STDMETHODCALLTYPE
HostContainerFactory::DisconnectObject(DWORD dwReserved) {
  CComPtr<IMarshal> marshal;
  RETURN_IF_FAILED(CoGetStandardMarshal(
      IID_IUnknown, GetInterfaceToBeMarshaled(IID_IUnknown), MSHCTX_LOCAL,
      nullptr, MSHLFLAGS_NORMAL, &marshal
  ));
  return marshal->DisconnectObject(dwReserved);
}
