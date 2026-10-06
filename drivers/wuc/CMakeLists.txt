# Copyright 2026 NXP
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_WUC_MCUX_LLWU wuc_nxp_llwu.c)
zephyr_library_sources_ifdef(CONFIG_WUC_MCUX_SLEEPCON wuc_nxp_sleepcon.c)
zephyr_library_sources_ifdef(CONFIG_WUC_MCUX_WUU wuc_nxp_wuu.c)
zephyr_library_sources_ifdef(CONFIG_WUC_SILABS_SIWX91X wuc_silabs_siwx91x.c)
# zephyr-keep-sorted-stop
