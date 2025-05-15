# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/saej2716/saej2716.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SAEJ2716_NXP_S32 saej2716_nxp_s32.c)
