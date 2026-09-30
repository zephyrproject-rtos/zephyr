# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hl78xx_at_monitor)

set(HL78XX_DIR ${ZEPHYR_BASE}/drivers/modem/vendor_standalone/hl78xx)

# The AT monitor library alone, without the modem driver: the test calls the
# dispatch entry point the driver's chat layer would call.
target_sources(app PRIVATE
               ${HL78XX_DIR}/hl78xx_at_monitor/hl78xx_at_monitor.c
               src/main.c)
target_include_directories(app PRIVATE ${HL78XX_DIR}/hl78xx_at_monitor)
# Static monitors live in an iterable section the library's linker snippet
# defines.
zephyr_linker_sources(RWDATA ${HL78XX_DIR}/hl78xx_at_monitor/hl78xx_at_monitor.ld)
