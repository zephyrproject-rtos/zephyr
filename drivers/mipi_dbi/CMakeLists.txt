# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MIPI_DBI_SPI mipi_dbi_spi.c)
zephyr_sources_ifdef(CONFIG_MIPI_DBI_SMARTBOND mipi_dbi_smartbond.c)
zephyr_sources_ifdef(CONFIG_MIPI_DBI_NXP_LCDIC mipi_dbi_nxp_lcdic.c)
