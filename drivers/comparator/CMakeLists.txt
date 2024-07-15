# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/comparator.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE comparator_handlers.c)
