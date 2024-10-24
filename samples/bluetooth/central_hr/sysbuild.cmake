# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_NET_CORE_IMAGE_HCI_IPC)
  # For builds in the nrf5340, we build the netcore image with the controller
  set(NET_APP hci_ipc)
  set(NET_APP_SRC_DIR ${ZEPHYR_BASE}/samples/bluetooth/${NET_APP})

  ExternalZephyrProject_Add(
    APPLICATION ${NET_APP}
    SOURCE_DIR  ${NET_APP_SRC_DIR}
    BOARD       ${SB_CONFIG_NET_CORE_BOARD}
  )

  set(${NET_APP}_CONF_FILE
      ${NET_APP_SRC_DIR}/nrf5340_cpunet_iso-bt_ll_sw_split.conf
      CACHE INTERNAL ""
  )

  if(SB_CONFIG_TEST_MODE_PHY_CODED)
    set_config_bool(${NET_APP} CONFIG_BT_CTLR_PHY_CODED y)
    set_config_bool(${NET_APP} CONFIG_BT_EXT_ADV y)
  elseif(SB_CONFIG_TEST_MODE_EXTENDED)
    set_config_bool(${NET_APP} CONFIG_BT_CTLR_PHY_CODED n)
    set_config_bool(${NET_APP} CONFIG_BT_EXT_ADV y)
  else()
    set_config_bool(${NET_APP} CONFIG_BT_CTLR_PHY_CODED n)
    set_config_bool(${NET_APP} CONFIG_BT_EXT_ADV n)
  endif()

  if(SB_CONFIG_TEST_MODE_EXTENDED OR SB_CONFIG_TEST_MODE_PHY_CODED)
    set_config_int(${NET_APP} CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX ${SB_CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX})
    set_config_int(${NET_APP} CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE ${SB_CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE})
  endif()

  native_simulator_set_child_images(${DEFAULT_IMAGE} ${NET_APP})
endif()

native_simulator_set_final_executable(${DEFAULT_IMAGE})

if(SB_CONFIG_BT_CTLR_PHY_CODED)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_CTLR_PHY_CODED y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_EXT_ADV y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_AUTO_PHY_UPDATE n)
elseif(SB_CONFIG_TEST_MODE_EXTENDED)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_CTLR_PHY_CODED n)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_EXT_ADV y)
else()
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_CTLR_PHY_CODED n)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_EXT_ADV n)
endif()

if(SB_CONFIG_TEST_MODE_EXTENDED OR SB_CONFIG_TEST_MODE_PHY_CODED)
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX ${SB_CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE ${SB_CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE})
endif()
