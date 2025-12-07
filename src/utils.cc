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

#include "utils.h"

#include <wil/resource.h>

#include <QCoreApplication>

QString GetLastErrorMessage(DWORD err) {
  wil::unique_hlocal_string buf;
  DWORD len = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(buf.put()), 0, nullptr
  );
  QString msg;
  if (len && buf) {
    msg = QString::fromWCharArray(buf.get(), len).trimmed();
  }
  return msg;
}

BOOL ExitApplicationLater(int retcode) {
  QCoreApplication *app = QCoreApplication::instance();
  if (app) {
    return QMetaObject::invokeMethod(
        app, "exit", Qt::QueuedConnection, Q_ARG(int, retcode)
    );
  }
  return false;
}
