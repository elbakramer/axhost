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

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QString>

#include "config.h"

#include "com_initialize_context.h"
#include "command_line_parser.h"
#include "container_factory_registry.h"
#include "exit_condition_checker.h"
#include "utils.h"

int main(int argc, char *argv[]) {
  ComInitializeContext coinit;
  QApplication app(argc, argv);

  app.setApplicationDisplayName(CMAKE_PROJECT_NAME);
  app.setApplicationVersion(CMAKE_PROJECT_VERSION);

  if (!coinit.IsInitialized()) {
    DWORD err = GetLastError();
    QString msg = GetLastErrorMessage(err);
    QString text = QString(R"(
Error: COM Initialization Failed

CoInitializeEx failed.

Error message:
%1)")
                       .arg(msg)
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    return err;
  }

  CommandLineParser parser;
  ParsedResult parsed = parser.parse(argc, argv);

  if (parsed.code != 0 || parsed.specs.empty()) {
    return parsed.code;
  }

  ExitConditionChecker *checker =
      new ExitConditionChecker(parsed.timeout, &app);
  HostContainerFactoryRegistry registry(parsed.specs);

  return app.exec();
}
