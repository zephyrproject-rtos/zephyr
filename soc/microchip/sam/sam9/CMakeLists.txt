# Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(${SOC_SERIES})

add_subdirectory(../common ${CMAKE_BINARY_DIR}/common)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/arm9/scripts/linker.ld CACHE INTERNAL "")
