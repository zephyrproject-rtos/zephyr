# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  bt.c
  bt_shell_private.c
  )
zephyr_library_sources_ifdef(CONFIG_BT_CONN gatt.c)
zephyr_library_sources_ifdef(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL l2cap.c)
zephyr_library_sources_ifdef(CONFIG_BT_ISO iso.c)
zephyr_library_sources_ifdef(CONFIG_BT_CHANNEL_SOUNDING cs.c)
