# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as llvm binary tool set

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
  set(find_program_binutils_args PATHS ${TOOLCHAIN_HOME})
endif()

# Extract the clang version.  Debian (maybe other distros?) will
# append a version to llvm-objdump/objcopy
execute_process(COMMAND ${CMAKE_C_COMPILER} --version
                OUTPUT_VARIABLE CLANGVER)
string(REGEX REPLACE "[^0-9]*([0-9]+).*" "\\1" CLANGVER ${CLANGVER})

find_program(CMAKE_AR      llvm-ar      ${find_program_clang_args})
find_program(CMAKE_NM      llvm-nm      ${find_program_clang_args})
find_program(CMAKE_OBJDUMP NAMES
                           llvm-objdump
                           llvm-objdump-${CLANGVER}
                           ${find_program_clang_args})
find_program(CMAKE_RANLIB  llvm-ranlib  ${find_program_clang_args})
find_program(CMAKE_STRIP   llvm-strip   ${find_program_clang_args})
find_program(CMAKE_OBJCOPY NAMES
                           llvm-objcopy
                           llvm-objcopy-${CLANGVER}
                           objcopy
                           ${find_program_binutils_args})
find_program(CMAKE_READELF readelf      ${find_program_binutils_args})

# Use the gnu binutil abstraction
include(${ZEPHYR_BASE}/cmake/bintools/llvm/target_bintools.cmake)
