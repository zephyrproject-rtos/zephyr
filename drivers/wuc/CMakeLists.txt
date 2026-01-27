# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_WUC_ITE wuc_ite.c)
zephyr_library_sources_ifdef(CONFIG_WUC_MCUX_LLWU wuc_nxp_llwu.c)

# zephyr-keep-sorted-stop
