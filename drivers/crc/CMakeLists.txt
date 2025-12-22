# Copyright (c) 2025 Renesas Electronics Corporation
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/crc.h)

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_CRC_DRIVER_NXP crc_nxp.c)
zephyr_library_sources_ifdef(CONFIG_CRC_DRIVER_RENESAS_RA crc_renesas_ra.c)
zephyr_library_sources_ifdef(CONFIG_CRC_DRIVER_SF32LB crc_sf32lb.c)
# zephyr-keep-sorted-stop
