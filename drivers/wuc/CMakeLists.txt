# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_WUC_MCUX_LLWU wuc_nxp_llwu.c)
zephyr_library_sources_ifdef(CONFIG_WUC_MCUX_WUU wuc_nxp_wuu.c)
zephyr_library_sources_ifdef(CONFIG_WUC_RENESAS_RA_LPM wuc_renesas_ra_lpm.c)
# zephyr-keep-sorted-stop
