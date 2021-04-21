# SPDX-License-Identifier: Apache-2.0

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER clang ${find_program_clang_args})

