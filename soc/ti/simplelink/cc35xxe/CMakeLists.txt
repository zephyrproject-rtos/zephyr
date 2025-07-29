# Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)

set_target_properties(zephyr_property_target PROPERTIES SIGNING_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/toolbox.cmake)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
zephyr_linker_sources(RAMFUNC_SECTION SORT_KEY 0 ramfunc_linker.ld)

# Add custom linker section to support shared memory with the RF core.
zephyr_linker_sources(
  SECTIONS
  wlan.ld
)
