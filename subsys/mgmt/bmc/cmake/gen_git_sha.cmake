# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
#
# Generates bmc_git_sha.h at build time so the SHA reflects the current commit.

if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} -C ${SOURCE_DIR} rev-parse --short HEAD
    OUTPUT_VARIABLE sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
endif()

if(NOT sha OR sha STREQUAL "")
  set(sha "unknown")
endif()

set(content "#pragma once\n#define PROJECT_GIT_SHA \"${sha}\"\n")
if(EXISTS "${OUTPUT_FILE}")
  file(READ "${OUTPUT_FILE}" existing)
  if(existing STREQUAL content)
    return()
  endif()
endif()

file(WRITE "${OUTPUT_FILE}" "${content}")
