# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_NET_PKT_FILTER)

zephyr_library_sources(base.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_ETHERNET ethernet.c)

endif()
