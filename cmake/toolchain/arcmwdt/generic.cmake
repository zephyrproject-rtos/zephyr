# SPDX-License-Identifier: Apache-2.0

zephyr_get(ARCMWDT_TOOLCHAIN_PATH)

set(METAWARE_ROOT $ENV{METAWARE_ROOT})
if(NOT DEFINED ARCMWDT_TOOLCHAIN_PATH AND DEFINED METAWARE_ROOT)
  message(STATUS "ARCMWDT_TOOLCHAIN_PATH is not set, use default toolchain from METAWARE_ROOT: '${METAWARE_ROOT}'")
  if(NOT EXISTS ${METAWARE_ROOT})
    message(FATAL_ERROR "Nothing found at METAWARE_ROOT: '${METAWARE_ROOT}'")
  endif()
  cmake_path(GET METAWARE_ROOT PARENT_PATH ARCMWDT_TOOLCHAIN_PATH)
elseif(NOT DEFINED ARCMWDT_TOOLCHAIN_PATH AND NOT DEFINED METAWARE_ROOT)
  message(FATAL_ERROR "No MWDT toolchain specified: neither ARCMWDT_TOOLCHAIN_PATH nor METAWARE_ROOT defined")
endif()

if(NOT EXISTS ${ARCMWDT_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at ARCMWDT_TOOLCHAIN_PATH: '${ARCMWDT_TOOLCHAIN_PATH}'")
endif()

# arcmwdt relies on Zephyr SDK for the use of C preprocessor (devicetree) and objcopy
find_package(Zephyr-sdk 0.15 REQUIRED)
# Handling to be improved in Zephyr SDK, we can drop setting TOOLCHAIN_HOME after
# https://github.com/zephyrproject-rtos/sdk-ng/pull/682 got merged and we switch to proper SDK
# version.
set(TOOLCHAIN_HOME ${ZEPHYR_SDK_INSTALL_DIR})
include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/target.cmake)
set(ZEPHYR_SDK_CROSS_COMPILE ${CROSS_COMPILE})
# Handling to be improved in Zephyr SDK, to avoid overriding ZEPHYR_TOOLCHAIN_VARIANT by
# find_package(Zephyr-sdk) if it's already set
set(ZEPHYR_TOOLCHAIN_VARIANT arcmwdt)

set(TOOLCHAIN_HOME ${ARCMWDT_TOOLCHAIN_PATH}/MetaWare)

set(COMPILER arcmwdt)
set(LINKER arcmwdt)
set(BINTOOLS arcmwdt)

set(SYSROOT_TARGET arc)

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/arc/bin/)
set(SYSROOT_DIR ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
