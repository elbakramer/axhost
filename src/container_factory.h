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

#include <windows.h>

#include "class_spec.h"
#include "unknown_impl.h"

class HostContainerFactory : public CUnknownImpl<IClassFactory> {
private:
  ClassSpec m_spec;

public:
  HostContainerFactory(const ClassSpec &spec);

  HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;
  HRESULT STDMETHODCALLTYPE
  CreateInstance(IUnknown *outer, REFIID riid, void **ppv) override;
};

#endif // CONTAINER_FACTORY_H
