# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_get(TRICORE_TOOLCHAIN_PATH)
if(NOT TRICORE_TOOLCHAIN_PATH)
  set(TRICORE_TOOLCHAIN_PATH "/opt/toolchains/tricore-elf")
endif()
set(CROSS_COMPILE ${TRICORE_TOOLCHAIN_PATH}/bin/tricore-elf- CACHE FILEPATH "" FORCE)
include(${ZEPHYR_BASE}/cmake/toolchain/cross-compile/generic.cmake)
set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_BASE}/cmake/toolchain/cross-compile)
