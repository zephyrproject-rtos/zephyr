# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/misc/nxp_rtxxx_dsp_ctrl/nxp_rtxxx_dsp_ctrl.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NXP_RTXXX_DSP_CTRL
  nxp_rtxxx_dsp_ctrl.c
)
