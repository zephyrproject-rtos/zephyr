# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  bt.c
  hci.c
  )
zephyr_library_sources_ifdef(
  CONFIG_BT_CONN
  gatt.c
  )
zephyr_library_sources_ifdef(
  CONFIG_BT_BREDR
  bredr.c
  )
zephyr_library_sources_ifdef(
  CONFIG_BT_L2CAP_DYNAMIC_CHANNEL
  l2cap.c
  )
zephyr_library_sources_ifdef(
  CONFIG_BT_RFCOMM
  rfcomm.c
  )

if(CONFIG_BT_CTLR)
  if(CONFIG_BT_LL_SW_LEGACY OR CONFIG_BT_LL_SW_SPLIT)
    zephyr_library_sources(
      ll.c
      ticker.c
      )
    zephyr_include_directories(
      ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
      )
  endif()
endif()
