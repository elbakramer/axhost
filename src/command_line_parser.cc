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

#include "as_classid.h"
#include "as_duration.h"
#include "command_line_formatter.h"
#include "help_dialog.h"

CommandLineParser::CommandLineParser()
    : m_appName(QCoreApplication::applicationName()),
      m_appVersion(QCoreApplication::applicationVersion()),
      m_appDescription("A minimal host process for COM/ActiveX controls"),
      m_app(m_appDescription.toStdString(), m_appName.toStdString()) {
  AsClassId as_classid;
  AsDuration as_duration;

  std::string ad_classid_desc = as_classid.get_description();
  std::string as_duration_desc = as_duration.get_description();

  as_classid.description("");
  as_duration.description("");

  m_app.formatter(std::make_shared<Formatter>());

  m_app.allow_windows_style_options();
  m_app.allow_non_standard_option_names();

  m_app.set_help_flag("--help", "Print this help message and exit.");
  m_app.set_version_flag(
      "--version", m_appVersion.toStdString(), "Print app version and exit."
  );

  m_app.add_flag_callback(
           "-h,-?,-Help", []() { throw CLI::CallForHelp{}; }
  )->group("");
  m_app.add_flag_callback(
           "-V,-Version", []() { throw CLI::CallForVersion{}; }
  )->group("");

  auto surrogate = m_app.add_option_group(
      "Surrogate",
      "Options used by the COM Service Control Manager (SCM) when launching "
      "this process as a surrogate server. These parameters are "
      "system-supplied and not intended for direct user invocation."
  );

  surrogate->allow_windows_style_options();
  surrogate->allow_non_standard_option_names();

  surrogate
      ->add_option(
          "CLSID", m_result.classId,
          "CLSID of the COM class to be activated by the surrogate. Supplied "
          "automatically by the COM runtime."
      )
      ->type_name("")
      ->transform(as_classid);

  surrogate->add_flag(
      "-Embedding", m_result.embedding,
      "Indicates that the process was launched by SCM as a COM server"
  );

  surrogate->add_flag(
      "-Automation", m_result.automation,
      "Indicates activation via OLE Automation. (reserved for "
      "compatibility; normally unused)"
  );

  surrogate
      ->add_option(
          "-Processid", m_result.processId,
          "Process identifier supplied by SCM during surrogate activation. "
          "(reserved for compatibility; normally unused)"
      )
      ->type_name("<id>");

  auto standalone = m_app.add_option_group(
      "Standalone",
      "Options used when running this process as a standalone COM local "
      "server. These parameters enable manual execution for development, "
      "debugging, or controlled hosting scenarios."
  );

  standalone->allow_windows_style_options();
  standalone->allow_non_standard_option_names();

  standalone
      ->add_option(
          "--clsid", m_result.specs, R"(Register a COM class (repeatable).
            Format: c[/a[/cc[/cr[/r]]]] where
            - c  = CLSID
            - a  = alias CLSID
            - cc = CLSCTX for Create (DWORD)
            - cr = CLSCTX for Register (DWORD)
            - r  = REGCLS (DWORD)
            Use decimal or 0x-prefixed hex for DWORD values.
            Omit a field ("//") to keep its default:
            - a=c,
            - cc=CLSCTX_INPROC_SERVER,
            - cr=CLSCTX_LOCAL_SERVER,
            - r=REGCLS_MULTI_SEPARATE|REGCLS_SINGLEUSE (or |REGCLS_MULTIPLEUSE with --multiple-use).
            Notes:
            - REGCLS_SUSPENDED is added automatically during registration when applicable.
            - Explicitly specifying 'r' bypasses all defaults and global flags; your value is used exactly as provided.
            - Multiple-use mode avoids self-instantiation by re-registering the original InProc when available. Explicit alias recommended.
            Examples:
            - {CLSID}/{ALIAS}
            - {CLSID}/{ALIAS}/0x1/0x4/0x2
            - {CLSID}//0x1//0x2)"
      )
      ->type_name("<item>")
      ->expected(-1);
  standalone->add_option("-ClassId", m_result.specs)
      ->type_name("<item>")
      ->expected(-1)
      ->group("");

  standalone
      ->add_option(
          "--timeout", m_result.timeout,
          "Exit automatically if no COM activation occurs for the given time "
          "(in milliseconds if no unit is specified, default=60000)."
      )
      ->type_name(as_duration_desc)
      ->transform(as_duration);
  standalone->add_option("-Timeout", m_result.timeout)
      ->type_name(as_duration_desc)
      ->transform(as_duration)
      ->group("");

  standalone
      ->add_option(
          "--ready-event", m_result.readyEvent,
          "Signal the specified named event after all class factories have "
          "been registered. (default=Local\\AxHost_Ready_{PID})."
      )
      ->type_name("<name>");
  standalone->add_option("-ReadyEvent", m_result.readyEvent)
      ->type_name("<name>")
      ->group("");

  auto single_use_callback = [&]() { m_result.regcls = REGCLS_SINGLEUSE; };
  auto multiple_use_callback = [&]() { m_result.regcls = REGCLS_MULTIPLEUSE; };

  standalone->add_flag_callback(
      "--single-use", single_use_callback,
      "Use single-use mode: class factory serves one instance then "
      "unregisters (REGCLS_SINGLEUSE). This is the default behavior."
  );
  standalone->add_flag_callback("-SingleUse", single_use_callback)->group("");

  standalone->add_flag_callback(
      "--multiple-use", multiple_use_callback,
      "Use multiple-use mode: class factory can serve multiple instances "
      "(REGCLS_MULTIPLEUSE). Process waits for timeout period after "
      "instances are released to allow subsequent connections."
  );
  standalone->add_flag_callback("-MultipleUse", multiple_use_callback)
      ->group("");

  standalone->add_flag(
      "--enable-logging", m_result.enableLogging,
      "Enable logging for this process."
  );
  standalone->add_flag("-EnableLogging", m_result.enableLogging)->group("");

  standalone
      ->add_option(
          "--log-level", m_result.logLevel,
          "Minimum logging level (trace, debug, info, warn, error)."
      )
      ->type_name("<level>");
  standalone->add_option("-LogLevel", m_result.logLevel)
      ->type_name("<level>")
      ->group("");

  standalone
      ->add_option(
          "--log-file", m_result.logFile,
          "Write logs to the specified file (implies --enable-logging)."
      )
      ->type_name("<file>");
  standalone->add_option("-LogFile", m_result.logFile)
      ->type_name("<file>")
      ->group("");

  standalone
      ->add_option(
          "--log-dir", m_result.logDir,
          "Directory for log output; filename is chosen automatically "
          "(implies --enable-logging)."
      )
      ->type_name("<dir>");
  standalone->add_option("-LogDirectory", m_result.logDir)
      ->type_name("<dir>")
      ->group("");

  auto registry = m_app.add_option_group(
      "Registry",
      "Options used to configure or remove registry entries required for "
      "surrogate activation. These operations modify system-wide COM "
      "registration and generally require administrative privileges."
  );

  registry->allow_windows_style_options();
  registry->allow_non_standard_option_names();

  auto register_function = [&](const QString &value) {
    m_result.registerClassId = value;
    if (m_result.registerAppId.isEmpty()) {
      m_result.registerAppId = m_result.registerClassId;
    }
  };

  registry
      ->add_option_function<QString>(
          "--register", register_function,
          "Register axhost as DllSurrogate for the specified CLSID."
      )
      ->type_name("<clsid>");
  registry->add_option_function<QString>("-Register", register_function)
      ->type_name("<clsid>")
      ->group("");

  registry
      ->add_option(
          "--register-appid", m_result.registerAppId,
          "Specify AppID for surrogate registration (defaults to CLSID if not "
          "specified)."
      )
      ->type_name("<appid>");
  registry->add_option("-RegisterAppId", m_result.registerAppId)
      ->type_name("<appid>")
      ->group("");

  registry->add_flag(
      "--register-logging", m_result.registerLogging,
      "Enable logging for the CLSID/AppID being registered (used by "
      "surrogate mode). Requires --register."
  );
  registry->add_flag("-RegisterLogging", m_result.registerLogging)->group("");

  registry
      ->add_option(
          "--register-log-level", m_result.registerLogLevel,
          "Default logging level for surrogate instances (trace, debug, info, "
          "warn, error). Requires --register."
      )
      ->type_name("<level>");
  registry->add_option("-RegisterLogLevel", m_result.registerLogLevel)
      ->type_name("<level>")
      ->group("");

  registry
      ->add_option(
          "--register-log-dir", m_result.registerLogDir,
          "Default log directory for surrogate instances. Log filenames are "
          "chosen automatically. Requires --register."
      )
      ->type_name("<dir>");
  registry->add_option("-RegisterLogDirectory", m_result.registerLogDir)
      ->type_name("<dir>")
      ->group("");

  registry
      ->add_option(
          "--unregister", m_result.unregisterClassId,
          "Unregister axhost as DllSurrogate for the specified CLSID."
      )
      ->type_name("<clsid>");
  registry->add_option("-Unregister", m_result.unregisterClassId)
      ->type_name("<clsid>")
      ->group("");
}

