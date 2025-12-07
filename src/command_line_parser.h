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

#ifndef COMMAND_LINE_PARSER_H
#define COMMAND_LINE_PARSER_H

#include <CLI/CLI.hpp>

#include <QList>
#include <QString>
#include <QUuid>

#include <windows.h>

#include "class_spec.h"

struct ParsedResult {
  QUuid classId;
  QString processId;
  bool embedding;
  bool automation;

  QList<ClassSpec> specs;
  int timeout = 60000;
  QString readyEvent;
  DWORD regcls = 0;

  bool enableLogging = false;
  QString logLevel;
  QString logFile;
  QString logDir;

  QString registerClassId;
  QString registerAppId;
  QString unregisterClassId;

  bool registerLogging = false;
  QString registerLogLevel;
  QString registerLogDir;

  int code = 0;
  QString msg;
};

class CommandLineParser {
private:
  QString m_appName;
  QString m_appVersion;
  QString m_appDescription;

  CLI::App m_app;
  ParsedResult m_result;

public:
  CommandLineParser();

  ParsedResult tryParse(int argc, char *argv[]);
  ParsedResult parse(int argc, char *argv[]);
};

#endif // COMMAND_LINE_PARSER_H
