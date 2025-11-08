# Copyright 2025 Yunseong Hwang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-FileCopyrightText: 2025 Yunseong Hwang
#
# SPDX-License-Identifier: Apache-2.0

[CmdletBinding(DefaultParameterSetName = "Launch")]
param (
    [Parameter(ParameterSetName = "Configure")]
    [switch]
    $Configure,

    [Parameter(ParameterSetName = "Launch")]
    [switch]
    $Launch,

    [Parameter(ParameterSetName = "Configure")]
    [Parameter(ParameterSetName = "Launch")]
    [ValidateSet('x86', 'amd64', 'arm', 'arm64')]
    [string]
    $Arch,

    [Parameter(ParameterSetName = "Configure")]
    [Parameter(ParameterSetName = "Launch")]
    [ValidateSet('x86', 'amd64')]
    [string]
    $HostArch,

    [Parameter(ParameterSetName = "Configure")]
    [Parameter(ParameterSetName = "Launch")]
    [string]
    $File,

    [Parameter(ParameterSetName = "Launch", ValueFromRemainingArguments = $true)]
    [string[]]
    $Command
)

function Configure {
    $devShellPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1"

    $names = @(
        '__DOTNET_ADD_32BIT',
        '__DOTNET_PREFERRED_BITNESS',
        '__VSCMD_PREINIT_PATH',
        'CommandPromptType',
        'DevEnvDir',
        'ExtensionSdkDir',
        'EXTERNAL_INCLUDE',
        'Framework40Version',
        'FrameworkDir',
        'FrameworkDir32',
        'FrameworkVersion',
        'FrameworkVersion32',
        'INCLUDE',
        'LIB',
        'LIBPATH',
        'PATH',
        'PROMPT',
        'UCRTVersion',
        'UniversalCRTSdkDir',
        'VCIDEInstallDir',
        'VCINSTALLDIR',
        'VCPKG_ROOT',
        'VCToolsInstallDir',
        'VCToolsRedistDir',
        'VCToolsVersion',
        'VisualStudioVersion',
        'VS170COMNTOOLS',
        'VSCMD_ARG_app_plat',
        'VSCMD_ARG_HOST_ARCH',
        'VSCMD_ARG_TGT_ARCH',
        'VSCMD_VER',
        'VSINSTALLDIR',
        'WindowsLibPath',
        'WindowsSdkBinPath',
        'WindowsSdkDir',
        'WindowsSDKLibVersion',
        'WindowsSdkVerBinPath',
        'WindowsSDKVersion'
    )

    $params = @{
        SkipAutomaticLocation = $true
    }

    if ($Arch) {
        $params.Arch = $Arch
    }

    if ($HostArch) {
        $params.HostArch = $HostArch
    }

    & $devShellPath @params

    if ($File) {
        Get-ChildItem Env: |
        Where-Object { $names -contains $_.Name } |
        Select-Object Name, Value |
        ConvertTo-Json |
        Out-File $File -Encoding UTF8
    }
}

function Launch {
    if ($File) {
        $envs = @(Get-Content $File | ConvertFrom-Json)

        foreach ($item in $envs) {
            Set-Item "Env:$($item.Name)" $item.Value
        }
    }
    else {
        & (Get-ChildItem "Function:Configure")
    }

    if ($Command.Count -gt 0) {
        & $Command[0] @($Command[1..($Command.Count - 1)])
        exit $LASTEXITCODE
    }
}

if ($PSCmdlet.ParameterSetName) {
    & (Get-ChildItem "Function:$($PSCmdlet.ParameterSetName)")
    exit
}
