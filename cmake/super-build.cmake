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

cmake_minimum_required(VERSION 3.31)

include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/external-project.cmake")

project(axhost VERSION 0.1.0 LANGUAGES CXX)

add_external_project(zlib
    GIT_REPOSITORY "https://github.com/zlib-ng/zlib-ng.git"
    GIT_TAG "2.2.4"
    CMAKE_CACHE_ARGS
        "-DZLIB_ENABLE_TESTS:BOOL=${BUILD_TESTING}"
        "-DZLIB_COMPAT:BOOL=ON"
)

if (BUILD_SHARED_LIBS)
    set(USE_STATIC_LIBS ON)
else()
    set(USE_STATIC_LIBS OFF)
endif()

add_external_project(qt6
    GIT_REPOSITORY "https://code.qt.io/qt/qt5.git"
    GIT_TAG "v6.10.0"
    GIT_SUBMODULES qtbase qtactiveqt qttools qtrepotools
    CMAKE_CACHE_ARGS
        "-DZLIB_USE_STATIC_LIBS:BOOL=${USE_STATIC_LIBS}"
        "-DQT_BUILD_TESTS:BOOL=${BUILD_TESTING}"
        "-DQT_BUILD_EXAMPLES:BOOL=${BUILD_SAMPLES}"
        "-DBUILD_TESTING:BOOL=${BUILD_TESTING}"
        "-DBUILD_qtwebengine:BOOL=OFF"
        "-DFEATURE_system_zlib:BOOL=ON"
        "-DFEATURE_cxx20:BOOL=ON"
    DEPENDS zlib
)

add_external_project(cli11
    GIT_REPOSITORY "https://github.com/CLIUtils/CLI11.git"
    GIT_TAG "v2.6.1"
)

add_external_project(spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG "v1.16.0"
)

add_external_project(axhost
    BUILD_ALWAYS ON
    CMAKE_CACHE_ARGS
        "-DAXHOST_SUPERBUILD:BOOL=OFF"
        "-DAXHOST_OUTPUT_NAME:STRING=${AXHOST_OUTPUT_NAME}"
    DEPENDS qt6 cli11 spdlog
)

ExternalProject_Get_Property(axhost INSTALL_DIR)

add_executable(axhost_executable IMPORTED)
set_target_properties(axhost_executable PROPERTIES
    IMPORTED_LOCATION "${INSTALL_DIR}/bin/${AXHOST_OUTPUT_NAME}.exe"
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "${AXHOST_OUTPUT_NAME}"
)

install(IMPORTED_RUNTIME_ARTIFACTS axhost_executable
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
