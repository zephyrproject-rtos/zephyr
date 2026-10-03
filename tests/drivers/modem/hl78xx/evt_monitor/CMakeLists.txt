# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hl78xx_evt_monitor)

set(HL78XX_DIR ${ZEPHYR_BASE}/drivers/modem/vendor_standalone/hl78xx)

# The monitor library alone, without the modem driver: the test supplies the
# one driver symbol the library needs (hl78xx_evt_notif_handler_set) and
# drives hl78xx_evt_monitor_dispatch() directly.
target_sources(app PRIVATE
               ${HL78XX_DIR}/hl78xx_evt_monitor/hl78xx_evt_monitor.c
               src/main.c)
# Static monitors live in an iterable section the library's linker snippet
# defines.
zephyr_linker_sources(RWDATA ${HL78XX_DIR}/hl78xx_evt_monitor/hl78xx_evt_monitor.ld)
