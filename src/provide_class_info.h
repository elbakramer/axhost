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

#ifndef PROVIDE_CLASS_INFO_H
#define PROVIDE_CLASS_INFO_H

#include <atomic>

#include <atlcomcli.h>

class HostProvideClassInfo : public IProvideClassInfo2 {
private:
  std::atomic<ULONG> m_ref{0};

  CLSID m_classId;

  CComPtr<IProvideClassInfo> m_underlying;
  CComPtr<IProvideClassInfo2> m_underlying2;

private:
  static HRESULT ReadTypeLibIdFromCLSID(REFCLSID rclsid, GUID *pLibid);
  static HRESULT
  FindLatestTypeLibVersion(REFGUID libid, USHORT *pMajor, USHORT *pMinor);
  static HRESULT GetTypeLibFromCLSID(REFCLSID rclsid, ITypeLib **ppTL);
  static HRESULT FindDefaultSourceIID(ITypeInfo *pCoClassTI, GUID *pOut);

public:
  HostProvideClassInfo(
      REFCLSID classId, IProvideClassInfo *underlying,
      IProvideClassInfo2 *underlying2
  );

public:
  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override;

public:
  HRESULT STDMETHODCALLTYPE GetClassInfo(ITypeInfo **ppTI) override;
  HRESULT STDMETHODCALLTYPE GetGUID(DWORD dwGuidKind, GUID *pGUID) override;
};

#endif // PROVIDE_CLASS_INFO_H
