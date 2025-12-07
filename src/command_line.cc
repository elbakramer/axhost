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

#include "command_line.h"

#include <QChar>
#include <QLatin1Char>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QStringView>

static const QSet<QChar> specials = [] {
  QSet<QChar> s;
  const QString chars = QStringLiteral("\t \"&\'(){}*<>\\`^|\n");
  s.reserve(chars.size());
  for (const QChar &c : chars) {
    s.insert(c);
  }
  return s;
}();

bool ArgNeedsQuotes(QStringView arg) {
  if (arg.isEmpty())
    return true;
  for (QChar c : arg) {
    if (specials.contains(c))
      return true;
  }
  return false;
}

QString QuoteSingleArg(QStringView arg) {
  QString result;
  result.reserve(arg.size() + 2);
  result += QLatin1Char('"');
  int backslashes = 0;
  for (QChar ch : arg) {
    if (ch == QLatin1Char('\\')) {
      ++backslashes;
      continue;
    }
    if (ch == QLatin1Char('"')) {
      if (backslashes > 0) {
        result += QString(backslashes * 2, QLatin1Char('\\'));
        backslashes = 0;
      }
      result += QLatin1Char('\\');
      result += QLatin1Char('"');
    } else {
      if (backslashes > 0) {
        result += QString(backslashes, QLatin1Char('\\'));
        backslashes = 0;
      }
      result += ch;
    }
  }
  if (backslashes > 0) {
    result += QString(backslashes * 2, QLatin1Char('\\'));
  }
  result += QLatin1Char('"');
  return result;
}

QString CreateCommandLine(
    const QString &program, const QStringList &arguments,
    const QString &nativeArguments
) {
  QStringList args;

  if (!program.isEmpty()) {
    QString prog = program;
    prog.replace(QLatin1Char('/'), QLatin1Char('\\'));
    args << QuoteSingleArg(prog);
  }

  for (const QString &arg : arguments) {
    if (ArgNeedsQuotes(arg)) {
      args << QuoteSingleArg(arg);
    } else {
      args << arg;
    }
  }

  QString cmdLine = args.join(QLatin1Char(' '));

  if (!nativeArguments.isEmpty()) {
    if (!cmdLine.isEmpty())
      cmdLine += QLatin1Char(' ');
    cmdLine += nativeArguments;
  }

  return cmdLine;
}

QString CreateCommandLine(const QStringList &arguments) {
  return CreateCommandLine(arguments.at(0), arguments.sliced(1), "");
}

QString CreateCommandLine(int argc, char *argv[]) {
  QStringList args;
  for (int i = 0; i < argc; ++i) {
    QString arg = QString::fromLocal8Bit(argv[i]);
    args.append(arg);
  }
  return CreateCommandLine(args);
}
