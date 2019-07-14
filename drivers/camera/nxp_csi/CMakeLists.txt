# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NXP_MCUX_CSI nxp_mcux_csi.c)
