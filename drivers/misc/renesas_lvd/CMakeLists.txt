# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/misc/renesas_lvd/renesas_lvd.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE renesas_lvd_handlers.c)

zephyr_library_sources_ifdef(CONFIG_RENESAS_RX_LVD renesas_rx_lvd.c)
