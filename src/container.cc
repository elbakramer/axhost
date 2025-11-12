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

#include "class_spec.h"
#include "connection_point_container.h"
#include "external_connection.h"
#include "globals.h"
#include "provide_class_info.h"
#include "utils.h"

HostContainer::HostContainer(const ClassSpec &spec)
    : m_spec(spec),
      m_control(new QAxWidget()) {
  HRESULT conv = CLSIDFromString(spec.clsid.toStdWString().c_str(), &m_classId);

  if (FAILED(conv)) {
    QString text = QString(R"(
Error: CLSID Parsing Failed

Invalid CLSID: '%1'
)")
                       .arg(spec.clsid)
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    return;
  }

  m_control->setClassContext(spec.clsctx_create);

  if (m_control->setControl(spec.clsid) && !m_control->isNull()) {
    CoAddRefServerProcess();
    g_locks.ref();
    g_creations.ref();

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
    QString msg = GetLastErrorMessage(err);
    QString text = QString(R"(
Error: Control Loading Failed

Failed to load control.

CLSID: '%1'
CLSCTX: 0x%2

Error message:
%3)")
                       .arg(spec.clsid)
                       .arg(QString::number(spec.clsctx_create, 16))
                       .arg(msg)
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
  }
}

HostContainer::~HostContainer() {
  bool shouldExit = CoReleaseServerProcess() == 0;
  bool shouldTestExit = !g_locks.deref();
  if (shouldExit) {
    QCoreApplication::exit();
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
