# SPDX-License-Identifier: Apache-2.0


zephyr_sources_ifdef(CONFIG_BT_DIS dis.c)

zephyr_sources_ifdef(CONFIG_BT_BAS bas.c)

zephyr_sources_ifdef(CONFIG_BT_HRS hrs.c)

zephyr_sources_ifdef(CONFIG_BT_TPS tps.c)

add_subdirectory_ifdef(CONFIG_BT_OTS ots)
