# axhost

**axhost** is a lightweight, standalone COM/ActiveX host process that emulates the behavior of `dllhost.exe`, and addresses two long-standing issues commonly encountered in COM/ActiveX automation - **bitness isolation** and **headless control hosting**.

## Motivation

`axhost` was created to address practical interoperability and hosting problems when working with legacy COM components:

1. **Cross-bitness isolation**

   * Many older COM servers only implement a 32-bit `InProcServer32`.
   * `axhost` allows 64-bit clients to control such components by running them in a dedicated 32-bit surrogate process.

2. **Headless OCX hosting**

   * Some ActiveX controls require a GUI container (`IOleClientSite`, window handle, etc.) to initialize.
   * `axhost` provides a minimal, invisible hosting window internally so that even GUI-dependent controls can be automated from non-GUI clients or services.

## Architecture Overview

`axhost` functions as a **custom surrogate process**.

It dynamically registers one or more COM classes using `CoRegisterClassObject`, and exposes them to other processes via **COM’s native interprocess communication layer (DCOM/SCM/RPCSS)**.
This design achieves process isolation and bitness bridging **without** relying on any external IPC mechanism such as sockets or gRPC.

Internally, each class registration is backed by a `QAxWidget` instance (through Qt's ActiveX container), which provides the minimal windowed environment required for GUI-bound controls.

This project builds upon prior work and discussions around 32 ↔ 64-bit COM/ActiveX hosting. For deeper technical background, see:

* "Accessing 32-bit DLLs from 64-bit code" (Matt Mags Blog): https://blog.mattmags.com/2007/06/30/accessing-32-bit-dlls-from-64-bit-code/
* Stack Overflow: "Load 32-bit DLL library in 64-bit application": https://stackoverflow.com/questions/2265023/load-32-bit-dll-library-in-64-bit-application
* Stack Overflow: "Calling 32-bit code from 64-bit process": https://stackoverflow.com/questions/128445/calling-32bit-code-from-64bit-process
* Writing a Custom Surrogate: https://learn.microsoft.com/en-us/windows/win32/com/writing-a-custom-surrogate

## Key Features

| Feature                            | Description                                                                                              |
| ---------------------------------- | -------------------------------------------------------------------------------------------------------- |
| **Surrogate-style COM hosting** | Mimics `dllhost.exe` behavior, but runs as a user-defined host.                                          |
| **Bitness bridge**              | Allows 64-bit clients to automate 32-bit `InProcServer` components.                                      |
| **Headless OCX container**      | Supports controls that require a GUI without showing any window.                                         |
| **Native COM RPC**              | Uses COM/DCOM RPC internally — no need for custom IPC bridges.                                           |

## Command-Line Usage

`axhost` supports two operation modes:

1. **Surrogate Mode** (no arguments): Registers as a COM surrogate via `CoRegisterSurrogate`. This mode is typically invoked automatically by the COM runtime (SCM/RPCSS) when a client requests a component configured to use `axhost` as its DllSurrogate. The process stays alive with a 60-second timeout protection.

2. **Standalone Mode** (with `--clsid` arguments): Manually registers specific CLSIDs and hosts them directly. This mode is used for explicit, user-controlled component hosting.

### Register one or more classes (Standalone Mode)

```bash
axhost --clsid "{0002DF01-0000-0000-C000-000000000046}" --clsid ...
```

### Detailed format

Each `--clsid` argument follows this structure:

```
c[/a[/cc[/cr[/r]]]]
```

| Field | Meaning                                  | Default                                                         |
| ----- | ---------------------------------------- | ----------------------                                          |
| c     | CLSID (required)                         | —                                                               |
| a     | Alias CLSID (used for registration name) | same as `c`                                                     |
| cc    | `CLSCTX` for creation                    | `CLSCTX_INPROC_SERVER`                                          |
| cr    | `CLSCTX` for registration                | `CLSCTX_LOCAL_SERVER`                                           |
| r     | `REGCLS` flags                           | `REGCLS_MULTI_SEPARATE \| REGCLS_SINGLEUSE` (or `\| REGCLS_MULTIPLEUSE` with `--multiple-use`)  |

#### Notes

* Note that the default alias is the same as the original, so it can replace the original registration entry, effectively shadowing the source CLSID.
* Numeric fields (`cc`, `cr`, `r`) accept decimal or hex (`0x...`) values.
* You can use `//` to skip intermediate fields while keeping their defaults.
* Trailing fields can be omitted entirely if not needed.
* `REGCLS_SUSPENDED` is added automatically during registration when applicable.
* Explicitly specifying the `r` field bypasses all defaults (`REGCLS_MULTI_SEPARATE`, `REGCLS_SUSPENDED`) and global flags (`--single-use`/`--multiple-use`). Your value is used exactly as provided.
* When using `--multiple-use`, the host tries to re-register the original InProc class factory when applicable to prevent the surrogate from instantiating itself. Using an explicit alias is recommended to clearly separate the surrogate registration from the source.

#### Examples

```bash
axhost --clsid "{CLSID}/{ALIAS}"
axhost --clsid "{CLSID}//0x1//0x2"
axhost --clsid "{CLSID}/{ALIAS}/0x1/0x4/0x2"
```

### Surrogate Mode Configuration

To use `axhost` as a DllSurrogate for a specific CLSID, configure the Windows registry:

```reg
[HKEY_CLASSES_ROOT\CLSID\{YOUR-CLSID}]
"AppID"="{YOUR-APP-ID}"

[HKEY_CLASSES_ROOT\AppID\{YOUR-APP-ID}]
"DllSurrogate"="C:\\path\\to\\axhost.exe"
```

When a client requests this CLSID with `CLSCTX_LOCAL_SERVER`, the COM runtime automatically launches `axhost` in Surrogate Mode.

### Timeout

```bash
axhost --clsid "{CLSID}" --timeout 30s
```

Exits automatically if no COM activity occurs within the specified period (default: 1 minute; supports units: `ms`, `s`, `m`, `h`).

**Note**: Both Surrogate Mode and Standalone Mode enforce a 60-second timeout. If no COM interaction occurs during this period, a warning dialog is shown and the process exits gracefully.

### Registration Mode

By default, `axhost` uses **single-use mode** where class factories serve one instance and then unregister.
You can change this behavior with command-line flags:

```bash
# Single-use mode (default) - process exits immediately when instances are released
axhost --clsid "{CLSID}"

# Multiple-use mode - process waits for timeout after instances are released
axhost --clsid "{SOURCE-CLSID}/{ALIAS-CLSID}" --multiple-use
```

**Single-use mode** (`--single-use`, default):
- Class factory serves one instance, then unregisters automatically
- Process exits immediately when all instances are released
- Each client connection requires launching a new host process

**Multiple-use mode** (`--multiple-use`):
- Class factory remains registered and can serve multiple instances
- Process waits for the timeout period after all instances are released
- Allows subsequent clients to connect during the timeout window without restarting
- The host tries to re-register the original InProc to prevent self-instantiation (explicit alias recommended)

Example with explicit alias:
```bash
axhost --clsid "{SOURCE-CLSID}/{ALIAS-CLSID}" --multiple-use
```

Note: Explicit `REGCLS` values in the `--clsid` format override these flags.

## Technical Notes

* Built on **Qt** (for GUI control hosting) and **CLI11** (for argument parsing).
* All COM interactions use standard **STA model** (`COINIT_APARTMENTTHREADED`).
* `CoAddRefServerProcess()` / `CoReleaseServerProcess()` are used for lifecycle control.

## Lifetime Behavior

### Surrogate Mode

In Surrogate Mode (no command-line arguments):

* `axhost` registers itself via `CoRegisterSurrogate`
* The COM runtime (SCM/RPCSS) manages class factory registration automatically
* Process lifetime is controlled by COM reference counting and timeout protection
* If no COM interaction occurs within 60 seconds of launch, the process exits with a warning

### Single-Use Mode (Default in Standalone Mode)

In single-use mode (`--single-use` or default), class factories are registered with **REGCLS_SINGLEUSE**:

* After the first client connects, the class factory is removed from public view ([Microsoft Learn][1])
* When the server's reference count drops to zero via `CoReleaseServerProcess`, the process exits immediately
* Subsequent activation requests require launching a new host process

### Multiple-Use Mode

In multiple-use mode (`--multiple-use`), class factories are registered with **REGCLS_MULTIPLEUSE**:

* The class factory remains registered and can serve multiple client connections
* When the server's reference count drops to zero, the process waits for the timeout period (configurable via `--timeout`)
* During the timeout window, new clients can connect without restarting the host
* If no new connections occur within the timeout period, the process exits gracefully

When `CoReleaseServerProcess` returns zero (no active references), OLE automatically calls `CoSuspendClassObjects`, preventing further activations until the timeout-based exit check determines the final action. ([Microsoft Learn][2])

[1]: https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/ne-combaseapi-regcls "REGCLS (combaseapi.h) - Win32 apps | Microsoft Learn"
[2]: https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coreleaseserverprocess "CoReleaseServerProcess function (combaseapi.h)"

## Error Handling

* Initialization, registration, and class loading errors are displayed via `QMessageBox` dialogs.
* If all class registrations fail, the process terminates automatically.
* When the timeout period elapses with no initial COM interaction, a warning dialog is shown (`QMessageBox::warning`), and the process exits gracefully.

## Example Use Case

```python
import time
import uuid
from subprocess import Popen

from comtypes import CLSCTX_LOCAL_SERVER
from comtypes.client import CreateObject

source = "{0002DF01-0000-0000-C000-000000000046}"
alias = "{" + str(uuid.uuid4()).upper() + "}"

axhost_commands = [
    "axhost.exe",
    "--clsid",
    f"{source}/{alias}/0x4",
]

with Popen(
    axhost_commands,
) as process:
    time.sleep(1)  # wait for registration
    obj = CreateObject(alias, CLSCTX_LOCAL_SERVER)  # create object
    obj.Visible = True  # use object
    obj.Release()  # release object
```

## Building from Source

### Install Tools for Building Project

#### Install MSVC BuildTools

https://learn.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio?view=vs-2022

```
vs_buildtools.exe
    --add "Microsoft.VisualStudio.Workload.VCTools"
    --add "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
    --includeRecommended
    --passive
    --norestart
```

#### Install Chocolatey

https://chocolatey.org/install

```
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

#### Install GIt

https://git-scm.com/

```
choco install git.install
```

#### Install Python

https://www.python.org/

```
choco install python311
```

#### Install CMake

https://cmake.org/

```
choco install cmake
```

#### Install Ninja

https://ninja-build.org/

```
choco install ninja
```

### Build Project using CMake

#### List Configure Presets

```
cmake --list-presets
```

#### Configure

```
cmake --preset amd64
```

#### Build

```
cmake --build --preset amd64-release
```

Note that you should build `${host_arch}-release` version at first in order to support the tools for the later cross-compling.

## License

Licensed under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)

SPDX-License-Identifier: Apache-2.0

© 2025 Yunseong Hwang
