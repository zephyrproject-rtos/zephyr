#
# Copyright (c) 2017-2021, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_sources_ifdef(CONFIG_SOC_SERIES_IMX_RT11XX soc_rt11xx.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_IMX_RT10XX soc_rt10xx.c)

zephyr_linker_sources_ifdef(CONFIG_NXP_IMX_RT_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

# Enable custom OCRAM noncacheable region
zephyr_linker_sources_ifdef(CONFIG_OCRAM_NOCACHE SECTIONS sections.ld)
zephyr_sources_ifdef(CONFIG_OCRAM_NOCACHE mpu_regions.c)

zephyr_linker_section_configure(
  SECTION .rom_start
  INPUT ".boot_hdr.conf"
  OFFSET ${CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET}
  KEEP
  PRIO 10
)

if(CONFIG_DEVICE_CONFIGURATION_DATA)
  set(boot_hdr_dcd_data_section ".boot_hdr.dcd_data")
endif()

if(CONFIG_PM)
  zephyr_sources_ifdef(CONFIG_SOC_SERIES_IMX_RT11XX power_rt11xx.c)
endif()

if (CONFIG_PM AND CONFIG_SOC_SERIES_IMX_RT10XX)
  zephyr_sources(power_rt10xx.c)
  zephyr_code_relocate(power_rt10xx.c ITCM_TEXT)
  if (CONFIG_SOC_MIMXRT1064)
    zephyr_sources(lpm_rt1064.c)
    zephyr_code_relocate(lpm_rt1064.c ITCM_TEXT)
  endif()
endif()

zephyr_compile_definitions(
  XIP_EXTERNAL_FLASH
)
zephyr_linker_sources(
  RWDATA nocache.ld)

zephyr_linker_section_configure(
  SECTION .rom_start
  INPUT ".boot_hdr.ivt"
        ".boot_hdr.data"
        ${boot_hdr_dcd_data_section}
  OFFSET ${CONFIG_IMAGE_VECTOR_TABLE_OFFSET}
  KEEP
  PRIO 11
)
