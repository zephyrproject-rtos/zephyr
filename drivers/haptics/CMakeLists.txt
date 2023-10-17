# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/haptics.h)

zephyr_library_sources_ifdef(CONFIG_USERSPACE haptics_handlers.c)
