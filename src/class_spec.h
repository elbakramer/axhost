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

class ClassSpec {
public:
  QString clsid;
  QString alias;
  DWORD clsctx_create = CLSCTX_INPROC_SERVER;
  DWORD clsctx_register = CLSCTX_LOCAL_SERVER;
  DWORD regcls = REGCLS_SINGLEUSE | REGCLS_MULTI_SEPARATE | REGCLS_SUSPENDED;

public:
  static ClassSpec fromString(const QString &item);
};

std::istream &operator>>(std::istream &is, ClassSpec &out);
std::ostream &operator<<(std::ostream &os, const ClassSpec &s);

#endif // CLASS_SPEC_H
