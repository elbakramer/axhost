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

#include <windows.h>

#include <QCoreApplication>

#include "class_spec.h"
#include "container.h"
#include "globals.h"
#include "unknown_impl.h"

HostContainerFactory::HostContainerFactory(const ClassSpec &spec)
    : m_spec(spec) {}

HRESULT STDMETHODCALLTYPE HostContainerFactory::LockServer(BOOL fLock) {
  if (fLock) {
    CoAddRefServerProcess();
    g_locks.ref();
  } else {
    bool shouldExit = CoReleaseServerProcess() == 0;
    bool shouldTestExit = !g_locks.deref();
    if (shouldExit) {
      QCoreApplication::exit();
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
  CComPtr<HostContainer> container = new HostContainer(m_spec);
  if (!container)
    return E_OUTOFMEMORY;
  if (!container->IsInitialized()) {
    return CLASS_E_CLASSNOTAVAILABLE;
  }
  return container->QueryInterface(riid, ppv);
}
