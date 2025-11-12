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

#include "provide_class_info.h"

#include <string>

HostProvideClassInfo::HostProvideClassInfo(
    REFCLSID classId, IProvideClassInfo *underlying,
    IProvideClassInfo2 *underlying2
)
    : m_classId(classId),
      m_underlying(underlying),
      m_underlying2(underlying2) {}

HRESULT
HostProvideClassInfo::ReadTypeLibIdFromCLSID(REFCLSID rclsid, GUID *pLibid) {
  if (!pLibid)
    return E_POINTER;

  LPOLESTR clsidStr = nullptr;
  HRESULT hr = StringFromCLSID(rclsid, &clsidStr);
  if (FAILED(hr))
    return hr;

  std::wstring key = L"CLSID\\";
  key += clsidStr;
  key += L"\\TypeLib";
  CoTaskMemFree(clsidStr);

  HKEY hKey = nullptr;
  LONG rc = RegOpenKeyExW(HKEY_CLASSES_ROOT, key.c_str(), 0, KEY_READ, &hKey);
  if (rc != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(rc);

  wchar_t buf[64];
  DWORD sz = sizeof(buf);
  rc = RegQueryValueExW(hKey, nullptr, nullptr, nullptr, (LPBYTE)buf, &sz);
  RegCloseKey(hKey);
  if (rc != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(rc);

  return CLSIDFromString(buf, pLibid);
}

HRESULT
HostProvideClassInfo::FindLatestTypeLibVersion(
    REFGUID libid, USHORT *pMajor, USHORT *pMinor
) {
  if (!pMajor || !pMinor)
    return E_POINTER;
  *pMajor = *pMinor = 0;

  LPOLESTR libStr = nullptr;
  HRESULT hr = StringFromCLSID(libid, &libStr);
  if (FAILED(hr))
    return hr;

  std::wstring key = L"TypeLib\\";
  key += libStr;
  CoTaskMemFree(libStr);

  HKEY hKey = nullptr;
  LONG rc = RegOpenKeyExW(HKEY_CLASSES_ROOT, key.c_str(), 0, KEY_READ, &hKey);
  if (rc != ERROR_SUCCESS)
    return TYPE_E_LIBNOTREGISTERED;

  DWORD index = 0;
  wchar_t name[64];
  DWORD namelen = _countof(name);
  while ((rc = RegEnumKeyExW(
              hKey, index++, name, &namelen, nullptr, nullptr, nullptr, nullptr
          )) == ERROR_SUCCESS) {
    unsigned x = 0, y = 0;
    if (swscanf_s(name, L"%u.%u", &x, &y) == 2) {
      if (x > *pMajor || (x == *pMajor && y > *pMinor)) {
        *pMajor = (USHORT)x;
        *pMinor = (USHORT)y;
      }
    }
    namelen = _countof(name);
  }
  RegCloseKey(hKey);

  return (*pMajor || *pMinor) ? S_OK : TYPE_E_LIBNOTREGISTERED;
}

HRESULT
HostProvideClassInfo::GetTypeLibFromCLSID(REFCLSID rclsid, ITypeLib **ppTL) {
  if (!ppTL)
    return E_POINTER;
  *ppTL = nullptr;

  GUID libid{};
  USHORT maj = 0, min = 0;

  HRESULT hr = ReadTypeLibIdFromCLSID(rclsid, &libid);
  if (FAILED(hr))
    return hr;

  hr = FindLatestTypeLibVersion(libid, &maj, &min);
  if (FAILED(hr))
    return hr;

  return LoadRegTypeLib(libid, maj, min, LOCALE_USER_DEFAULT, ppTL);
}

HRESULT
HostProvideClassInfo::FindDefaultSourceIID(ITypeInfo *pCoClassTI, GUID *pOut) {
  if (!pCoClassTI || !pOut)
    return E_POINTER;

  *pOut = GUID_NULL;

  for (UINT i = 0;; ++i) {
    INT implFlags = 0;
    HRESULT hr = pCoClassTI->GetImplTypeFlags(i, &implFlags);

    if (FAILED(hr))
      break;

    const bool isDefault = !!(implFlags & IMPLTYPEFLAG_FDEFAULT);
    const bool isSource = !!(implFlags & IMPLTYPEFLAG_FSOURCE);

    if (!isDefault)
      continue;
    if (!isSource)
      continue;

    HREFTYPE href = 0;
    hr = pCoClassTI->GetRefTypeOfImplType(i, &href);
    if (FAILED(hr))
      continue;

    ITypeInfo *pTI;
    hr = pCoClassTI->GetRefTypeInfo(href, &pTI);
    if (FAILED(hr) || !pTI)
      continue;

    TYPEATTR *pTA = nullptr;
    hr = pTI->GetTypeAttr(&pTA);
    if (FAILED(hr) || !pTA) {
      pTI->Release();
      continue;
    }

    *pOut = pTA->guid;
    pTI->ReleaseTypeAttr(pTA);
    pTI->Release();
    return S_OK;
  }

  return TYPE_E_ELEMENTNOTFOUND;
}

ULONG STDMETHODCALLTYPE HostProvideClassInfo::AddRef() { return ++m_ref; }

ULONG STDMETHODCALLTYPE HostProvideClassInfo::Release() {
  ULONG n = --m_ref;
  if (n == 0)
    delete this;
  return n;
}

HRESULT STDMETHODCALLTYPE
HostProvideClassInfo::QueryInterface(REFIID riid, void **ppv) {
  if (!ppv)
    return E_POINTER;
  *ppv = nullptr;
  if (riid == IID_IUnknown) {
    *ppv = static_cast<IUnknown *>(this);
  } else if (riid == IID_IProvideClassInfo) {
    *ppv = static_cast<IProvideClassInfo *>(this);
  } else if (riid == IID_IProvideClassInfo2) {
    *ppv = static_cast<IProvideClassInfo2 *>(this);
  } else {
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE HostProvideClassInfo::GetClassInfo(ITypeInfo **ppTI) {
  if (m_underlying) {
    HRESULT hr = m_underlying->GetClassInfo(ppTI);
    if (SUCCEEDED(hr))
      return hr;
  }
  if (!ppTI)
    return E_POINTER;
  CComPtr<ITypeLib> pTL;
  HRESULT hr = GetTypeLibFromCLSID(m_classId, &pTL);
  if (FAILED(hr))
    return hr;
  if (!pTL)
    return E_UNEXPECTED;
  return pTL->GetTypeInfoOfGuid(m_classId, ppTI);
}

HRESULT STDMETHODCALLTYPE
HostProvideClassInfo::GetGUID(DWORD dwGuidKind, GUID *pGUID) {
  if (m_underlying2) {
    HRESULT hr = m_underlying2->GetGUID(dwGuidKind, pGUID);
    if (SUCCEEDED(hr))
      return hr;
  }
  if (!pGUID)
    return E_POINTER;
  if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    return E_INVALIDARG;
  CComPtr<ITypeInfo> pTI;
  HRESULT hr = GetClassInfo(&pTI);
  if (FAILED(hr))
    return hr;
  if (!pTI)
    return E_UNEXPECTED;
  return FindDefaultSourceIID(pTI, pGUID);
}
