# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/ptp_clock.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_PTP_CLOCK ptp_clock.c)
zephyr_library_sources_ifdef(CONFIG_PTP_CLOCK_NXP_ENET ptp_clock_nxp_enet.c)
