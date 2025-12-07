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

#include "logging.h"

#include <exception>
#include <memory>

#include <windows.h>

#include <wil/resource.h>
#include <wil/result.h>

#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QString>
#include <QtLogging>

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "command_line_parser.h"
#include "registry_helper.h"

terminate_handler OriginalTerminateHandler;
LPTOP_LEVEL_EXCEPTION_FILTER OriginalExceptionFilter;
QtMessageHandler OriginalMessageHandler;

QString GetLoggerName() {
  QString name = QCoreApplication::applicationName();
  if (name.isEmpty())
    name = "axhost";
  return name;
}

QString GetLogDirectory() {
  QString name = GetLoggerName();
  QString tempDir =
      QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  QString logDir = QDir::cleanPath(tempDir + "/" + name);
  return logDir;
}

QString GetLogFilename(const QString &directory = QString()) {
  QString name = GetLoggerName();
  DWORD pid = GetCurrentProcessId();

  QDir dir;

  if (!directory.isEmpty()) {
    dir = QDir(directory);
  } else {
    dir = QDir(GetLogDirectory());
  }

  if (!dir.exists()) {
    dir.mkpath(".");
  }

  QString filename = QString("%1-%2.log").arg(name).arg(pid);
  QString filepath = dir.filePath(filename);
  return filepath;
}

spdlog::level::level_enum ParseLogLevel(
    const QString &level,
    spdlog::level::level_enum defaultLevel = spdlog::level::info
) {
  QString lower = level.toLower();
  if (lower == "trace")
    return spdlog::level::trace;
  if (lower == "debug")
    return spdlog::level::debug;
  if (lower == "info")
    return spdlog::level::info;
  if (lower == "warn" || lower == "warning")
    return spdlog::level::warn;
  if (lower == "error" || lower == "err")
    return spdlog::level::err;
  return defaultLevel;
}

std::shared_ptr<spdlog::logger> CreateDefaultLogger(const QString &filepath) {
  auto name = GetLoggerName();
  std::shared_ptr<spdlog::logger> logger;

  if (!filepath.isEmpty()) {
    logger =
        spdlog::basic_logger_mt(name.toStdString(), filepath.toStdString());
  } else {
    logger = spdlog::stderr_color_mt(name.toStdString());
  }

  spdlog::set_default_logger(logger);
  return logger;
}

void SetLogLevel(spdlog::level::level_enum level) {
  return spdlog::set_level(level);
}

void SetLogLevel(const QString &level) {
  return SetLogLevel(ParseLogLevel(level));
}

QString GetCurrentExceptionRepr() {
  auto exc = std::current_exception();
  QString repr;
  if (exc) {
    try {
      std::rethrow_exception(exc);
    } catch (const std::exception &e) {
      repr = QString("%1(%2)").arg(typeid(e).name()).arg(e.what());
    } catch (...) {
      repr = "<unknown C++ exception>";
    }
  }
  return repr;
}

void __stdcall CustomResultMessageCallback(
    _Inout_ wil::FailureInfo *failure,
    _Inout_updates_opt_z_(cchDebugMessage) PWSTR pszDebugMessage,
    _Pre_satisfies_(cchDebugMessage > 0) size_t cchDebugMessage
) noexcept {
  PCSTR pszType = "";
  switch (failure->type) {
  case wil::FailureType::Exception:
    pszType = "Exception";
    break;
  case wil::FailureType::Return:
    if (WI_IsFlagSet(failure->flags, wil::FailureFlags::NtStatus)) {
      pszType = "ReturnNt";
    } else {
      pszType = "ReturnHr";
    }
    break;
  case wil::FailureType::Log:
    if (WI_IsFlagSet(failure->flags, wil::FailureFlags::NtStatus)) {
      pszType = "LogNt";
    } else {
      pszType = "LogHr";
    }
    break;
  case wil::FailureType::FailFast:
    pszType = "FailFast";
    break;
  }

  DWORD errorTextLen = 256;
  wil::unique_hlocal_string errorTextBuffer;
  LONG errorCode = 0;
  QString errorText;

  if (WI_IsFlagSet(failure->flags, wil::FailureFlags::NtStatus)) {
    errorCode = failure->status;
    errorTextBuffer = wil::make_unique_string<wil::unique_hlocal_string>(
        nullptr, errorTextLen
    );
    if (wil::details::g_pfnFormatNtStatusMsg) {
      wil::details::g_pfnFormatNtStatusMsg(
          failure->status, reinterpret_cast<PWSTR>(errorTextBuffer.get()),
          errorTextLen
      );
      errorText = QString::fromWCharArray(errorTextBuffer.get()).trimmed();
    }
  } else {
    errorCode = failure->hr;
    if (errorCode == 0x8007023E) {
      errorText = GetCurrentExceptionRepr();
    } else {
      errorTextLen = FormatMessageW(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
              FORMAT_MESSAGE_ALLOCATE_BUFFER,
          nullptr, failure->hr, GetThreadUILanguage(),
          reinterpret_cast<LPWSTR>(errorTextBuffer.put()), 0, nullptr
      );
      errorText = QString::fromWCharArray(errorTextBuffer.get()).trimmed();
    }
  }

  PWSTR dest = pszDebugMessage;
  PCWSTR destEnd = (pszDebugMessage + cchDebugMessage);

  dest = wil::details::LogStringPrintf(
      dest, destEnd, L"%ws (0x%08X) %hs(%d)", errorText.utf16(), errorCode,
      pszType, failure->cFailureCount
  );

  if (failure->pszMessage != nullptr) {
    dest = wil::details::LogStringPrintf(
        dest, destEnd, L" << %ws", failure->pszMessage
    );
  }
}

