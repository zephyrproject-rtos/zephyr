# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_get(RH850_TOOLCHAIN_PATH)
if(NOT RH850_TOOLCHAIN_PATH)
  set(RH850_TOOLCHAIN_PATH "/opt/toolchains/v850-elf")
endif()
set(CROSS_COMPILE ${RH850_TOOLCHAIN_PATH}/bin/v850-elf- CACHE FILEPATH "" FORCE)
include(${ZEPHYR_BASE}/cmake/toolchain/cross-compile/generic.cmake)
set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_BASE}/cmake/toolchain/cross-compile)
