# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(VERSION_TYPE STREQUAL KERNEL)
  set(BUILD_VERSION_NAME BUILD_VERSION)
else()
  set(BUILD_VERSION_NAME ${VERSION_TYPE}_BUILD_VERSION)
endif()

if(NOT DEFINED ${BUILD_VERSION_NAME})
  cmake_path(GET VERSION_FILE PARENT_PATH work_dir)
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=12 --always
      WORKING_DIRECTORY                ${work_dir}
      OUTPUT_VARIABLE                  ${BUILD_VERSION_NAME}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE                   stderr
      RESULT_VARIABLE                  return_code
    )
    if(return_code)
      message(STATUS "git describe failed: ${stderr}")
    elseif(NOT "${stderr}" STREQUAL "")
      message(STATUS "git describe warned: ${stderr}")
    endif()
  endif()
endif()

include(${ZEPHYR_BASE}/cmake/modules/version.cmake)
file(READ ${ZEPHYR_BASE}/version.h.in version_content)
string(CONFIGURE "${version_content}" version_content)
string(CONFIGURE "${version_content}" version_content)
file(WRITE ${OUT_FILE} "${version_content}")
