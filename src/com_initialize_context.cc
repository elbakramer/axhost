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

#include "com_initialize_context.h"

ComInitializeContext::ComInitializeContext(DWORD dwCoInit) {
  m_hr = CoInitializeEx(nullptr, dwCoInit);
  m_initialized = (m_hr == S_OK || m_hr == S_FALSE);
}

ComInitializeContext::~ComInitializeContext() {
  if (m_initialized) {
    CoUninitialize();
  }
}

bool ComInitializeContext::IsInitialized() { return m_initialized; }
HRESULT ComInitializeContext::Result() { return m_hr; }