void __stdcall CustomLoggingCallback(wil::FailureInfo const &failure) noexcept {
  constexpr std::size_t len = 2048;
  wchar_t buf[len];
  if (SUCCEEDED(wil::GetFailureLogString(buf, len, failure))) {
    auto logger = spdlog::default_logger();
    auto msg = QString::fromWCharArray(buf).trimmed();
    logger->error(msg.toStdString());
    logger->flush();
  }
}

LONG WINAPI CustomExceptionFilter(EXCEPTION_POINTERS *p) {
  auto logger = spdlog::default_logger();
  DWORD code = p->ExceptionRecord->ExceptionCode;
  if (code == 0xE06D7363) {
    logger->critical("Unhandled C++ exception. (0x{:08X})", code);
  } else {
    logger->critical("Unhandled SEH exception. (0x{:08X})", code);
  }
  logger->flush();
  if (OriginalExceptionFilter) {
    return OriginalExceptionFilter(p);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

void CustomTerminateHandler() {
  auto exc = std::current_exception();
  if (exc) {
    try {
      std::rethrow_exception(exc);
    } catch (...) {
      LOG_CAUGHT_EXCEPTION();
    }
  }
  std::abort();
}

spdlog::level::level_enum LevelFromType(QtMsgType type) {
  switch (type) {
  case QtDebugMsg:
    return spdlog::level::debug;
  case QtInfoMsg:
    return spdlog::level::info;
  case QtWarningMsg:
    return spdlog::level::warn;
  case QtCriticalMsg:
    return spdlog::level::err;
  case QtFatalMsg:
    return spdlog::level::critical;
  }
  return spdlog::level::debug;
}

spdlog::source_loc SourceLocFromContext(const QMessageLogContext &context) {
  return spdlog::source_loc(context.file, context.line, context.function);
}

void CustomMessageHandler(
    QtMsgType type, const QMessageLogContext &context, const QString &msg
) {
  auto logger = spdlog::default_logger();
  logger->log(
      SourceLocFromContext(context), LevelFromType(type), msg.toStdString()
  );
}

void InstallCustomHandlers() {
  wil::SetResultLoggingCallback(CustomLoggingCallback);
  if (!IsDebuggerPresent()) {
    wil::SetResultMessageCallback(CustomResultMessageCallback);
  }
  OriginalTerminateHandler = std::set_terminate(CustomTerminateHandler);
  OriginalExceptionFilter = SetUnhandledExceptionFilter(CustomExceptionFilter);
  OriginalMessageHandler = qInstallMessageHandler(CustomMessageHandler);
}

void InitializeLoggingStandalone(const ParsedResult &parsed) {
  bool loggingEnabled = parsed.enableLogging;

  QString logFile;
  spdlog::level::level_enum logLevel = spdlog::level::info;

  if (!parsed.logFile.isEmpty() || !parsed.logDir.isEmpty()) {
    loggingEnabled = true;
  }

  if (!parsed.logFile.isEmpty()) {
    logFile = parsed.logFile;
  } else if (!parsed.logDir.isEmpty()) {
    logFile = GetLogFilename(parsed.logDir);
  } else if (loggingEnabled) {
    logFile = GetLogFilename();
  }

  if (!parsed.logLevel.isEmpty()) {
    logLevel = ParseLogLevel(parsed.logLevel, logLevel);
  }

  CreateDefaultLogger(logFile);
  SetLogLevel(logLevel);
  InstallCustomHandlers();
}

void InitializeLoggingSurrogate(const QString &clsid) {
  LoggingSettings settings = ReadLoggingSettings(clsid);

  QString logFile;
  spdlog::level::level_enum logLevel = spdlog::level::info;

  if (settings.enabled) {
    if (!settings.directory.isEmpty()) {
      logFile = GetLogFilename(settings.directory);
    } else {
      logFile = GetLogFilename();
    }

    if (!settings.level.isEmpty()) {
      logLevel = ParseLogLevel(settings.level, logLevel);
    }
  }

  CreateDefaultLogger(logFile);
  SetLogLevel(logLevel);
  InstallCustomHandlers();
}
