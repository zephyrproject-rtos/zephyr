# Copyright (c) 2021-2025 ATL Electronics
# Copyright (c) 2024-2026 MASSDRIVER EI
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_SERIES_BL60X OR CONFIG_SOC_SERIES_BL70X OR CONFIG_SOC_SERIES_BL70XL)
  zephyr_include_directories(e24)
  zephyr_sources(
    e24/soc_irq_privileged.c
    e24/soc.c
    e24/sf_offset.S
  )
endif()

if(CONFIG_SOC_SERIES_BL61X OR CONFIG_SOC_SERIES_BL808 OR CONFIG_SOC_SERIES_BL616CL)
  zephyr_include_directories(thead)
  zephyr_sources(
    thead/sf_offset.S
  )
endif()

# Add offset change as the very first code in ROM
zephyr_linker_sources(ROM_START SORT_KEY "0x0bflb" sf_offset.ld)

zephyr_sources(
  power.c
  checks.c
)

# Relocate drivers that need to be entirely relocated
# Cache management defaults to XIP, this is fine but we need to use it in the flash driver
# Efuses do clock operations, almost all its function would need to be moved, so take the easy way
# Flash and Clock Control drivers handle relocation on their own
if(CONFIG_SOC_SERIES_BL61X OR CONFIG_SOC_SERIES_BL808 OR CONFIG_SOC_SERIES_BL616CL)
  zephyr_code_relocate(
    FILES ${ZEPHYR_BASE}/arch/riscv/custom/thead/cache_xtheadcmo.c LOCATION RAM NOKEEP
  )
  zephyr_code_relocate_ifdef(CONFIG_OTP_BFLB LIBRARY drivers__otp LOCATION RAM NOKEEP)
else()
  zephyr_code_relocate_ifdef(CONFIG_CACHE_BFLB_L1C LIBRARY drivers__cache LOCATION ITCM NOKEEP)
  zephyr_code_relocate_ifdef(CONFIG_OTP_BFLB LIBRARY drivers__otp LOCATION ITCM NOKEEP)
endif()
