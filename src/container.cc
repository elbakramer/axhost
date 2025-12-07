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

#include "container.h"

#include <windows.h>

#include <atlcomcli.h>
#include <ocidl.h>
#include <oleidl.h>

#include <QAxWidget>
#include <QCoreApplication>
#include <QMessageBox>
#include <QSharedPointer>
#include <QString>
#include <QUuid>

#include "connection_point_container.h"
#include "external_connection.h"
#include "provide_class_info.h"
#include "surrogate_runtime.h"
#include "utils.h"

HostContainer::HostContainer(REFCLSID clsid, DWORD clsctx)
    : m_classId(clsid),
      m_classContext(clsctx),
      m_control(new QAxWidget()) {
  if (auto *runtime = HostSurrogateRuntime::instance()) {
    runtime->AddServerReference();
  }
  m_control->setClassContext(m_classContext);

  if (m_control->setControl(m_classId.toString()) && !m_control->isNull()) {
    CComPtr<IProvideClassInfo> underlyingPCI;
    CComPtr<IProvideClassInfo2> underlyingPCI2;
    m_control->queryInterface(IID_IProvideClassInfo, (void **)&underlyingPCI);
    m_control->queryInterface(IID_IProvideClassInfo2, (void **)&underlyingPCI2);
    m_provideClassInfo =
        new HostProvideClassInfo(m_classId, underlyingPCI, underlyingPCI2);

    CComPtr<IConnectionPointContainer> underlyingCPC;
    m_control->queryInterface(
        IID_IConnectionPointContainer, (void **)&underlyingCPC
    );
    m_connectionPointContainer =
        new HostConnectionPointContainer(underlyingCPC);

    CComPtr<IExternalConnection> underlyingEC;
    m_control->queryInterface(IID_IExternalConnection, (void **)&underlyingEC);
    m_externalConnection = new HostExternalConnection(underlyingEC);
  } else {
    DWORD err = GetLastError();
    QString classId = m_classId.toString();
    QString classContext = QString::number(m_classContext, 16);
    QString errorMessage = GetLastErrorMessage(err);
    QString text = QString(R"(
Error: Control Loading Failed

Failed to load control.

CLSID: '%1'
CLSCTX: 0x%2

Error message:
%3)")
                       .arg(classId)
                       .arg(classContext)
                       .arg(errorMessage)
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
  }
}

HostContainer::~HostContainer() {
  if (auto *runtime = HostSurrogateRuntime::instance()) {
    runtime->ReleaseServerReference();
  }
}

bool HostContainer::IsInitialized() {
  return m_control && !m_control->isNull();
}

ULONG STDMETHODCALLTYPE HostContainer::AddRef() { return ++m_ref; }

ULONG STDMETHODCALLTYPE HostContainer::Release() {
  ULONG n = --m_ref;
  if (n <= 0) {
    delete this;
  }
  return n;
}

HRESULT STDMETHODCALLTYPE
HostContainer::QueryInterface(REFIID riid, void **ppv) {
  if (!ppv)
    return E_POINTER;
  *ppv = nullptr;
  if (riid == IID_IUnknown) {
    *ppv = static_cast<IProvideClassInfo2 *>(this);
  } else if (riid == IID_IProvideClassInfo2) {
    *ppv = static_cast<IProvideClassInfo2 *>(this);
  } else if (riid == IID_IProvideClassInfo) {
    *ppv = static_cast<IProvideClassInfo *>(this);
  } else if (riid == IID_IConnectionPointContainer) {
    *ppv = static_cast<IConnectionPointContainer *>(this);
  } else if (riid == IID_IExternalConnection) {
    *ppv = static_cast<IExternalConnection *>(this);
  } else {
    if (!m_control || m_control->isNull())
      return E_NOINTERFACE;
    return m_control->queryInterface(riid, ppv);
  }
  AddRef();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE HostContainer::GetClassInfo(ITypeInfo **ppTI) {
  return m_provideClassInfo->GetClassInfo(ppTI);
}

HRESULT STDMETHODCALLTYPE
HostContainer::GetGUID(DWORD dwGuidKind, GUID *pGUID) {
  return m_provideClassInfo->GetGUID(dwGuidKind, pGUID);
}

HRESULT STDMETHODCALLTYPE
HostContainer::EnumConnectionPoints(IEnumConnectionPoints **ppEnum) {
  return m_connectionPointContainer->EnumConnectionPoints(ppEnum);
}

HRESULT STDMETHODCALLTYPE
HostContainer::FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP) {
  return m_connectionPointContainer->FindConnectionPoint(riid, ppCP);
}

DWORD STDMETHODCALLTYPE
HostContainer::AddConnection(DWORD extconn, DWORD reserved) {
  return m_externalConnection->AddConnection(extconn, reserved);
}

DWORD STDMETHODCALLTYPE HostContainer::ReleaseConnection(
    DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses
) {
  return m_externalConnection->ReleaseConnection(
      extconn, reserved, fLastReleaseCloses
  );
}
