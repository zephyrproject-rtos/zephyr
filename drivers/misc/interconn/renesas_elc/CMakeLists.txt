# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/misc/interconn/renesas_elc/renesas_elc.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE renesas_elc_handlers.c)

zephyr_library_sources_ifdef(CONFIG_RENESAS_RA_ELC renesas_ra_elc.c)
