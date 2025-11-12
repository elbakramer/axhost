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

#ifndef HELP_DIALOG_H
#define HELP_DIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QString>
#include <QWidget>

class HelpDialog : public QDialog {
  Q_OBJECT

private:
  QPlainTextEdit *m_text = nullptr;

public:
  HelpDialog(QWidget *parent = nullptr);

  void setHelpText(const QString &text);
};

#endif // HELP_DIALOG_H
