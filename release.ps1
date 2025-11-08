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

$dist = "dist"

New-Item -ItemType Directory -Path "$dist" -Force

cmake --preset amd64
cmake --build --preset amd64-release

Copy-Item ".\build\amd64\axhost-release\bin\axhost.exe" ".\$dist\axhost-amd64.exe"

cmake --preset x86
cmake --build --preset x86-release

Copy-Item ".\build\x86\axhost-release\bin\axhost.exe" ".\$dist\axhost-x86.exe"

$version = "VERSION"

$ver = (Get-Content "$version" | Out-String).Trim()

$tag = "v$ver"
$title = "$tag"

git rev-parse "$tag" 2>$null

if ($LASTEXITCODE -ne 0) {
    git tag -a "$tag" -m "$title"
    git push origin "$tag"
}

$notes = "NOTES.md"

if (-not (Test-Path $notes)) {
    $prev = git describe --tags --abbrev=0 --match "v*" --exclude="$tag" 2>$null
    $lines = @("## Changes")
    if ($LASTEXITCODE -eq 0 -and $prev) {
        $logs = git log --pretty="* %s (%h) by %an" "$prev..$tag"
    }
    else {
        $logs = git log --pretty="* %s (%h) by %an" "$tag"
    }
    $lines += $logs
    $lines | Set-Content $notes -Encoding UTF8
}

$files = @("$dist\*.exe")
$flags = @("--draft")

$repo = "elbakramer/axhost"

gh release create "$tag" @files --title "$title" -F "$notes" @flags --repo "$repo"
