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
include(ExternalProject)

function(add_external_project EXTERNAL_NAME)
    set(OPTIONS)
    set(ONE_VALUE_KEYWORDS
        SOURCE_DIR
        PREFIX
        BINARY_DIR
        INSTALL_DIR
        TMP_DIR
        STAMP_DIR
        LOG_DIR
    )
    set(MULTI_VALUE_KEYWORDS
        CMAKE_ARGS
        CMAKE_CACHE_ARGS
        DEPENDS
    )
    cmake_parse_arguments(EXTERNAL
        "${OPTIONS}"
        "${ONE_VALUE_KEYWORDS}"
        "${MULTI_VALUE_KEYWORDS}"
        ${ARGN}
    )

    if (NOT EXTERNAL_SOURCE_DIR)
        if(EXTERNAL_NAME STREQUAL PROJECT_NAME)
            set(EXTERNAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        else()
            set(EXTERNAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/${EXTERNAL_NAME}")
        endif()
    endif()

    if (NOT EXTERNAL_PREFIX)
        set(EXTERNAL_PREFIX       "${CMAKE_CURRENT_BINARY_DIR}/${EXTERNAL_NAME}")
    endif()
    if (NOT EXTERNAL_BINARY_DIR)
        set(EXTERNAL_BINARY_DIR   "${CMAKE_CURRENT_BINARY_DIR}/${EXTERNAL_NAME}-build")
    endif()
    if (NOT EXTERNAL_INSTALL_DIR)
        set(EXTERNAL_INSTALL_DIR  "${EXTERNAL_PREFIX}")
    endif()
    if (NOT EXTERNAL_TMP_DIR)
        set(EXTERNAL_TMP_DIR      "${EXTERNAL_PREFIX}/tmp")
    endif()
    if (NOT EXTERNAL_STAMP_DIR)
        set(EXTERNAL_STAMP_DIR    "${EXTERNAL_TMP_DIR}/stamp")
    endif()
    if (NOT EXTERNAL_LOG_DIR)
        set(EXTERNAL_LOG_DIR      "${EXTERNAL_TMP_DIR}/log")
    endif()
    if (NOT EXTERNAL_DOWNLOAD_DIR)
        set(EXTERNAL_DOWNLOAD_DIR "${EXTERNAL_TMP_DIR}/download")
    endif()

    if(NOT EXTERNAL_CMAKE_COMMAND)
        set(EXTERNAL_CMAKE_COMMAND "${CMAKE_COMMAND}")
    endif()

    set(EXTERNAL_CMAKE_ARGS
        "-DCMAKE_SYSTEM_PROCESSOR:STRING=${CMAKE_SYSTEM_PROCESSOR}"
        -P "${CMAKE_CURRENT_LIST_DIR}/toolchain-msvc.cmake"
        -- "${EXTERNAL_CMAKE_COMMAND}" ${EXTERNAL_CMAKE_ARGS}
    )

    set(EXTERNAL_CMAKE_CACHE_ARGS
        "-DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}"
        "-DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}"
        "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
        ${EXTERNAL_CMAKE_CACHE_ARGS}
    )

    if(NOT EXTERNAL_CMAKE_GENERATOR)
        set(EXTERNAL_CMAKE_GENERATOR "${CMAKE_GENERATOR}")
    endif()

    foreach(EXTERNAL_DEPEND_NAME IN LISTS EXTERNAL_DEPENDS)
        get_property(EXTERNAL_DEPEND_PREFIX_PATH_FROM_DEPEND  TARGET "${EXTERNAL_DEPEND_NAME}" PROPERTY PREFIX_PATH)
        get_property(EXTERNAL_DEPEND_PROGRAM_PATH_FROM_DEPEND TARGET "${EXTERNAL_DEPEND_NAME}" PROPERTY PROGRAM_PATH)
        if(EXTERNAL_DEPEND_PREFIX_PATH_FROM_DEPEND)
            foreach(EXTERNAL_DEPEND_PREFIX_PATH_ITEM IN LISTS EXTERNAL_DEPEND_PREFIX_PATH_FROM_DEPEND)
                list(APPEND EXTERNAL_DEPEND_PREFIX_PATH "${EXTERNAL_DEPEND_PREFIX_PATH_ITEM}")
            endforeach()
        endif()
        if(EXTERNAL_DEPEND_PROGRAM_PATH_FROM_DEPEND)
            foreach(EXTERNAL_DEPEND_PROGRAM_PATH_ITEM IN LISTS EXTERNAL_DEPEND_PROGRAM_PATH_FROM_DEPEND)
                list(APPEND EXTERNAL_DEPEND_PROGRAM_PATH "${EXTERNAL_DEPEND_PROGRAM_PATH_ITEM}")
            endforeach()
        endif()
    endforeach()

    list(REMOVE_DUPLICATES EXTERNAL_DEPEND_PREFIX_PATH)
    list(REMOVE_DUPLICATES EXTERNAL_DEPEND_PROGRAM_PATH)

    if(EXTERNAL_DEPEND_PREFIX_PATH)
        list(JOIN EXTERNAL_DEPEND_PREFIX_PATH ";" EXTERNAL_DEPEND_PREFIX_PATH_JOINED)
        list(APPEND EXTERNAL_CMAKE_CACHE_ARGS "-DCMAKE_PREFIX_PATH:PATH=${EXTERNAL_DEPEND_PREFIX_PATH_JOINED}")
    endif()
    if(EXTERNAL_DEPEND_PROGRAM_PATH)
        list(JOIN EXTERNAL_DEPEND_PROGRAM_PATH ";" EXTERNAL_DEPEND_PROGRAM_PATH_JOINED)
        list(APPEND EXTERNAL_CMAKE_CACHE_ARGS "-DCMAKE_PROGRAM_PATH:PATH=${EXTERNAL_DEPEND_PROGRAM_PATH_JOINED}")
    endif()

    set(EXTERNAL_CONFIGURE_COMMAND
        "${EXTERNAL_CMAKE_COMMAND}"
        ${EXTERNAL_CMAKE_ARGS}
        "-G" "${EXTERNAL_CMAKE_GENERATOR}"
        "-S" "<SOURCE_DIR><SOURCE_SUBDIR>"
        "-B" "<BINARY_DIR>"
        "-U" "*_DIR"
        "-U" "*_LIBRARY"
        "-U" "*_LIBRARY_*"
    )

    set(EXTERNAL_BUILD_COMMAND
        "${EXTERNAL_CMAKE_COMMAND}"
        ${EXTERNAL_CMAKE_ARGS}
        "--build" "<BINARY_DIR>"
        "$<$<BOOL:$<CONFIG>>:--config>" "$<CONFIG>"
    )

    set(EXTERNAL_INSTALL_COMMAND
        "${EXTERNAL_CMAKE_COMMAND}"
        "--install" "<BINARY_DIR>"
        "--prefix" "<INSTALL_DIR>$<$<BOOL:$<CONFIG>>:-$<LOWER_CASE:$<CONFIG>>>"
        "$<$<BOOL:$<CONFIG>>:--config>" "$<CONFIG>"
    )

    if(EXISTS "${EXTERNAL_SOURCE_DIR}/.git")
        execute_process(
            COMMAND git -C "${EXTERNAL_SOURCE_DIR}" tag --points-at HEAD
            OUTPUT_VARIABLE EXTERNAL_GIT_TAG_CHECKED
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    if(NOT EXTERNAL_GIT_TAG_CHECKED)
        set(EXTERNAL_SHOULD_UPDATE TRUE)
    elseif("${EXTERNAL_GIT_TAG_CHECKED}" MATCHES "${EXTERNAL_GIT_TAG}")
        set(EXTERNAL_SHOULD_UPDATE FALSE)
    else()
        set(EXTERNAL_SHOULD_UPDATE TRUE)
    endif()

    set(HAS_CMAKE_CACHE_ARGS 0)
    if(NOT "${EXTERNAL_CMAKE_CACHE_ARGS}" STREQUAL "")
        set(HAS_CMAKE_CACHE_ARGS 1)
    endif()

    set(HAS_CMAKE_CACHE_DEFAULT_ARGS 0)
    if(NOT "${EXTERNAL_CMAKE_CACHE_DEFAULT_ARGS}" STREQUAL "")
        set(HAS_CMAKE_CACHE_DEFAULT_ARGS 1)
    endif()

    if(HAS_CMAKE_CACHE_ARGS OR HAS_CMAKE_CACHE_DEFAULT_ARGS)
        set(EXTERNAL_CACHE_SCRIPT "<TMP_DIR>/${EXTERNAL_NAME}-cache-$<CONFIG>.cmake")
        list(APPEND EXTERNAL_CONFIGURE_COMMAND "-C" "${EXTERNAL_CACHE_SCRIPT}")
    endif()

    if (EXTERNAL_SHOULD_UPDATE)
        ExternalProject_Add("${EXTERNAL_NAME}"
            PREFIX "${EXTERNAL_PREFIX}"
            TMP_DIR "${EXTERNAL_TMP_DIR}"
            STAMP_DIR "${EXTERNAL_STAMP_DIR}"
            LOG_DIR "${EXTERNAL_LOG_DIR}"
            DOWNLOAD_DIR "${EXTERNAL_DOWNLOAD_DIR}"
            SOURCE_DIR "${EXTERNAL_SOURCE_DIR}"
            BINARY_DIR "${EXTERNAL_BINARY_DIR}"
            INSTALL_DIR "${EXTERNAL_INSTALL_DIR}"
            CMAKE_COMMAND "${EXTERNAL_CMAKE_COMMAND}"
            CMAKE_ARGS ${EXTERNAL_CMAKE_ARGS}
            CMAKE_CACHE_ARGS ${EXTERNAL_CMAKE_CACHE_ARGS}
            CMAKE_GENERATOR "${EXTERNAL_CMAKE_GENERATOR}"
            CONFIGURE_COMMAND ${EXTERNAL_CONFIGURE_COMMAND}
            BUILD_COMMAND ${EXTERNAL_BUILD_COMMAND}
            INSTALL_COMMAND ${EXTERNAL_INSTALL_COMMAND}
            DEPENDS ${EXTERNAL_DEPENDS}
            ${EXTERNAL_UNPARSED_ARGUMENTS}
        )
    else()
        ExternalProject_Add("${EXTERNAL_NAME}"
            PREFIX "${EXTERNAL_PREFIX}"
            TMP_DIR "${EXTERNAL_TMP_DIR}"
            STAMP_DIR "${EXTERNAL_STAMP_DIR}"
            LOG_DIR "${EXTERNAL_LOG_DIR}"
            DOWNLOAD_DIR "${EXTERNAL_DOWNLOAD_DIR}"
            SOURCE_DIR "${EXTERNAL_SOURCE_DIR}"
            BINARY_DIR "${EXTERNAL_BINARY_DIR}"
            INSTALL_DIR "${EXTERNAL_INSTALL_DIR}"
            CMAKE_COMMAND "${EXTERNAL_CMAKE_COMMAND}"
            CMAKE_ARGS ${EXTERNAL_CMAKE_ARGS}
            CMAKE_CACHE_ARGS ${EXTERNAL_CMAKE_CACHE_ARGS}
            CMAKE_GENERATOR "${EXTERNAL_CMAKE_GENERATOR}"
            CONFIGURE_COMMAND ${EXTERNAL_CONFIGURE_COMMAND}
            BUILD_COMMAND ${EXTERNAL_BUILD_COMMAND}
            INSTALL_COMMAND ${EXTERNAL_INSTALL_COMMAND}
            DEPENDS ${EXTERNAL_DEPENDS}
            DOWNLOAD_COMMAND ""
            UPDATE_COMMAND ""
            ${EXTERNAL_UNPARSED_ARGUMENTS}
        )
    endif()

    if(HAS_CMAKE_CACHE_ARGS OR HAS_CMAKE_CACHE_DEFAULT_ARGS)
        if(HAS_CMAKE_CACHE_ARGS)
            _ep_command_line_to_initial_cache(
                script_initial_cache_force
                "${EXTERNAL_CMAKE_CACHE_ARGS}"
                1
            )
        endif()
        if(HAS_CMAKE_CACHE_DEFAULT_ARGS)
            _ep_command_line_to_initial_cache(
                script_initial_cache_default
                "${HAS_CMAKE_CACHE_DEFAULT_ARGS}"
                0
            )
        endif()

        _ep_write_initial_cache(
            "${EXTERNAL_NAME}"
            "${EXTERNAL_CACHE_SCRIPT}"
            "${script_initial_cache_force}${script_initial_cache_default}"
        )
        _ep_replace_location_tags("${EXTERNAL_NAME}" EXTERNAL_CACHE_SCRIPT)

        ExternalProject_Add_StepDependencies("${EXTERNAL_NAME}" "configure" "${EXTERNAL_CACHE_SCRIPT}")
    endif()

    get_property(CMAKE_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(CMAKE_GENERATOR_IS_MULTI_CONFIG)
        list(APPEND EXTERNAL_DEPEND_PREFIX_PATH  "${EXTERNAL_INSTALL_DIR}-$<LOWER_CASE:$<CONFIG>>")
        list(APPEND EXTERNAL_DEPEND_PROGRAM_PATH "${EXTERNAL_INSTALL_DIR}-release/bin")
        list(APPEND EXTERNAL_DEPEND_PROGRAM_PATH "${EXTERNAL_INSTALL_DIR}-$<LOWER_CASE:$<CONFIG>>/bin")
        list(APPEND EXTERNAL_DEPEND_PROGRAM_PATH "${EXTERNAL_INSTALL_DIR}-$<LOWER_CASE:$<CONFIG>>/bin/$<CONFIG>")
    else()
        list(APPEND EXTERNAL_DEPEND_PREFIX_PATH  "${EXTERNAL_INSTALL_DIR}")
        list(APPEND EXTERNAL_DEPEND_PROGRAM_PATH "${EXTERNAL_INSTALL_DIR}/bin")
    endif()

    list(REMOVE_DUPLICATES EXTERNAL_DEPEND_PREFIX_PATH)
    list(REMOVE_DUPLICATES EXTERNAL_DEPEND_PROGRAM_PATH)

    set_property(TARGET "${EXTERNAL_NAME}" PROPERTY PREFIX_PATH  "${EXTERNAL_DEPEND_PREFIX_PATH}")
    set_property(TARGET "${EXTERNAL_NAME}" PROPERTY PROGRAM_PATH "${EXTERNAL_DEPEND_PROGRAM_PATH}")
endfunction()
