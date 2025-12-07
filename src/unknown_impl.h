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

#ifndef UNKNOWN_IMPL_H
#define UNKNOWN_IMPL_H

#include <atomic>

#include <windows.h>

template <typename Derived> class CUnknownImpl : public Derived {
private:
  std::atomic<ULONG> m_ref{0};

protected:
  CUnknownImpl() = default;
  virtual ~CUnknownImpl() = default;

  CUnknownImpl(const CUnknownImpl &) = delete;
  CUnknownImpl &operator=(const CUnknownImpl &) = delete;
  CUnknownImpl(CUnknownImpl &&) = delete;
  CUnknownImpl &operator=(CUnknownImpl &&) = delete;

public:
  ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }

  ULONG STDMETHODCALLTYPE Release() override {
    ULONG n = --m_ref;
    if (n == 0)
      delete this;
    return n;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override {
    if (!ppv)
      return E_POINTER;
    *ppv = nullptr;
    if (riid == IID_IUnknown) {
      *ppv = static_cast<IUnknown *>(this);
    } else if (riid == __uuidof(Derived)) {
      *ppv = static_cast<Derived *>(this);
    } else {
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
  }
};

#endif // UNKNOWN_IMPL_H
