# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/crc.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_CRC_DRIVER_RENESAS_RA crc_renesas_ra.c)
