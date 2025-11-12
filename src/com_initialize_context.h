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

#ifndef COM_INITIALIZE_CONTEXT_H
#define COM_INITIALIZE_CONTEXT_H

#include <windows.h>

class ComInitializeContext {
private:
  bool m_initialized;
  HRESULT m_hr;

public:
  ComInitializeContext(DWORD dwCoInit = COINIT_APARTMENTTHREADED);
  ~ComInitializeContext();

  bool IsInitialized();
  HRESULT Result();
};

#endif // COM_INITIALIZE_CONTEXT_H
