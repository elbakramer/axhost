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

#include "command_line_parser.h"

#include <iostream>

#include <QMessageBox>

#include "help_dialog.h"

const std::map<std::string, int> AsDuration::m_mapping = {
    {"ms", 1},
    {"s", 1000},
    {"m", 60 * 1000},
    {"h", 60 * 60 * 1000},
};
const std::vector<std::string> AsDuration::m_units = {"ms", "s", "m", "h"};

std::string AsDuration::generate_description(
    const std::string &type_name, const std::string &unit_name, Options opts
) {
  std::stringstream out;
  out << type_name;
  if (opts & UNIT_REQUIRED) {
    out << '(' << unit_name << ')';
  } else {
    out << '[' << unit_name << ']';
  }
  return out.str();
}

AsDuration::AsDuration()
    : CLI::AsNumberWithUnit(m_mapping) {
  std::string type_name = "<duration>";
  QStringList units;
  for (const auto &unit : m_units) {
    units << QString::fromStdString(unit);
  }
  Options opts = Options::DEFAULT;
  std::string unit_name = units.join("|").toStdString();
  description(generate_description(type_name, unit_name, opts));
}

CommandLineParser::CommandLineParser()
    : m_appName(QCoreApplication::applicationName()),
      m_appVersion(QCoreApplication::applicationVersion()),
      m_appDescription("A minimal host process for COM/ActiveX controls"),
      m_app(m_appDescription.toStdString(), m_appName.toStdString()) {
  m_app.allow_windows_style_options();

  AsDuration as_duration;

  std::string as_duration_desc = as_duration.get_description();
  as_duration.description("");

  m_app.set_help_flag("--help", "Print this help message and exit.");
  m_app.set_version_flag(
      "--version", m_appVersion.toStdString(), "Print app version and exit."
  );

  m_app
      .add_option("--clsid", m_result.specs, R"(
            Register a COM class (repeatable).
            Format: c[/a[/cc[/cr[/r]]]] where
            - c  = CLSID
            - a  = alias CLSID
            - cc = CLSCTX for Create (DWORD)
            - cr = CLSCTX for Register (DWORD)
            - r  = REGCLS (DWORD)
            Use decimal or 0x-prefixed hex for DWORD values.
            Omit a field (\"//\") to keep its default:
            - a=c,
            - cc=CLSCTX_INPROC_SERVER,
            - cr=CLSCTX_LOCAL_SERVER,
            - r=REGCLS_SINGLEUSE|REGCLS_MULTI_SEPARATE|REGCLS_SUSPENDED.
            Examples:
            - {CLSID}/{ALIAS}
            - {CLSID}/{ALIAS}/0x1/0x4/0x2
            - {CLSID}//0x1//0x2
            Invalid values are currently being ignored.
        )")
      ->type_name("<item>")
      ->expected(-1);
  m_app
      .add_option(
          "--timeout", m_result.timeout,
          "Exit automatically if no COM activity occurs for the given time "
          "(in milliseconds if no unit is specified, default=60000)."
      )
      ->transform(as_duration)
      ->type_name(as_duration_desc);
  m_app
      .add_option(
          "--ready-event", m_result.readyEvent,
          "Fire named event when classes are available "
          "(default=Local\\AxHost_Ready_{PID})."
      )
      ->type_name("<name>");
}

ParsedResult CommandLineParser::parse(int argc, char *argv[]) {
  try {
    m_app.parse(argc, argv);
  } catch (const CLI::CallForHelp &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    HelpDialog help;
    help.setHelpText(QString::fromStdString(m_app.help()));
    help.exec();
  } catch (const CLI::CallForAllHelp &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    QMessageBox::information(
        nullptr, QCoreApplication::applicationName(),
        QString::fromStdString(m_app.help("", CLI::AppFormatMode::All))
    );
  } catch (const CLI::CallForVersion &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    QMessageBox::about(
        nullptr, QCoreApplication::applicationName(),
        QString::fromStdString(m_app.version())
    );
  } catch (const CLI::ParseError &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    QMessageBox::critical(
        nullptr, QCoreApplication::applicationName(),
        QString::fromStdString(e.what())
    );
  }
  return m_result;
}
