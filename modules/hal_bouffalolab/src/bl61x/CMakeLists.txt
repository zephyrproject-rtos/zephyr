# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

set(BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/${SOC_SERIES})

if(CONFIG_BT_BFLB_BL61X)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/ble)

  # Select controller variant
  if(CONFIG_BFLB_BL61X_BLE_1M0S1)
    set(BLE_VARIANT_LIB libbtblecontroller_bl616_ble1m0s1bredr0.a)
  elseif(CONFIG_BFLB_BL61X_BLE_1M10S1)
    set(BLE_VARIANT_LIB libbtblecontroller_bl616_ble1m10s1bredr0.a)
  elseif(CONFIG_BFLB_BL61X_BLE_1M2S1BREDR1)
    set(BLE_VARIANT_LIB libbtblecontroller_bl616_ble1m2s1bredr1.a)
  endif()

  message(STATUS "BL61x BLE controller: ${BLE_VARIANT_LIB}")
endif()

if(CONFIG_WIFI_BFLB)
  # Select MACSW variant
  if(CONFIG_BFLB_WIFI_BL61X_MACSW_DEFAULT)
    set(MACSW_LIB libmacsw_bl616_default.a)
    set(MACSW_CFG_LIB libmacsw_config_bl616_default.a)
  elseif(CONFIG_BFLB_WIFI_BL61X_MACSW_BRIDGE)
    set(MACSW_LIB libmacsw_bl616_bridge.a)
    set(MACSW_CFG_LIB libmacsw_config_bl616_bridge.a)
  elseif(CONFIG_BFLB_WIFI_BL61X_MACSW_ACK)
    set(MACSW_LIB libmacsw_bl616_ack.a)
    set(MACSW_CFG_LIB libmacsw_config_bl616_ack.a)
  elseif(CONFIG_BFLB_WIFI_BL61X_MACSW_COEX)
    set(MACSW_LIB libmacsw_bl616_coex.a)
    set(MACSW_CFG_LIB libmacsw_config_bl616_coex.a)
  endif()

  message(STATUS "BL61x WiFi MACSW: ${MACSW_LIB}")
endif()

if((CONFIG_BT_BFLB_BL61X OR CONFIG_WIFI_BFLB) AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  # phyrf is shared between BLE and WiFi
  zephyr_link_libraries(${BLOBS_DIR}/libbl616_phyrf.a)

  if(CONFIG_BT_BFLB_BL61X)
    zephyr_link_libraries(${BLOBS_DIR}/${BLE_VARIANT_LIB})
  endif()

  if(CONFIG_WIFI_BFLB)
    # WiFi blobs are fat LTO objects with incompatible IR; disable LTO at link time
    zephyr_link_libraries($<TARGET_PROPERTY:compiler,prohibit_lto>)

    zephyr_link_libraries(
      ${BLOBS_DIR}/libwl80211_bl616.a
      ${BLOBS_DIR}/${MACSW_LIB}
      ${BLOBS_DIR}/${MACSW_CFG_LIB}
    )
  endif()

endif()
