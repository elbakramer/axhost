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
#include <QGuiApplication>
#include <QMessageBox>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

#include "config.h"

#include "com_initialize_context.h"
#include "command_line.h"
#include "command_line_parser.h"
#include "logging.h"
#include "registry_helper.h"
#include "surrogate_runtime.h"
#include "utils.h"

#include <wil/resource.h>
#include <wil/result.h>

#include "spdlog/spdlog.h"

void SetApplicationInformation() {
  QApplication::setApplicationName(CMAKE_PROJECT_NAME);
  QApplication::setApplicationDisplayName(CMAKE_PROJECT_NAME);
  QApplication::setApplicationVersion(CMAKE_PROJECT_VERSION);
}

void SetPreferredLanguages() {
  QStringList languages = {"en-US"};
  QChar separator = QChar(u'\0');
  QString languagesJoined = languages.join(separator);
  languagesJoined.append(separator);
  PCZZWSTR buffer = reinterpret_cast<PCZZWSTR>(languagesJoined.utf16());
  THROW_LAST_ERROR_IF(
      !SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, buffer, nullptr) &&
      IsDebuggerPresent()
  );
}

wil::unique_couninitialize_call
InitializeCom(DWORD dwCoInit = COINIT_APARTMENTTHREADED) {
  THROW_IF_FAILED_MSG(
      ::CoInitializeEx(nullptr, dwCoInit), "CoInitializeEx failed."
  );
  wil::unique_couninitialize_call cleanup;
  return cleanup;
}

void InitializeComSecurity() {
  THROW_IF_FAILED_MSG(
      ::CoInitializeSecurity(
          nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_APPID, nullptr
      ),
      "CoInitializeSecurity failed."
  );
}

int main(int argc, char *argv[]) {
  SetApplicationInformation();

  CommandLineParser parser;
  ParsedResult parsed;

  parsed = parser.tryParse(argc, argv);

  if (!parsed.classId.isNull() && parsed.embedding) {
    InitializeLoggingSurrogate(parsed.classId.toString());
  } else {
    InitializeLoggingStandalone(parsed);
  }

  spdlog::info("Command line: {}", CreateCommandLine(argc, argv).toStdString());

  SetPreferredLanguages();

  auto UninitializeCom = InitializeCom();
  InitializeComSecurity();

  QApplication app(argc, argv);
  QScopedPointer<HostSurrogateRuntime> runtime;

  parsed = parser.parse(argc, argv);

  if (!parsed.registerAppId.isEmpty()) {
    if (!parsed.registerClassId.isEmpty()) {
      THROW_IF_FAILED_MSG(
          RegisterSurrogate(parsed.registerClassId, parsed.registerAppId),
          "RegisterSurrogate failed."
      );
    }
    LoggingSettings settings;
    settings.enabled = parsed.registerLogging;
    settings.level = parsed.registerLogLevel;
    settings.directory = parsed.registerLogDir;
    THROW_IF_FAILED_MSG(
        WriteLoggingSettings(parsed.registerAppId, settings),
        "WriteLoggingSettings failed."
    );
    return 0;
  }

  if (!parsed.unregisterClassId.isEmpty()) {
    THROW_IF_FAILED_MSG(
        UnregisterSurrogate(parsed.unregisterClassId),
        "UnregisterSurrogate failed."
    );
    return 0;
  }

  if (!parsed.classId.isNull() && parsed.embedding) {
    runtime.reset(new RunAsSurrogate(parsed.classId, parsed.embedding));
  } else if (!parsed.specs.isEmpty()) {
    runtime.reset(
        new RunAsStandalone(parsed.specs, parsed.readyEvent, parsed.regcls)
    );
  } else {
    return 0;
  }

  return app.exec();
}
