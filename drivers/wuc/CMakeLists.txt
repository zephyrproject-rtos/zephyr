# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_WUC_MCUX_WUU nxp_wuu.c)
# zephyr-keep-sorted-stop
