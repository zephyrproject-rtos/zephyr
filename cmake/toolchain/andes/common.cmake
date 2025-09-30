# SPDX-License-Identifier: Apache-2.0

zephyr_get(ANDES_TOOLCHAIN_PATH)
assert(ANDES_TOOLCHAIN_PATH "ANDES_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${ANDES_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at ANDES_TOOLCHAIN_PATH: '${ANDES_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${ANDES_TOOLCHAIN_PATH})

cmake_path(GET ANDES_TOOLCHAIN_PATH FILENAME TOOLCHAIN_BASENAME)

if(TOOLCHAIN_BASENAME MATCHES "nds64le-")
  set(triple riscv64-elf)
elseif(TOOLCHAIN_BASENAME MATCHES "nds32le-")
  set(triple riscv32-elf)
else()
  message(FATAL_ERROR "Unknown Andes toolchain name: '${TOOLCHAIN_BASENAME}'")
endif()

set(CROSS_COMPILE_TARGET ${triple})
set(SYSROOT_TARGET       ${triple})

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

# Andes toolchain naming rule:
#   nds[32|64]-[elf|linux]-[mculib|newlib|glibc]-[v5e|v5|v5f|v5d]
# The kernel configuration must match the toolchain for ABI
cmake_path(GET ANDES_TOOLCHAIN_PATH FILENAME TOOLCHAIN_BASENAME)

string(REPLACE "-" ";" TOOLCHAIN_PARTS "${TOOLCHAIN_BASENAME}")
list(GET TOOLCHAIN_PARTS 1 TOOLCHAIN_ENV)   # elf / linux
list(GET TOOLCHAIN_PARTS 2 TOOLCHAIN_LIBC)  # mculib / newlib / glibc

# Check toolchain environment
if(NOT TOOLCHAIN_ENV STREQUAL "elf")
  message(FATAL_ERROR "'${TOOLCHAIN_BASENAME}' uses '${TOOLCHAIN_ENV}', only 'elf' is supported")
endif()

# Check toolchain libc
if(TOOLCHAIN_LIBC STREQUAL "newlib")
  set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
endif()
