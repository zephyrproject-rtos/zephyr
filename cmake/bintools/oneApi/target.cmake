# SPDX-License-Identifier: Apache-2.0

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} ${ONEAPI_PYTHON_PATH} NO_DEFAULT_PATH)
  set(find_program_binutils_args PATHS ${TOOLCHAIN_HOME} )
endif()

find_program(CMAKE_AR      llvm-ar      ${find_program_clang_args}   )
find_program(CMAKE_NM      llvm-nm      ${find_program_clang_args}   )
find_program(CMAKE_OBJDUMP llvm-objdump ${find_program_clang_args}   )
find_program(CMAKE_RANLIB  llvm-ranlib  ${find_program_clang_args}   )
find_program(CMAKE_OBJCOPY llvm-objcopy ${find_program_binutils_args})
find_program(CMAKE_READELF readelf      ${find_program_binutils_args})
find_program(CMAKE_STRIP   llvm-strip   ${find_program_binutils_args})

find_program(CMAKE_GDB     gdb-oneapi)

# Use the gnu binutil abstraction
include(${ZEPHYR_BASE}/cmake/bintools/llvm/target_bintools.cmake)

