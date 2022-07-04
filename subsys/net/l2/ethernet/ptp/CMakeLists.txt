# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NET_PTP_MASTER ptp_master.c)
zephyr_library_sources_ifdef(CONFIG_NET_PTP_SLAVE ptp_slave.c)
