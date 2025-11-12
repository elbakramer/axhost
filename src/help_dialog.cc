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

#include "help_dialog.h"

#include <Qt>

#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextCursor>
#include <QTextOption>
#include <QVBoxLayout>

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(QCoreApplication::applicationName());
  setModal(true);
  setAttribute(Qt::WA_DeleteOnClose);
  setMinimumSize(800, 600);
  setSizeGripEnabled(true);

  auto *vbox = new QVBoxLayout(this);
  m_text = new QPlainTextEdit(this);
  m_text->setReadOnly(true);
  m_text->document()->setDocumentMargin(12);
  QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  mono.setStyleHint(QFont::Monospace);
  m_text->setFont(mono);
  m_text->setWordWrapMode(QTextOption::NoWrap);
  vbox->addWidget(m_text, 1);

  auto *hbox = new QHBoxLayout();
  hbox->addStretch();
  auto *ok = new QPushButton("Confirm", this);
  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  hbox->addWidget(ok);
  vbox->addLayout(hbox);
}

void HelpDialog::setHelpText(const QString &text) {
  m_text->setPlainText(text);
  m_text->moveCursor(QTextCursor::Start);
}
