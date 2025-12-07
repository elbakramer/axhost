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

#ifndef LOGGING_H
#define LOGGING_H

#include <QString>

#include "command_line_parser.h"

struct LoggingSettings {
  bool enabled = false;
  QString level;
  QString directory;
  QString file;
};

void InitializeLoggingStandalone(const ParsedResult &parsed);
void InitializeLoggingSurrogate(const QString &clsid);

#endif // LOGGING_H
