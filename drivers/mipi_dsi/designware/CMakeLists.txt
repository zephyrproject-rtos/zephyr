# Copyright (c) 2026 Analog Devices Inc
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(dsi_dw.c)
zephyr_sources(dphy_dw.c)
zephyr_sources_ifdef(CONFIG_MIPI_DSI_DW_ALIF dsi_dw_alif.c)
