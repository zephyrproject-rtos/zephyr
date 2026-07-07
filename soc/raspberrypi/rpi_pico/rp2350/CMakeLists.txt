# Copyright (c) 2021 Yonatan Schachter
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_RISCV vector.S)
zephyr_library_sources_ifdef(CONFIG_MPU mpu_regions.c)

zephyr_library_sources_ifdef(CONFIG_PM power.c)

if(CONFIG_PM_S2RAM)
  zephyr_library_sources(pm_s2ram_skip.c)
  # For <cortex_m/exception.h>, used to restore core CPU state on S2RAM resume.
  zephyr_library_include_directories(${ZEPHYR_BASE}/arch/arm/include)
endif()

zephyr_include_directories(.)

# Add the image definition block if required
zephyr_linker_sources(ROM_START rom_start.ld)

# Conditionally include a linker script.
if(CONFIG_ARM)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
else()
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
endif()
