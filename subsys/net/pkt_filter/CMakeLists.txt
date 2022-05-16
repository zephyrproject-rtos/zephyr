# SPDX-License-Identifier: Apache-2.0


if(CONFIG_NET_PKT_FILTER)
zephyr_library()
zephyr_library_sources(base.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_ETHERNET ethernet.c)

endif()
