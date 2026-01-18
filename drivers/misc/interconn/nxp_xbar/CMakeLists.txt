# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/misc/interconn/nxp_xbar/nxp_xbar.h)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_USERSPACE nxp_xbar_handlers.c)
zephyr_library_sources(nxp_xbar.c)
