# Copyright (c) 2026, Qingsong Gou <gouqs@hotmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/jdi.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_JDI_SF32LB jdi_sf32lb.c)
