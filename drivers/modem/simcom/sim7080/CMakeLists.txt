# Copyright (c) 2025 Metratec GmbH.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/modem)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)
zephyr_library_sources(
    sim7080.c
    sim7080_pdp.c
    sim7080_dns.c
    sim7080_sock.c
    sim7080_sms.c
    sim7080_ftp.c
    sim7080_gps.c
    sim7080_meas.c
    sim7080_utils.c)
