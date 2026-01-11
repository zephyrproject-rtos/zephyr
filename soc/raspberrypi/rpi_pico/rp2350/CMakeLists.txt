# Copyright (c) 2021 Yonatan Schachter
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_RISCV vector.S)

zephyr_include_directories(.)

# Add the image definition block if required
zephyr_linker_sources(ROM_START rom_start.ld)

# Conditionally include a linker script.
if(CONFIG_ARM)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
else()
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
endif()