ParsedResult CommandLineParser::tryParse(int argc, char *argv[]) {
  try {
    argv = m_app.ensure_utf8(argv);
    m_result = ParsedResult{};
    m_app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    m_result.code = e.get_exit_code();
    m_result.msg = e.what();
  } catch (...) {
  }
  return m_result;
}

ParsedResult CommandLineParser::parse(int argc, char *argv[]) {
  try {
    argv = m_app.ensure_utf8(argv);
    m_result = ParsedResult{};
    m_app.parse(argc, argv);
  } catch (const CLI::CallForHelp &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    HelpDialog help;
    help.setHelpText(QString::fromStdString(m_app.help()));
    help.exec();
    std::exit(m_result.code);
  } catch (const CLI::CallForAllHelp &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    HelpDialog help;
    help.setHelpText(
        QString::fromStdString(m_app.help("", CLI::AppFormatMode::All))
    );
    help.exec();
    std::exit(m_result.code);
  } catch (const CLI::CallForVersion &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    QMessageBox::about(
        nullptr, QCoreApplication::applicationName(),
        QString::fromStdString(m_app.version())
    );
    std::exit(m_result.code);
  } catch (const CLI::ParseError &e) {
    m_result.code = m_app.exit(e);
    m_result.msg = e.what();
    QMessageBox::critical(
        nullptr, QCoreApplication::applicationName(),
        QString::fromStdString(e.what())
    );
    std::exit(m_result.code);
  }
  return m_result;
}
