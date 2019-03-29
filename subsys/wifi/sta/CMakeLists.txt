# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(sta.c)
zephyr_library_sources(sm.c)

zephyr_library_sources_ifdef(CONFIG_WIFIMGR_DHCPC dhcpc.c)

zephyr_library_link_libraries(subsys__wifi)
