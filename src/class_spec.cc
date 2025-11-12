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

#include <QStringList>

ClassSpec ClassSpec::fromString(const QString &item) {
  ClassSpec spec;
  QStringList parts = item.split("/");
  if (parts.size() > 0 && !parts[0].isEmpty()) {
    spec.clsid = parts[0].trimmed();
  }
  if (parts.size() > 1 && !parts[1].isEmpty()) {
    spec.alias = parts[1].trimmed();
  } else {
    spec.alias = spec.clsid;
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
    }
  }
  return spec;
}

std::istream &operator>>(std::istream &is, ClassSpec &out) {
  std::string token;
  if (!(is >> token))
    return is;
  out = ClassSpec::fromString(QString::fromStdString(token));
  return is;
}

std::ostream &operator<<(std::ostream &os, const ClassSpec &s) {
  os << s.clsid.toStdString() << "/" << s.alias.toStdString() << "/"
     << s.clsctx_create << "/" << s.clsctx_register << "/" << s.regcls;
  return os;
}
