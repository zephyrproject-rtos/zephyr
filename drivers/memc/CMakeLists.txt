# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MEMC_HIFIVE_UNMATCHED_DRAM   sifive_ddr.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MAX32_HPB               memc_max32_hpb.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI            memc_mcux_flexspi.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI_APS6404L   memc_mcux_flexspi_aps6404l.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI_APS6408L   memc_mcux_flexspi_aps6408l.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI_IS66WVQ8M4 memc_mcux_flexspi_is66wvq8m4.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI_S27KS0641  memc_mcux_flexspi_s27ks0641.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI_W956A8MBYA memc_mcux_flexspi_w956a8mbya.c)
if((DEFINED CONFIG_FLASH_MCUX_FLEXSPI_XIP) AND (DEFINED CONFIG_FLASH))
  zephyr_code_relocate(FILES memc_mcux_flexspi.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
endif()

zephyr_library_sources_ifdef(CONFIG_MEMC_MSPI_APS6404L           memc_mspi_aps6404l.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_MSPI_APS_Z8             memc_mspi_aps_z8.c)
zephyr_library_include_directories_ifdef(CONFIG_MEMC_MSPI ${ZEPHYR_BASE}/drivers/mspi)

zephyr_library_sources_ifdef(CONFIG_MEMC_NXP_S32_QSPI            memc_nxp_s32_qspi.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_RENESAS_RA_SDRAM        memc_renesas_ra_sdram.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_SAM_SMC                 memc_sam_smc.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_SILABS_SIWX91X_QSPI     memc_silabs_siwx91x_qspi.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_SMARTBOND               memc_smartbond_nor_psram.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_STM32                   memc_stm32.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_STM32_NOR_PSRAM         memc_stm32_nor_psram.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_STM32_SDRAM             memc_stm32_sdram.c)

zephyr_library_sources_ifdef(CONFIG_MEMC_STM32_XSPI_PSRAM        memc_stm32_xspi_psram.c)
zephyr_library_sources_ifdef(CONFIG_MEMC_STM32_OSPI_PSRAM        memc_stm32_ospi_psram.c)
