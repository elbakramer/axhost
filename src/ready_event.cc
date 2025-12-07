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

#include "ready_event.h"

QString CreateDefaultHostReadyEventName() {
  return QString("Local\\AxHost_Ready_%1").arg(GetCurrentProcessId());
}

wil::unique_event CreateEventHandle(const QString &event) {
  QString ready;
  bool valid = !event.isEmpty();
  if (!valid) {
    ready = CreateDefaultHostReadyEventName();
  }
  const QString &name = valid ? event : ready;
  wil::unique_event evt(
      ::CreateEventW(
          nullptr, TRUE, FALSE, reinterpret_cast<LPCWSTR>(name.utf16())
      )
  );
  return evt;
}

HostReadyEvent::HostReadyEvent(const QString &event)
    : m_event(CreateEventHandle(event)) {}

HostReadyEvent::HostReadyEvent()
    : HostReadyEvent(CreateDefaultHostReadyEventName()) {}

BOOL HostReadyEvent::SetEvent() {
  if (m_event)
    return ::SetEvent(m_event.get());
  return FALSE;
}

BOOL HostReadyEvent::IsValid() { return m_event.is_valid(); }
