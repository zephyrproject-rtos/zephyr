#
#  Copyright (c) 2025 Netfeasa
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(
    hl78xx.c
    hl78xx_sockets.c
    hl78xx_utility.c
    hl78xx_apis.c
)

add_subdirectory_ifdef(CONFIG_HL78XX_EVT_MONITOR hl78xx_evt_monitor)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)
