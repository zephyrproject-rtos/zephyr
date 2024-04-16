# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0
if (CONFIG_SDHC)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_IMX_USDHC imx_usdhc.c)
zephyr_library_sources_ifdef(CONFIG_SPI_SDHC sdhc_spi.c)
zephyr_library_sources_ifdef(CONFIG_MCUX_SDIF mcux_sdif.c)
zephyr_library_sources_ifdef(CONFIG_SAM_HSMCI sam_hsmci.c)
zephyr_library_sources_ifdef(CONFIG_INTEL_EMMC_HOST intel_emmc_host.c)
endif()
