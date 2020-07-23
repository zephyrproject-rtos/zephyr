# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_BT_OTS
  ots.c
  ots_l2cap.c
  ots_obj_manager.c
  ots_oacp.c
  ots_olcp.c)
