# Copyright (c) 2026 The Zephyr Project Contributors
# Copyright (c) 2026 Dev It Wise
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/board_id.h)

zephyr_library_sources_ifdef(CONFIG_BOARD_ID_GPIO board_id_gpio.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE board_id_handlers.c)
