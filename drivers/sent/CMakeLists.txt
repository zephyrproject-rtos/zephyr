# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/sent/sent.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SENT_NXP_S32 sent_nxp_s32.c)
