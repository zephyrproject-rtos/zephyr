# SPDX-License-Identifier: Apache-2.0


zephyr_sources_ifdef(CONFIG_BT_DIS dis.c)

zephyr_sources_ifdef(CONFIG_BT_BAS bas.c)

zephyr_sources_ifdef(CONFIG_BT_HRS hrs.c)

zephyr_sources_ifdef(CONFIG_BT_TPS tps.c)

zephyr_sources_ifdef(CONFIG_BT_IAS ias.c)

if(CONFIG_BT_OTS OR CONFIG_BT_OTS_CLIENT)
  add_subdirectory(ots)
endif()
