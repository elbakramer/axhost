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

#ifndef REGISTRY_HELPER_H
#define REGISTRY_HELPER_H

#include <windows.h>

#include <QString>

#include "logging.h"

// Register axhost as DllSurrogate for a CLSID
// Sets HKCR\CLSID\{clsid}\AppID and HKCR\AppID\{appid}\DllSurrogate
HRESULT RegisterSurrogate(const QString &clsid, const QString &appid);

// Unregister DllSurrogate for a CLSID
// Removes HKCR\CLSID\{clsid}\AppID and HKCR\AppID\{appid}\DllSurrogate
HRESULT UnregisterSurrogate(const QString &clsid);

// Write logging settings to registry for a specific AppID
// Sets HKCR\AppID\{appid}\LoggingEnabled, LoggingLevel, LoggingDirectory
HRESULT
WriteLoggingSettings(const QString &appid, const LoggingSettings &settings);

// Read logging settings from registry for a specific CLSID
// Reads from HKCR\AppID\{appid}\ where appid is found via CLSID\{clsid}\AppID
LoggingSettings ReadLoggingSettings(const QString &clsid);

// Get the full path to the current executable
QString GetExecutablePath();

// Check if running with administrator privileges
bool IsRunningAsAdmin();

#endif // REGISTRY_HELPER_H
