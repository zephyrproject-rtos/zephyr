# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

include(extensions)

# Find the include-what-you-use executable.
find_program(IWYU_EXE NAMES include-what-you-use iwyu REQUIRED)
message(STATUS "Found SCA: include-what-you-use (${IWYU_EXE})")

# Get include-what-you-use user options. IWYU_OPTS is a semicolon separated
# list of options passed to the include-what-you-use tool itself. Each option
# is forwarded through the compiler driver using the '-Xiwyu' prefix as
# required by the CMAKE_<LANG>_INCLUDE_WHAT_YOU_USE handling.
zephyr_get(IWYU_OPTS)

set(iwyu_command ${IWYU_EXE})
if(DEFINED IWYU_OPTS)
  foreach(iwyu_option IN LISTS IWYU_OPTS)
    list(APPEND iwyu_command -Xiwyu ${iwyu_option})
  endforeach()
endif()

# Enable include-what-you-use using the native CMake support. CMake invokes the
# tool alongside the compiler for every translation unit and prints the
# suggested include changes to stderr during the build.
set(CMAKE_C_INCLUDE_WHAT_YOU_USE ${iwyu_command} CACHE INTERNAL "")
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${iwyu_command} CACHE INTERNAL "")
