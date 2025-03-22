# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(bredr.c)
zephyr_library_sources_ifdef(CONFIG_BT_RFCOMM rfcomm.c)
zephyr_library_sources_ifdef(CONFIG_BT_A2DP a2dp.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVRCP avrcp.c)
zephyr_library_sources_ifdef(CONFIG_BT_GOEP goep.c)
