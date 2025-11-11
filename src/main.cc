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

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <ocidl.h>
#include <windows.h>

#include <QApplication>
#include <QAtomicInteger>
#include <QAxWidget>
#include <QDialog>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QTextCursor>
#include <QTextOption>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>

#include <CLI/CLI.hpp>

#include "config.h"

static QAtomicInteger<ulong> g_addInst = 0;
static QAtomicInteger<ulong> g_serverLock = 0;

static bool g_warnedIdle = false;

QString GetLastErrorMessage(DWORD err = GetLastError()) {
  LPWSTR buf = nullptr;
  DWORD len = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0,
      nullptr
  );
  QString msg;
  if (len && buf) {
    msg = QString::fromWCharArray(buf, len).trimmed();
  } else {
    msg = "Unknown error";
  }
  if (buf)
    LocalFree(buf);
  return msg;
}

HRESULT ReadTypeLibIdFromCLSID(REFCLSID rclsid, GUID *pLibid) {
  if (!pLibid)
    return E_POINTER;

  LPOLESTR clsidStr = nullptr;
  HRESULT hr = StringFromCLSID(rclsid, &clsidStr);
  if (FAILED(hr))
    return hr;

  std::wstring key = L"CLSID\\";
  key += clsidStr;
  key += L"\\TypeLib";
  CoTaskMemFree(clsidStr);

  HKEY hKey = nullptr;
  LONG rc = RegOpenKeyExW(HKEY_CLASSES_ROOT, key.c_str(), 0, KEY_READ, &hKey);
  if (rc != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(rc);

  wchar_t buf[64];
  DWORD sz = sizeof(buf);
  rc = RegQueryValueExW(hKey, nullptr, nullptr, nullptr, (LPBYTE)buf, &sz);
  RegCloseKey(hKey);
  if (rc != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(rc);

  return CLSIDFromString(buf, pLibid);
}

HRESULT
FindLatestTypeLibVersion(REFGUID libid, USHORT *pMajor, USHORT *pMinor) {
  if (!pMajor || !pMinor)
    return E_POINTER;
  *pMajor = *pMinor = 0;

  LPOLESTR libStr = nullptr;
  HRESULT hr = StringFromCLSID(libid, &libStr);
  if (FAILED(hr))
    return hr;

  std::wstring key = L"TypeLib\\";
  key += libStr;
  CoTaskMemFree(libStr);

  HKEY hKey = nullptr;
  LONG rc = RegOpenKeyExW(HKEY_CLASSES_ROOT, key.c_str(), 0, KEY_READ, &hKey);
  if (rc != ERROR_SUCCESS)
    return TYPE_E_LIBNOTREGISTERED;

  DWORD index = 0;
  wchar_t name[64];
  DWORD namelen = _countof(name);
  while ((rc = RegEnumKeyExW(
              hKey, index++, name, &namelen, nullptr, nullptr, nullptr, nullptr
          )) == ERROR_SUCCESS) {
    unsigned x = 0, y = 0;
    if (swscanf_s(name, L"%u.%u", &x, &y) == 2) {
      if (x > *pMajor || (x == *pMajor && y > *pMinor)) {
        *pMajor = (USHORT)x;
        *pMinor = (USHORT)y;
      }
    }
    namelen = _countof(name);
  }
  RegCloseKey(hKey);

  return (*pMajor || *pMinor) ? S_OK : TYPE_E_LIBNOTREGISTERED;
}

HRESULT GetTypeLibFromCLSID(REFCLSID rclsid, ITypeLib **ppTL) {
  if (!ppTL)
    return E_POINTER;
  *ppTL = nullptr;

  GUID libid{};
  USHORT maj = 0, min = 0;

  HRESULT hr = ReadTypeLibIdFromCLSID(rclsid, &libid);
  if (FAILED(hr))
    return hr;

  hr = FindLatestTypeLibVersion(libid, &maj, &min);
  if (FAILED(hr))
    return hr;

  return LoadRegTypeLib(libid, maj, min, LOCALE_USER_DEFAULT, ppTL);
}

HRESULT FindDefaultSourceIID(ITypeInfo *pCoClassTI, GUID *pOut) {
  if (!pCoClassTI || !pOut)
    return E_POINTER;

  *pOut = GUID_NULL;

  for (UINT i = 0;; ++i) {
    INT implFlags = 0;
    HRESULT hr = pCoClassTI->GetImplTypeFlags(i, &implFlags);

    if (FAILED(hr))
      break;

    const bool isDefault = !!(implFlags & IMPLTYPEFLAG_FDEFAULT);
    const bool isSource = !!(implFlags & IMPLTYPEFLAG_FSOURCE);

    if (!isDefault)
      continue;
    if (!isSource)
      continue;

    HREFTYPE href = 0;
    hr = pCoClassTI->GetRefTypeOfImplType(i, &href);
    if (FAILED(hr))
      continue;

    ITypeInfo *pTI;
    hr = pCoClassTI->GetRefTypeInfo(href, &pTI);
    if (FAILED(hr) || !pTI)
      continue;

    TYPEATTR *pTA = nullptr;
    hr = pTI->GetTypeAttr(&pTA);
    if (FAILED(hr) || !pTA) {
      pTI->Release();
      continue;
    }

    *pOut = pTA->guid;
    pTI->ReleaseTypeAttr(pTA);
    pTI->Release();
    return S_OK;
  }

  return TYPE_E_ELEMENTNOTFOUND;
}

class ComInitializeContext {
private:
  bool m_initialized;
  HRESULT m_hr;

public:
  ComInitializeContext() {
    m_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    m_initialized = (m_hr == S_OK || m_hr == S_FALSE);
  }

  ~ComInitializeContext() {
    if (m_initialized) {
      CoUninitialize();
    }
  }

  bool initialized() { return m_initialized; }
  HRESULT result() { return m_hr; }
};

class ClassSpec {
public:
  QString clsid;
  QString alias;
  DWORD clsctx_create = CLSCTX_INPROC_SERVER;
  DWORD clsctx_reigster = CLSCTX_LOCAL_SERVER;
  DWORD regcls = REGCLS_SINGLEUSE | REGCLS_MULTI_SEPARATE | REGCLS_SUSPENDED;

public:
  static ClassSpec fromString(const QString &item) {
    ClassSpec spec;
    QStringList parts = item.split("/");
    if (parts.size() > 0 && !parts[0].isEmpty()) {
      spec.clsid = parts[0].trimmed();
    }
    if (parts.size() > 1 && !parts[1].isEmpty()) {
      spec.alias = parts[1].trimmed();
    } else {
      spec.alias = spec.clsid;
    }
    if (parts.size() > 2 && !parts[2].isEmpty()) {
      bool ok = false;
      DWORD value = parts[2].toUInt(&ok, 0);
      if (ok) {
        spec.clsctx_create = value;
      }
    }
    if (parts.size() > 3 && !parts[3].isEmpty()) {
      bool ok = false;
      DWORD value = parts[3].toUInt(&ok, 0);
      if (ok) {
        spec.clsctx_reigster = value;
      }
    }
    if (parts.size() > 4 && !parts[4].isEmpty()) {
      bool ok = false;
      DWORD value = parts[4].toUInt(&ok, 0);
      if (ok) {
        spec.regcls = value;
      }
    }
    return spec;
  }
};

inline std::istream &operator>>(std::istream &is, ClassSpec &out) {
  std::string token;
  if (!(is >> token))
    return is;
  out = ClassSpec::fromString(QString::fromStdString(token));
  return is;
}

inline std::ostream &operator<<(std::ostream &os, const ClassSpec &s) {
  os << s.clsid.toStdString() << "/" << s.alias.toStdString() << "/"
     << s.clsctx_create << "/" << s.clsctx_reigster << "/" << s.regcls;
  return os;
}

class ExitConditionChecker : public QTimer {
  Q_OBJECT

private:
  ulong m_timeout;

public:
  ExitConditionChecker(ulong timeout, QObject *parent = nullptr)
      : m_timeout(timeout),
        QTimer(parent) {
    connect(this, &QTimer::timeout, this, &ExitConditionChecker::check);
    start();
  }

public slots:
  void start() { QTimer::start(m_timeout); }

  void check() {
    ULONG a = CoAddRefServerProcess();
    ULONG r = CoReleaseServerProcess();
    if (r <= 0) {
      if (g_addInst == 0 && !g_warnedIdle) {
        g_warnedIdle = true;
        QString text = QString(R"(
Warning: Timed Out For No Interaction

No COM interaction has occurred for the timeout period (%1 ms).
Server process will now terminate.
)")
                           .arg(m_timeout)
                           .trimmed();
        QMessageBox::warning(
            nullptr, QCoreApplication::applicationName(), text
        );
      }
      if (g_serverLock.loadRelaxed() <= 0) {
        QCoreApplication::exit();
      }
    }
  }

  void checkLater(int delay = 5000) {
    QTimer::singleShot(delay, this, &ExitConditionChecker::check);
  }
};

class HostingClass : public IProvideClassInfo2 {
private:
  volatile LONG m_ref = 1;
  ClassSpec m_spec;
  QPointer<QAxWidget> m_control;
  QPointer<ExitConditionChecker> m_checker;
  CLSID m_classId;

public:
  explicit HostingClass(const ClassSpec &spec, ExitConditionChecker *checker)
      : m_spec(spec),
        m_control(new QAxWidget()),
        m_checker(checker) {
    m_control->setClassContext(spec.clsctx_create);

    HRESULT conv =
        CLSIDFromString(spec.clsid.toStdWString().c_str(), &m_classId);

    if (FAILED(conv)) {
      QString text = QString(R"(
Error: CLSID Parsing Failed

Invalid CLSID: '%1'
)")
                         .arg(spec.clsid)
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      return;
    }

    if (m_control->setControl(spec.clsid) && !m_control->isNull()) {
      g_addInst.ref();
      g_serverLock.ref();
      CoAddRefServerProcess();
    } else {
      DWORD err = GetLastError();
      QString msg = GetLastErrorMessage(err);
      QString text = QString(R"(
Error: Control Loading Failed

Failed to load control.

CLSID: '%1'
CLSCTX: 0x%2

Error message:
%3)")
                         .arg(spec.clsid)
                         .arg(QString::number(spec.clsctx_create, 16))
                         .arg(msg)
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    }
  }

  ~HostingClass() {}

  bool IsValid() { return m_control && !m_control->isNull(); }

  ULONG STDMETHODCALLTYPE AddRef() override {
    return InterlockedIncrement(&m_ref);
  }

  ULONG STDMETHODCALLTYPE Release() override {
    ULONG r = InterlockedDecrement(&m_ref);
    if (r <= 0) {
      if (m_control) {
        bool shouldReset = IsValid();
        bool shouldExit = false;
        bool shouldTestExit = false;
        if (shouldReset) {
          m_control->resetControl();
          shouldExit = CoReleaseServerProcess() == 0;
          shouldTestExit = !g_serverLock.deref();
        }
        m_control->deleteLater();
        if (shouldExit) {
          QCoreApplication::exit();
        } else if (shouldTestExit) {
          m_checker->checkLater();
        }
      }
      delete this;
    }
    return r;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override {
    if (!ppv)
      return E_POINTER;
    if (riid == IID_IUnknown) {
      *ppv = this;
      AddRef();
      return S_OK;
    }
    if (!m_control || m_control->isNull()) {
      *ppv = nullptr;
      return E_NOINTERFACE;
    }
    if (riid == IID_IProvideClassInfo || riid == IID_IProvideClassInfo2) {
      HRESULT hr = m_control->queryInterface(riid, ppv);
      if (SUCCEEDED(hr)) {
        return hr;
      }
      *ppv = this;
      AddRef();
      return S_OK;
    }
    return m_control->queryInterface(riid, ppv);
  }

  HRESULT STDMETHODCALLTYPE GetClassInfo(ITypeInfo **ppTI) override {
    if (!ppTI)
      return E_POINTER;
    ITypeLib *pTL;
    HRESULT hr = GetTypeLibFromCLSID(m_classId, &pTL);
    if (FAILED(hr)) {
      return hr;
    }
    hr = pTL->GetTypeInfoOfGuid(m_classId, ppTI);
    pTL->Release();
    return hr;
  }

  HRESULT STDMETHODCALLTYPE GetGUID(DWORD dwGuidKind, GUID *pGUID) override {
    if (!pGUID)
      return E_POINTER;
    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
      return E_INVALIDARG;
    ITypeInfo *pTI;
    HRESULT hr = GetClassInfo(&pTI);
    if (FAILED(hr)) {
      return hr;
    }
    hr = FindDefaultSourceIID(pTI, pGUID);
    pTI->Release();
    return hr;
  }
};

class HostingClassFactory : public IClassFactory {
private:
  volatile LONG m_ref = 1;
  ClassSpec m_spec;
  QPointer<ExitConditionChecker> m_checker;

public:
  explicit HostingClassFactory(
      const ClassSpec &spec, ExitConditionChecker *checker
  )
      : m_spec(spec),
        m_checker(checker) {}

  ~HostingClassFactory() {}

  ULONG STDMETHODCALLTYPE AddRef() override {
    return InterlockedIncrement(&m_ref);
  }

  ULONG STDMETHODCALLTYPE Release() override {
    ULONG r = InterlockedDecrement(&m_ref);
    if (r <= 0) {
      delete this;
    }
    return r;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override {
    if (!ppv)
      return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IClassFactory) {
      *ppv = static_cast<IClassFactory *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }

  HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override {
    if (fLock) {
      g_serverLock.ref();
      CoAddRefServerProcess();
    } else {
      bool shouldExit = CoReleaseServerProcess() == 0;
      bool shouldTestExit = !g_serverLock.deref();
      if (shouldExit) {
        QCoreApplication::exit();
      } else if (shouldTestExit) {
        m_checker->checkLater();
      }
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE
  CreateInstance(IUnknown *outer, REFIID riid, void **ppv) override {
    if (!ppv)
      return E_POINTER;
    if (outer)
      return CLASS_E_NOAGGREGATION;
    HostingClass *obj = new HostingClass(m_spec, m_checker);
    HRESULT hr = E_UNEXPECTED;
    if (obj->IsValid()) {
      hr = obj->QueryInterface(riid, ppv);
    }
    obj->Release();
    return hr;
  }
};

class HostingClassFactoryRegistry {
private:
  QList<DWORD> m_registry;

public:
  HostingClassFactoryRegistry(
      const QList<ClassSpec> &specs, ExitConditionChecker *checker
  ) {
    HRESULT sus = CoSuspendClassObjects();
    if (FAILED(sus)) {
      return;
    }
    int success = 0;
    for (const ClassSpec &spec : specs) {
      CLSID classId;
      HRESULT conv =
          CLSIDFromString(spec.alias.toStdWString().c_str(), &classId);
      if (FAILED(conv)) {
        QString text = QString(R"(
Error: CLSID Parsing Failed

Invalid CLSID: '%1'
)")
                           .arg(spec.alias)
                           .trimmed();
        QMessageBox::warning(
            nullptr, QCoreApplication::applicationName(), text
        );
        continue;
      }
      IClassFactory *factory = new HostingClassFactory(spec, checker);
      DWORD cookie = 0;
      HRESULT reg = CoRegisterClassObject(
          classId, factory, spec.clsctx_reigster, spec.regcls, &cookie
      );
      if (SUCCEEDED(reg)) {
        m_registry.push_back(cookie);
        ++success;
      } else {
        DWORD err = GetLastError();
        QString msg = GetLastErrorMessage(err);
        QString text = QString(R"(
Error: Class Registration Failed

CoRegisterClassObject failed.

CLSID: '%1'
CLSCTX: 0x%2

Error message:
%3)")
                           .arg(spec.alias)
                           .arg(QString::number(spec.clsctx_reigster, 16))
                           .arg(msg)
                           .trimmed();
        QMessageBox::warning(
            nullptr, QCoreApplication::applicationName(), text
        );
      }
      factory->Release();
    }
    if (SUCCEEDED(sus)) {
      HRESULT res = CoResumeClassObjects();
    }
    if (success == 0) {
      QString text = QString(R"(
Error: No Class Registered

No class factory could be registered. Exiting.
)")
                         .trimmed();
      QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
      DWORD err = GetLastError();
      QCoreApplication::exit(err);
    }
  }

  ~HostingClassFactoryRegistry() {
    HRESULT sus = CoSuspendClassObjects();
    for (DWORD cookie : m_registry) {
      HRESULT hr = CoRevokeClassObject(cookie);
    }
    if (SUCCEEDED(sus)) {
      HRESULT res = CoResumeClassObjects();
    }
  }
};

struct ParsedResult {
  QList<ClassSpec> specs;
  int timeout = 60000;

  int code = 0;
  QString msg;
};

class AsDuration : public CLI::AsNumberWithUnit {
private:
  inline static const std::map<std::string, int> m_mapping = {
      {"ms", 1},
      {"s", 1000},
      {"m", 60 * 1000},
      {"h", 60 * 60 * 1000},
  };
  inline static const std::vector<std::string> m_units = {"ms", "s", "m", "h"};

private:
  static std::string generate_description(
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

public:
  AsDuration()
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
};

class HelpDialog : public QDialog {
  Q_OBJECT
public:
  HelpDialog(QWidget *parent = nullptr)
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

  void setHelpText(const QString &text) {
    m_text->setPlainText(text);
    m_text->moveCursor(QTextCursor::Start);
  }

private:
  QPlainTextEdit *m_text = nullptr;
};

class CommandLineParser {
  Q_DECLARE_TR_FUNCTIONS(CommandLineParser)

private:
  const QString m_appName = QCoreApplication::applicationName();
  const QString m_appVersion = QCoreApplication::applicationVersion();
  const QString m_appDescription =
      "A minimal host process for COM/ActiveX controls";

  CLI::App m_app;
  ParsedResult m_result;

public:
  CommandLineParser()
      : m_app(m_appDescription.toStdString(), m_appName.toStdString()) {
    m_app.allow_windows_style_options();

    AsDuration as_duration;

    std::string as_duration_desc = as_duration.get_description();
    as_duration.description("");

    m_app.set_help_flag("--help", "Print this help message and exit.");
    m_app.set_version_flag(
        "--version", m_appVersion.toStdString(), "Print app version and exit."
    );

    m_app
        .add_option("--clsid", m_result.specs, R"(
            Register a COM class (repeatable).
            Format: c[/a[/cc[/cr[/r]]]] where
            - c  = CLSID
            - a  = alias CLSID
            - cc = CLSCTX for Create (DWORD)
            - cr = CLSCTX for Register (DWORD)
            - r  = REGCLS (DWORD)
            Use decimal or 0x-prefixed hex for DWORD values.
            Omit a field (\"//\") to keep its default:
            - a=c,
            - cc=CLSCTX_INPROC_SERVER,
            - cr=CLSCTX_LOCAL_SERVER,
            - r=REGCLS_SINGLEUSE|REGCLS_MULTI_SEPARATE|REGCLS_SUSPENDED.
            Examples:
            - {CLSID}/{ALIAS}
            - {CLSID}/{ALIAS}/0x1/0x4/0x2
            - {CLSID}//0x1//0x2
            Invalid values are currently being ignored.
        )")
        ->type_name("<item>")
        ->expected(-1);
    m_app
        .add_option(
            "--timeout", m_result.timeout,
            "Exit automatically if no COM activity occurs for the given time "
            "(in milliseconds if no unit is specified, default=60000)."
        )
        ->transform(as_duration)
        ->type_name(as_duration_desc);
  }

  ParsedResult parse(int argc, char *argv[]) {
    try {
      m_app.parse(argc, argv);
    } catch (const CLI::CallForHelp &e) {
      m_result.code = m_app.exit(e);
      m_result.msg = e.what();
      HelpDialog help;
      help.setHelpText(QString::fromStdString(m_app.help()));
      help.exec();
    } catch (const CLI::CallForAllHelp &e) {
      m_result.code = m_app.exit(e);
      m_result.msg = e.what();
      QMessageBox::information(
          nullptr, QCoreApplication::applicationName(),
          QString::fromStdString(m_app.help("", CLI::AppFormatMode::All))
      );
    } catch (const CLI::CallForVersion &e) {
      m_result.code = m_app.exit(e);
      m_result.msg = e.what();
      QMessageBox::about(
          nullptr, QCoreApplication::applicationName(),
          QString::fromStdString(m_app.version())
      );
    } catch (const CLI::ParseError &e) {
      m_result.code = m_app.exit(e);
      m_result.msg = e.what();
      QMessageBox::critical(
          nullptr, QCoreApplication::applicationName(),
          QString::fromStdString(e.what())
      );
    }
    return m_result;
  }
};

int main(int argc, char *argv[]) {
  ComInitializeContext coinit;
  QApplication app(argc, argv);

  app.setApplicationDisplayName(CMAKE_PROJECT_NAME);
  app.setApplicationVersion(CMAKE_PROJECT_VERSION);

  if (!coinit.initialized()) {
    DWORD err = GetLastError();
    QString msg = GetLastErrorMessage(err);
    QString text = QString(R"(
Error: COM Initialization Failed

CoInitializeEx failed.

Error message:
%1)")
                       .arg(msg)
                       .trimmed();
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), text);
    return err;
  }

  CommandLineParser parser;
  ParsedResult parsed = parser.parse(argc, argv);

  if (parsed.code != 0 || parsed.specs.empty()) {
    return parsed.code;
  }

  ExitConditionChecker *checker =
      new ExitConditionChecker(parsed.timeout, &app);
  HostingClassFactoryRegistry registry(parsed.specs, checker);

  return app.exec();
}

#include "main.moc"
