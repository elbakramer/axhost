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

#ifndef CLASS_SPEC_H
#define CLASS_SPEC_H

#include <iostream>

#include <windows.h>

#include <QString>
#include <QUuid>

class ClassSpec {
public:
  QUuid clsid;
  QUuid alias;
  QString clsid_input;
  QString alias_input;
  DWORD clsctx_create = 0;
  DWORD clsctx_register = 0;
  DWORD regcls = 0;
  bool regcls_explicit = false;
  HRESULT result = 0;
  DWORD error = 0;

public:
  void sanitize(DWORD regcls = 0);

public:
  static ClassSpec fromString(const QString &item);
};

std::istream &operator>>(std::istream &is, ClassSpec &out);
std::ostream &operator<<(std::ostream &os, const ClassSpec &s);

#endif // CLASS_SPEC_H
