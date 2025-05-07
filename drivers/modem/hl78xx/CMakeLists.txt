#
#  Copyright (c) 2018 Analog Devices Inc.
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(
    hl78xx.c
    hl78xx_sockets.c
    hl78xx_utility.c

)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)
