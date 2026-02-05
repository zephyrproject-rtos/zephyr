# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_VIDEO_IMX_DPHY_RX phy_imx_dphy-rx.c)
