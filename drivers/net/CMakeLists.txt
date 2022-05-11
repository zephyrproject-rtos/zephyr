# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NET_LOOPBACK loopback.c)
zephyr_library_sources_ifdef(CONFIG_NET_CANBUS canbus.c)

if(CONFIG_NET_NATIVE)
zephyr_library_sources_ifdef(CONFIG_SLIP slip.c)
zephyr_library_sources_ifdef(CONFIG_NET_PPP ppp.c)
endif()
