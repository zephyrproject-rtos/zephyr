# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/kscan.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_KSCAN_HT16K33	kscan_ht16k33.c)
zephyr_library_sources_ifdef(CONFIG_KSCAN_INPUT		kscan_input.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE	    kscan_handlers.c)
