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

#include <QCoreApplication>
#include <QString>

#include <map>
#include <string>
#include <vector>

#include "class_spec.h"

struct ParsedResult {
  QList<ClassSpec> specs;
  int timeout = 60000;
  QString readyEvent;

  int code = 0;
  QString msg;
};

class AsDuration : public CLI::AsNumberWithUnit {
private:
  static const std::map<std::string, int> m_mapping;
  static const std::vector<std::string> m_units;

private:
  static std::string generate_description(
      const std::string &type_name, const std::string &unit_name, Options opts
  );

public:
  AsDuration();
};

class CommandLineParser {
  Q_DECLARE_TR_FUNCTIONS(CommandLineParser)

private:
  QString m_appName;
  QString m_appVersion;
  QString m_appDescription;

  CLI::App m_app;
  ParsedResult m_result;

public:
  CommandLineParser();
  ParsedResult parse(int argc, char *argv[]);
};

#endif // COMMAND_LINE_PARSER_H
