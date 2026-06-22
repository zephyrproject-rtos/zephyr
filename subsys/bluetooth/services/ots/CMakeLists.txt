# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_BT_OTS
  ots.c
  ots_l2cap.c
  ots_obj_manager.c
  ots_oacp.c
  ots_olcp.c)

  zephyr_library_sources_ifdef(CONFIG_BT_OTS_DIR_LIST_OBJ ots_dir_list.c)

zephyr_library_sources_ifdef(
  CONFIG_BT_OTS_CLIENT
  ots_client.c
  ots_l2cap.c)
