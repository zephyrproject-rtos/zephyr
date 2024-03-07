# Copyright (c) 2023 IoT.bzh
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
