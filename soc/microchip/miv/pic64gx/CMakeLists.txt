# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

if(CONFIG_OPENAMP_RSC_TABLE)
  zephyr_linker_sources(SECTIONS ${CMAKE_CURRENT_SOURCE_DIR}/resource_table.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
