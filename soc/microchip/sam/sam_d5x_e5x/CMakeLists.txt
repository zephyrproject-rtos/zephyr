# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_include_directories(common)
zephyr_include_directories(${CONFIG_SOC_SERIES})

add_subdirectory(${ZEPHYR_BASE}/soc/microchip/common
                 ${CMAKE_CURRENT_BINARY_DIR}/mchp_common)
