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

#include "as_duration.h"

#include <QStringList>

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
