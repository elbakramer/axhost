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

#include "registry_helper.h"

#include <wil/registry.h>
#include <windows.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QString>

QString GetExecutablePath() {
  wchar_t path[MAX_PATH];
  GetModuleFileNameW(NULL, path, MAX_PATH);
  return QString::fromWCharArray(path);
}

bool IsRunningAsAdmin() {
  BOOL isAdmin = FALSE;
  PSID adminGroup = NULL;
  SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

  if (AllocateAndInitializeSid(
          &ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
          0, 0, 0, 0, 0, 0, &adminGroup
      )) {
    CheckTokenMembership(NULL, adminGroup, &isAdmin);
    FreeSid(adminGroup);
  }

  return isAdmin == TRUE;
}

HRESULT RegisterSurrogate(const QString &clsid, const QString &appid) {
  if (!IsRunningAsAdmin()) {
    QString text = QString(R"(
Error: Administrator Privileges Required

Registering COM surrogates requires administrator privileges.
Please run this command as administrator.
)")
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    return E_ACCESSDENIED;
  }

  QString exePath = GetExecutablePath();
  QString effectiveAppId = appid.isEmpty() ? clsid : appid;

  try {
    // Set HKCR\CLSID\{clsid}\AppID = {appid}
    QString clsidPath = QString("CLSID\\%1").arg(clsid);
    wil::unique_hkey clsidKey;
    HRESULT hr = wil::reg::create_unique_key_nothrow(
        HKEY_CLASSES_ROOT, clsidPath.toStdWString().c_str(), clsidKey,
        wil::reg::key_access::readwrite
    );

    if (FAILED(hr)) {
      QString text = QString(R"(
Error: Failed to Create Registry Key

Could not create registry key:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                         .arg(clsidPath)
                         .arg(QString::number(hr, 16).toUpper())
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      return hr;
    }

    hr = wil::reg::set_value_string_nothrow(
        clsidKey.get(), L"AppID", effectiveAppId.toStdWString().c_str()
    );
    if (FAILED(hr)) {
      QString text = QString(R"(
Error: Failed to Set AppID

Could not set AppID value in:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                         .arg(clsidPath)
                         .arg(QString::number(hr, 16).toUpper())
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      return hr;
    }

    // Set HKCR\AppID\{appid}\DllSurrogate = "path\to\axhost.exe"
    QString appidPath = QString("AppID\\%1").arg(effectiveAppId);
    wil::unique_hkey appidKey;
    hr = wil::reg::create_unique_key_nothrow(
        HKEY_CLASSES_ROOT, appidPath.toStdWString().c_str(), appidKey,
        wil::reg::key_access::readwrite
    );

    if (FAILED(hr)) {
      QString text = QString(R"(
Error: Failed to Create AppID Key

Could not create registry key:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                         .arg(appidPath)
                         .arg(QString::number(hr, 16).toUpper())
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      return hr;
    }

    hr = wil::reg::set_value_string_nothrow(
        appidKey.get(), L"DllSurrogate", exePath.toStdWString().c_str()
    );
    if (FAILED(hr)) {
      QString text = QString(R"(
Error: Failed to Set DllSurrogate

Could not set DllSurrogate value in:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                         .arg(appidPath)
                         .arg(QString::number(hr, 16).toUpper())
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      return hr;
    }

    QString text = QString(R"(
Success: Surrogate Registered

CLSID: %1
AppID: %2
DllSurrogate: %3

Registry keys created:
- HKEY_CLASSES_ROOT\CLSID\%1\AppID
- HKEY_CLASSES_ROOT\AppID\%2\DllSurrogate
)")
                       .arg(clsid)
                       .arg(effectiveAppId)
                       .arg(exePath)
                       .trimmed();
    QMessageBox::information(
        nullptr, QCoreApplication::applicationName(), text
    );

    return S_OK;
  } catch (...) {
    LOG_CAUGHT_EXCEPTION();
    QMessageBox::critical(
        nullptr, QCoreApplication::applicationName(),
        "Unexpected error during surrogate registration."
    );
    return E_FAIL;
  }
}

HRESULT UnregisterSurrogate(const QString &clsid) {
  if (!IsRunningAsAdmin()) {
    QString text = QString(R"(
Error: Administrator Privileges Required

Unregistering COM surrogates requires administrator privileges.
Please run this command as administrator.
)")
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    return E_ACCESSDENIED;
  }

  try {
    // Read AppID from HKCR\CLSID\{clsid}\AppID
    QString clsidPath = QString("CLSID\\%1").arg(clsid);
    wil::unique_hkey clsidKey;
    HRESULT hr = wil::reg::open_unique_key_nothrow(
        HKEY_CLASSES_ROOT, clsidPath.toStdWString().c_str(), clsidKey,
        wil::reg::key_access::read
    );

    QString appid;
    if (SUCCEEDED(hr)) {
      wil::unique_cotaskmem_string appidValue;
      hr = wil::reg::get_value_string_nothrow(
          clsidKey.get(), L"AppID", appidValue
      );
      if (SUCCEEDED(hr)) {
        appid = QString::fromWCharArray(appidValue.get());
      }
    }

    // Reopen for write access to delete the value
    hr = wil::reg::open_unique_key_nothrow(
        HKEY_CLASSES_ROOT, clsidPath.toStdWString().c_str(), clsidKey,
        wil::reg::key_access::readwrite
    );

    // Delete HKCR\CLSID\{clsid}\AppID value (WIL doesn't provide delete helper)
    if (SUCCEEDED(hr)) {
      LSTATUS status = RegDeleteValueW(clsidKey.get(), L"AppID");
      if (status != ERROR_FILE_NOT_FOUND)
        RETURN_IF_WIN32_ERROR(status);
    }

    // Delete HKCR\AppID\{appid}\DllSurrogate if we found the AppID
    if (!appid.isEmpty()) {
      QString appidPath = QString("AppID\\%1").arg(appid);
      wil::unique_hkey appidKey;
      hr = wil::reg::open_unique_key_nothrow(
          HKEY_CLASSES_ROOT, appidPath.toStdWString().c_str(), appidKey,
          wil::reg::key_access::readwrite
      );
      if (SUCCEEDED(hr)) {
        LSTATUS status = RegDeleteValueW(appidKey.get(), L"DllSurrogate");
        if (status != ERROR_FILE_NOT_FOUND)
          RETURN_IF_WIN32_ERROR(status);
      }
    }

    QString text =
        QString(R"(
Success: Surrogate Unregistered

CLSID: %1 %2

Registry values removed:
- HKEY_CLASSES_ROOT\CLSID\%1\AppID %3
)")
            .arg(clsid)
            .arg(appid.isEmpty() ? "" : QString("\nAppID: %1").arg(appid))
            .arg(
                appid.isEmpty()
                    ? ""
                    : QString("\n- HKEY_CLASSES_ROOT\\AppID\\%1\\DllSurrogate")
                          .arg(appid)
            )
            .trimmed();
    QMessageBox::information(
        nullptr, QCoreApplication::applicationName(), text
    );

    return S_OK;
  } catch (...) {
    LOG_CAUGHT_EXCEPTION();
    QMessageBox::critical(
        nullptr, QCoreApplication::applicationName(),
        "Unexpected error during surrogate unregistration."
    );
    return E_FAIL;
  }
}

HRESULT
WriteLoggingSettings(const QString &appid, const LoggingSettings &settings) {
  if (!IsRunningAsAdmin()) {
    QString text = QString(R"(
Error: Administrator Privileges Required

Writing logging settings requires administrator privileges.
Please run this command as administrator.
)")
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    return E_ACCESSDENIED;
  }

  try {
    QString appidPath = QString("AppID\\%1").arg(appid);
    wil::unique_hkey appidKey;
    HRESULT hr = wil::reg::open_unique_key_nothrow(
        HKEY_CLASSES_ROOT, appidPath.toStdWString().c_str(), appidKey,
        wil::reg::key_access::readwrite
    );

    if (FAILED(hr)) {
      QString text = QString(R"(
Error: Failed to Open AppID Key

Could not open registry key:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                         .arg(appidPath)
                         .arg(QString::number(hr, 16).toUpper())
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      return hr;
    }

    // Write LogEnabled (DWORD)
    DWORD enabledValue = settings.enabled ? 1 : 0;
    hr = wil::reg::set_value_nothrow(
        appidKey.get(), L"LogEnabled", enabledValue
    );
    if (FAILED(hr)) {
      QString text = QString(R"(
Error: Failed to Set LogEnabled

Could not set LogEnabled value in:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                         .arg(appidPath)
                         .arg(QString::number(hr, 16).toUpper())
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      return hr;
    }

    // Write LogLevel (string) if specified
    if (!settings.level.isEmpty()) {
      hr = wil::reg::set_value_string_nothrow(
          appidKey.get(), L"LogLevel", settings.level.toStdWString().c_str()
      );
      if (FAILED(hr)) {
        QString text = QString(R"(
Error: Failed to Set LogLevel

Could not set LogLevel value in:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                           .arg(appidPath)
                           .arg(QString::number(hr, 16).toUpper())
                           .trimmed();
        QMessageBox::critical(
            nullptr, QCoreApplication::applicationName(), text
        );
        return hr;
      }
    }

    // Write LogDirectory (string) if specified
    if (!settings.directory.isEmpty()) {
      hr = wil::reg::set_value_string_nothrow(
          appidKey.get(), L"LogDirectory",
          settings.directory.toStdWString().c_str()
      );
      if (FAILED(hr)) {
        QString text = QString(R"(
Error: Failed to Set LogDirectory

Could not set LogDirectory value in:
HKEY_CLASSES_ROOT\%1

HRESULT: 0x%2
)")
                           .arg(appidPath)
                           .arg(QString::number(hr, 16).toUpper())
                           .trimmed();
        QMessageBox::critical(
            nullptr, QCoreApplication::applicationName(), text
        );
        return hr;
      }
    }

    QString text =
        QString(R"(
Success: Surrogate Logging Configuration Applied

AppID: %1

Registry keys created:
- HKEY_CLASSES_ROOT\AppID\%1\LogEnabled %2 %3
)")
            .arg(appid)
            .arg(
                settings.level.isEmpty()
                    ? ""
                    : QString("\n- HKEY_CLASSES_ROOT\\AppID\\%1\\LogLevel")
                          .arg(appid)
            )
            .arg(
                settings.directory.isEmpty()
                    ? ""
                    : QString("\n- HKEY_CLASSES_ROOT\\AppID\\%1\\LogDirectory")
                          .arg(appid)
            )
            .trimmed();
    QMessageBox::information(
        nullptr, QCoreApplication::applicationName(), text
    );

    return S_OK;
  } catch (...) {
    LOG_CAUGHT_EXCEPTION();
    QMessageBox::critical(
        nullptr, QCoreApplication::applicationName(),
        "Unexpected error while writing logging settings."
    );
    return E_FAIL;
  }
}

LoggingSettings ReadLoggingSettings(const QString &clsid) {
  LoggingSettings settings;

  try {
    // First, read AppID from HKCR\CLSID\{clsid}\AppID
    QString clsidPath = QString("CLSID\\%1").arg(clsid);
    wil::unique_hkey clsidKey;
    HRESULT hr = wil::reg::open_unique_key_nothrow(
        HKEY_CLASSES_ROOT, clsidPath.toStdWString().c_str(), clsidKey,
        wil::reg::key_access::read
    );

    if (FAILED(hr)) {
      LOG_IF_FAILED(hr);
      return settings; // Return default settings
    }

    wil::unique_cotaskmem_string appidValue;
    hr = wil::reg::get_value_string_nothrow(
        clsidKey.get(), L"AppID", appidValue
    );
    if (FAILED(hr)) {
      LOG_IF_FAILED(hr);
      return settings; // Return default settings
    }

    QString appid = QString::fromWCharArray(appidValue.get());

    // Now read logging settings from HKCR\AppID\{appid}
    QString appidPath = QString("AppID\\%1").arg(appid);
    wil::unique_hkey appidKey;
    hr = wil::reg::open_unique_key_nothrow(
        HKEY_CLASSES_ROOT, appidPath.toStdWString().c_str(), appidKey,
        wil::reg::key_access::read
    );

    if (FAILED(hr)) {
      LOG_IF_FAILED(hr);
      return settings; // Return default settings
    }

    // Read LogEnabled (DWORD)
    DWORD enabledValue = 0;
    hr = wil::reg::get_value_nothrow(
        appidKey.get(), L"LogEnabled", &enabledValue
    );
    if (SUCCEEDED(hr)) {
      settings.enabled = (enabledValue != 0);
    }

    // Read LogLevel (string)
    wil::unique_cotaskmem_string levelValue;
    hr = wil::reg::get_value_string_nothrow(
        appidKey.get(), L"LogLevel", levelValue
    );
    if (SUCCEEDED(hr)) {
      settings.level = QString::fromWCharArray(levelValue.get());
    }

    // Read LogDirectory (string)
    wil::unique_cotaskmem_string dirValue;
    hr = wil::reg::get_value_string_nothrow(
        appidKey.get(), L"LogDirectory", dirValue
    );
    if (SUCCEEDED(hr)) {
      settings.directory = QString::fromWCharArray(dirValue.get());
    }

    return settings;
  } catch (...) {
    LOG_CAUGHT_EXCEPTION();
    return settings; // Return default settings on error
  }
}
