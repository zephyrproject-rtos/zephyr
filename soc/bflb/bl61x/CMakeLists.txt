# Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

if(CONFIG_BT_BFLB OR CONFIG_WIFI_BFLB)
  zephyr_sources(bflb_rf.c)
endif()
