# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as GNU binutils

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_binutils_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
  set(find_program_gdb_multiarch_args PATHS ${TOOLCHAIN_HOME})
endif()

find_program(CMAKE_OBJCOPY ${CROSS_COMPILE}objcopy ${find_program_binutils_args})
find_program(CMAKE_OBJDUMP ${CROSS_COMPILE}objdump ${find_program_binutils_args})
find_program(CMAKE_AS      ${CROSS_COMPILE}as      ${find_program_binutils_args})
find_program(CMAKE_AR      ${CROSS_COMPILE}ar      ${find_program_binutils_args})
find_program(CMAKE_RANLIB  ${CROSS_COMPILE}ranlib  ${find_program_binutils_args})
find_program(CMAKE_READELF ${CROSS_COMPILE}readelf ${find_program_binutils_args})
find_program(CMAKE_NM      ${CROSS_COMPILE}nm      ${find_program_binutils_args})

find_program(CMAKE_GDB     ${CROSS_COMPILE}gdb     ${find_program_binutils_args})
find_program(CMAKE_GDB     gdb-multiarch           ${find_program_gdb_multiarch_args})

# Include bin tool abstraction macros
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_memusage.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_objcopy.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_objdump.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_readelf.cmake)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_strip.cmake)
