# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/misc/nxp_rtxxx_adsp_ctl/nxp_rtxxx_adsp_ctl.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NXP_RTXXX_ADSP_CTL
  nxp_rtxxx_adsp_ctl.c
)
