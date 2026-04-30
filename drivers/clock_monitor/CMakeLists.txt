# Copyright (c) 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/clock_monitor.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE clock_monitor_handlers.c)

zephyr_library_sources_ifdef(CONFIG_CLOCK_MONITOR_NXP_CMU_FC clock_monitor_nxp_cmu_fc.c)
zephyr_library_sources_ifdef(CONFIG_CLOCK_MONITOR_NXP_CMU_FM clock_monitor_nxp_cmu_fm.c)
