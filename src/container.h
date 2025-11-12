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

#ifndef CONTAINER_H
#define CONTAINER_H

#include <atomic>

#include <windows.h>

#include <atlcomcli.h>
#include <ocidl.h>
#include <oleidl.h>

#include <QAxWidget>
#include <QSharedPointer>

#include "class_spec.h"
#include "connection_point_container.h"
#include "external_connection.h"
#include "provide_class_info.h"

class HostContainer : public IProvideClassInfo2,
                      public IConnectionPointContainer,
                      public IExternalConnection {
private:
  std::atomic<ULONG> m_ref{0};

  ClassSpec m_spec;

  CLSID m_classId;
  QSharedPointer<QAxWidget> m_control;

  CComPtr<HostProvideClassInfo> m_provideClassInfo;
  CComPtr<HostConnectionPointContainer> m_connectionPointContainer;
  CComPtr<HostExternalConnection> m_externalConnection;

public:
  HostContainer(const ClassSpec &spec);
  ~HostContainer();

  bool IsInitialized();

  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override;

  HRESULT STDMETHODCALLTYPE GetClassInfo(ITypeInfo **ppTI) override;
  HRESULT STDMETHODCALLTYPE GetGUID(DWORD dwGuidKind, GUID *pGUID) override;

  HRESULT STDMETHODCALLTYPE
  EnumConnectionPoints(IEnumConnectionPoints **ppEnum) override;
  HRESULT STDMETHODCALLTYPE
  FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP) override;

  DWORD STDMETHODCALLTYPE AddConnection(DWORD extconn, DWORD reserved) override;
  DWORD STDMETHODCALLTYPE ReleaseConnection(
      DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses
  ) override;
};

#endif // CONTAINER_H
