# SPDX-License-Identifier: Apache-2.0

zephyr_get(ANDES_TOOLCHAIN_PATH)
assert(ANDES_TOOLCHAIN_PATH "ANDES_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${ANDES_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at ANDES_TOOLCHAIN_PATH: '${ANDES_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${ANDES_TOOLCHAIN_PATH})

find_program(ANDES_GCC NAMES riscv32-elf-gcc riscv64-elf-gcc
  PATHS ${TOOLCHAIN_HOME}/bin NO_DEFAULT_PATH NO_CACHE
)
if(NOT ANDES_GCC)
  message(FATAL_ERROR "No riscv32-elf-gcc or riscv64-elf-gcc in '${TOOLCHAIN_HOME}/bin'")
endif()

get_filename_component(triple ${ANDES_GCC} NAME_WE)
string(REGEX REPLACE "-gcc$" "" triple ${triple})

set(CROSS_COMPILE_TARGET ${triple})
set(SYSROOT_TARGET       ${triple})

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

# The predefined macros describe the target and ABI the toolchain is built for.
execute_process(
  COMMAND ${ANDES_GCC} -dM -E -x c ${ZEPHYR_BASE}/misc/empty_file.c
  OUTPUT_VARIABLE ANDES_TOOLCHAIN_MACROS
)

if(NOT ANDES_TOOLCHAIN_MACROS MATCHES "#define __ELF__ " OR
   ANDES_TOOLCHAIN_MACROS MATCHES "#define __linux__ ")
  message(FATAL_ERROR "'${ANDES_TOOLCHAIN_PATH}' is not a bare-metal ELF toolchain")
endif()

# Only newlib provides _newlib_version.h, mculib does not.
if(EXISTS ${SYSROOT_DIR}/include/_newlib_version.h)
  set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
endif()
