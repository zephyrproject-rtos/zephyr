# Copyright (c) 2022 NXP
# SPDX-License-Identifier: Apache-2.0
if (CONFIG_SDHC)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_IMX_USDHC imx_usdhc.c)
endif()
