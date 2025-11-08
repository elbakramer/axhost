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

if(NOT CMAKE_HOST_SYSTEM_PROCESSOR)
    cmake_host_system_information(RESULT CMAKE_HOST_SYSTEM_PROCESSOR QUERY OS_PLATFORM)
endif()

set(CMAKE_SYSTEM_NAME Windows CACHE STRING "Target system")
set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "Target processor")

if (NOT DEFINED ENV{VSCMD_VER})
    set(ENV_FILE  "${CMAKE_CURRENT_BINARY_DIR}/Launch-VsDevShell.json")

    if (NOT EXISTS "${ENV_FILE}")
        set(TOOL_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchain-msvc.ps1")
        execute_process(
            COMMAND pwsh -NoProfile -ExecutionPolicy Bypass -File "${TOOL_FILE}" -Configure -Arch "${CMAKE_SYSTEM_PROCESSOR}" -HostArch "${CMAKE_HOST_SYSTEM_PROCESSOR}" -File "${ENV_FILE}"
            OUTPUT_VARIABLE ENV_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    if (EXISTS "${ENV_FILE}")
        file(READ "${ENV_FILE}" ENV_FILE_CONTENT)

        string(JSON ENV_COUNT LENGTH "${ENV_FILE_CONTENT}")
        math(EXPR IDX_END "${ENV_COUNT} - 1")

        foreach(IDX RANGE ${IDX_END})
            string(JSON NAME GET "${ENV_FILE_CONTENT}" ${IDX} Name)
            string(JSON VALUE GET "${ENV_FILE_CONTENT}" ${IDX} Value)
            set(ENV{${NAME}} "${VALUE}")
        endforeach()
    endif()
endif()

set(CMAKE_C_FLAGS_INIT   /source-charset:utf-8)
set(CMAKE_CXX_FLAGS_INIT /source-charset:utf-8)

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    set(_CMAKE_ARGS)
    foreach(_INDEX RANGE "${CMAKE_ARGC}")
        list(APPEND _CMAKE_ARGS "${CMAKE_ARGV${_INDEX}}")
    endforeach()

    set(_CMAKE_UNPARSED_SEP "--")
    list(FIND _CMAKE_ARGS "${_CMAKE_UNPARSED_SEP}" _CMAKE_UNPARSED_SEP_LOC)

    if(_CMAKE_UNPARSED_SEP_LOC LESS 0)
        return()
    endif()

    math(EXPR _CMAKE_UNPARSED_BEGIN "${_CMAKE_UNPARSED_SEP_LOC}+1")
    math(EXPR _CMAKE_UNPARSED_LENGTH "${CMAKE_ARGC}-${_CMAKE_UNPARSED_SEP_LOC}")

    list(SUBLIST _CMAKE_ARGS "${_CMAKE_UNPARSED_BEGIN}" "${_CMAKE_UNPARSED_LENGTH}" _CMAKE_UNPARSED_ARGS)

    if(NOT _CMAKE_UNPARSED_ARGS)
        return()
    endif()

    execute_process(
        COMMAND ${_CMAKE_UNPARSED_ARGS}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
