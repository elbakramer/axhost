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

#ifndef AS_DURATION_H
#define AS_DURATION_H

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

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

#endif // AS_DURATION_H
