# Copyright (c) 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/clock_monitor.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE clock_monitor_handlers.c)
