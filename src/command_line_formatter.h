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

#ifndef COMMAND_LINE_FORMATTER_H
#define COMMAND_LINE_FORMATTER_H

#include <CLI/CLI.hpp>

class Formatter : public CLI::Formatter {
public:
  Formatter()
      : CLI::Formatter() {
    long_option_alignment_ratio(2 / static_cast<float>(column_width_));
  }

  virtual std::string make_description(const CLI::App *app) const {
    std::string desc = CLI::Formatter::make_description(app);
    desc.erase(desc.find_last_not_of("\r\n") + 1);
    if (!desc.empty())
      desc += "\n";
    return desc;
  }

  virtual std::string
  make_expanded(const CLI::App *sub, CLI::AppFormatMode mode) const {
    std::string expanded = CLI::Formatter::make_expanded(sub, mode);
    if (!expanded.empty())
      expanded = "\n\n" + expanded;
    return expanded;
  }

  // Modified based on the following implementation
  // https://github.com/CLIUtils/CLI11/blob/bebc5a72c3d6ae6991abecede6cb9668267d9f86/include/CLI/impl/Formatter_inl.hpp#L276
  virtual std::string
  make_option(const CLI::Option *opt, bool is_positional) const {
    std::stringstream out;
    if (is_positional) {
      const std::string left =
          "  " + make_option_name(opt, true) + make_option_opts(opt);
      const std::string desc = make_option_desc(opt);
      out << std::setw(static_cast<int>(column_width_)) << std::left << left;

      if (!desc.empty()) {
        bool skipFirstLinePrefix = true;
        if (left.length() >= column_width_) {
          out << '\n';
          skipFirstLinePrefix = false;
        }
        CLI::detail::streamOutAsParagraph(
            out, desc, right_column_width_, std::string(column_width_, ' '),
            skipFirstLinePrefix
        );
      }
    } else {
      const std::string namesCombined = make_option_name(opt, false);
      const std::string opts = make_option_opts(opt);
      const std::string desc = make_option_desc(opt);

      // Split all names at comma and sort them into short names and long names
      const auto names = CLI::detail::split(namesCombined, ',');
      std::vector<std::string> vshortNames;
      std::vector<std::string> vlongNames;
      std::for_each(
          names.begin(), names.end(),
          [&vshortNames, &vlongNames](const std::string &name) {
            if (name.find("--", 0) != std::string::npos)
              vlongNames.push_back(name);
            else
              vshortNames.push_back(name);
          }
      );

      // Assemble short and long names
      std::string shortNames = CLI::detail::join(vshortNames, ", ");
      std::string longNames = CLI::detail::join(vlongNames, ", ");

      // Calculate setw sizes
      // Short names take enough width to align long names at the desired ratio
      const auto shortNamesColumnWidth = static_cast<int>(
          static_cast<float>(column_width_) * long_option_alignment_ratio_
      );
      const auto longNamesColumnWidth =
          static_cast<int>(column_width_) - shortNamesColumnWidth;
      int shortNamesOverSize = 0;

      // Print short names
      if (!shortNames.empty()) {
        shortNames = "  " + shortNames; // Indent
        if (longNames.empty() && !opts.empty())
          shortNames += opts; // Add opts if only short names and no long names
        if (!longNames.empty())
          shortNames += ",";
        if (static_cast<int>(shortNames.length()) >= shortNamesColumnWidth) {
          shortNames += " ";
          shortNamesOverSize =
              static_cast<int>(shortNames.length()) - shortNamesColumnWidth;
        }
        out << std::setw(shortNamesColumnWidth) << std::left << shortNames;
      } else {
        out << std::setw(shortNamesColumnWidth) << std::left << "  ";
      }

      // Adjust long name column width in case of short names column reaching
      // into long names column
      shortNamesOverSize =
          (std::min)(shortNamesOverSize,
                     longNamesColumnWidth); // Prevent negative result with
                                            // unsigned integers
      const auto adjustedLongNamesColumnWidth =
          longNamesColumnWidth - shortNamesOverSize;

      // Print long names
      if (!longNames.empty()) {
        if (!opts.empty())
          longNames += opts;
        if (static_cast<int>(longNames.length()) >=
            adjustedLongNamesColumnWidth)
          longNames += " ";

        out << std::setw(adjustedLongNamesColumnWidth) << std::left
            << longNames;
      } else {
        out << std::setw(adjustedLongNamesColumnWidth) << std::left << "";
      }

      if (!desc.empty()) {
        bool skipFirstLinePrefix = true;
        if (out.str().length() > column_width_) {
          out << '\n';
          skipFirstLinePrefix = false;
        }
        CLI::detail::streamOutAsParagraph(
            out, desc, right_column_width_, std::string(column_width_, ' '),
            skipFirstLinePrefix
        );
      }
    }

    out << '\n';
    return out.str();
  }
};

#endif // COMMAND_LINE_FORMATTER_H
