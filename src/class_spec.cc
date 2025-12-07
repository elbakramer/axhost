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

#include "class_spec.h"

#include <string>

#include <QCoreApplication>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QUuid>

HRESULT CLSIDFromQString(const QString &s, QUuid &u) {
  return CLSIDFromString(
      reinterpret_cast<LPCOLESTR>(s.utf16()), reinterpret_cast<LPCLSID>(&u)
  );
}

ClassSpec ClassSpec::fromString(const QString &item) {
  ClassSpec spec;
  QStringList parts = item.split("/");
  if (parts.size() > 0 && !parts[0].isEmpty()) {
    spec.clsid_input = parts[0].trimmed();
    HRESULT hr = CLSIDFromQString(spec.clsid_input, spec.clsid);
    if (FAILED(hr)) {
      QString text = QString(R"(
Error: CLSID Parsing Failed

Invalid CLSID: '%1'
)")
                         .arg(spec.clsid_input)
                         .trimmed();
      QMessageBox::warning(nullptr, QCoreApplication::applicationName(), text);
    }
  }
  if (parts.size() > 1 && !parts[1].isEmpty()) {
    spec.alias_input = parts[1].trimmed();
    HRESULT hr = CLSIDFromQString(spec.alias_input, spec.alias);
    if (FAILED(hr)) {
      QString text = QString(R"(
Error: CLSID Parsing Failed

Invalid CLSID: '%1'
)")
                         .arg(spec.alias_input)
                         .trimmed();
      QMessageBox::warning(nullptr, QCoreApplication::applicationName(), text);
    }
  }
  if (parts.size() > 2 && !parts[2].isEmpty()) {
    bool ok = false;
    DWORD value = parts[2].toUInt(&ok, 0);
    if (ok) {
      spec.clsctx_create = value;
    }
  }
  if (parts.size() > 3 && !parts[3].isEmpty()) {
    bool ok = false;
    DWORD value = parts[3].toUInt(&ok, 0);
    if (ok) {
      spec.clsctx_register = value;
    }
  }
  if (parts.size() > 4 && !parts[4].isEmpty()) {
    bool ok = false;
    DWORD value = parts[4].toUInt(&ok, 0);
    if (ok) {
      spec.regcls = value;
      spec.regcls_explicit = true;
    }
  }
  return spec;
}

void ClassSpec::sanitize(DWORD regcls) {
  if (alias_input.isEmpty()) {
    alias_input = clsid_input;
  }
  if (alias.isNull()) {
    alias = clsid;
  }
  if (clsctx_create == 0) {
    clsctx_create = CLSCTX_INPROC_SERVER;
  }
  if (clsctx_register == 0) {
    clsctx_register = CLSCTX_LOCAL_SERVER;
  }
  if (!regcls_explicit) {
    if (regcls == 0) {
      regcls = REGCLS_SINGLEUSE | REGCLS_MULTI_SEPARATE;
    }
    this->regcls = regcls;
  }
}

std::istream &operator>>(std::istream &is, ClassSpec &out) {
  std::string token;
  if (!(is >> token))
    return is;
  out = ClassSpec::fromString(QString::fromStdString(token));
  return is;
}

std::ostream &operator<<(std::ostream &os, const ClassSpec &s) {
  os << s.clsid_input.toStdString() << "/" << s.alias_input.toStdString() << "/"
     << s.clsctx_create << "/" << s.clsctx_register << "/" << s.regcls;
  return os;
}
