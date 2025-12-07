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

set(CMAKE_CXX_STANDARD 20)
option(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTING "Build tests" OFF)
option(BUILD_SAMPLES "Build samples" OFF)

option(AXHOST_SUPERBUILD "Build with all dependencies" OFF)
set(AXHOST_OUTPUT_NAME axhost)

if (AXHOST_SUPERBUILD)
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/super-build.cmake")
    return()
endif()

project(axhost VERSION 0.1.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets AxContainer Concurrent LinguistTools)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(WIL CONFIG REQUIRED)

get_target_property(QT6_AUTOMOC_EXECUTABLE Qt6::moc IMPORTED_LOCATION)
get_target_property(QT6_AUTOUIC_EXECUTABLE Qt6::uic IMPORTED_LOCATION)
get_target_property(QT6_AUTORCC_EXECUTABLE Qt6::rcc IMPORTED_LOCATION)

if(NOT QT6_AUTOMOC_EXECUTABLE)
    find_program(QT6_AUTOMOC_EXECUTABLE REQUIRED NAMES moc)
    set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION "${QT6_AUTOMOC_EXECUTABLE}")
endif()
if(NOT QT6_AUTOUIC_EXECUTABLE)
    find_program(QT6_AUTOUIC_EXECUTABLE REQUIRED NAMES uic)
    set_target_properties(Qt6::uic PROPERTIES IMPORTED_LOCATION "${QT6_AUTOUIC_EXECUTABLE}")
endif()
if(NOT QT6_AUTORCC_EXECUTABLE)
    find_program(QT6_AUTORCC_EXECUTABLE REQUIRED NAMES rcc)
    set_target_properties(Qt6::rcc PROPERTIES IMPORTED_LOCATION "${QT6_AUTORCC_EXECUTABLE}")
endif()

get_target_property(QT6_LUPDATE_EXECUTABLE Qt6::lupdate IMPORTED_LOCATION)
get_target_property(QT6_LRELEASE_EXECUTABLE Qt6::lrelease IMPORTED_LOCATION)
get_target_property(QT6_WINDEPLOYQT_EXECUTABLE Qt6::windeployqt IMPORTED_LOCATION)

if(NOT QT6_LUPDATE_EXECUTABLE)
    find_program(QT6_LUPDATE_EXECUTABLE REQUIRED NAMES lupdate)
    set_target_properties(Qt6::lupdate PROPERTIES IMPORTED_LOCATION "${QT6_LUPDATE_EXECUTABLE}")
endif()
if(NOT QT6_LRELEASE_EXECUTABLE)
    find_program(QT6_LRELEASE_EXECUTABLE REQUIRED NAMES lrelease)
    set_target_properties(Qt6::lrelease PROPERTIES IMPORTED_LOCATION "${QT6_LRELEASE_EXECUTABLE}")
endif()
if(NOT QT6_WINDEPLOYQT_EXECUTABLE)
    find_program(QT6_WINDEPLOYQT_EXECUTABLE REQUIRED NAMES windeployqt)
    set_target_properties(Qt6::windeployqt PROPERTIES IMPORTED_LOCATION "${QT6_WINDEPLOYQT_EXECUTABLE}")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

file(GLOB_RECURSE PROJECT_CXX_FILES
    "${CMAKE_CURRENT_LIST_DIR}/src/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cc"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.h.in"
)

qt_add_executable(axhost_executable MANUAL_FINALIZATION "${PROJECT_CXX_FILES}")
target_include_directories(axhost_executable PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(axhost_executable PRIVATE Qt6::Widgets Qt6::AxContainer Qt6::Concurrent CLI11::CLI11 spdlog::spdlog WIL::WIL)
set_target_properties(axhost_executable PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "${AXHOST_OUTPUT_NAME}"
)
qt_finalize_target(axhost_executable)

install(IMPORTED_RUNTIME_ARTIFACTS axhost_executable
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
