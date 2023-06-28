# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_FIRMWARE_IMX_SCU imx_scu.c)
