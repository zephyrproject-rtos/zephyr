# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(..)

zephyr_library_sources(imx-mipi-csi.c)
zephyr_library_sources(imx9-dphy-rx.c)
zephyr_library_sources(phy_imx95.c)
