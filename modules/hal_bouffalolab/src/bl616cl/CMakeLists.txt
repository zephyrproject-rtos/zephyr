# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

set(BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/${SOC_SERIES})

if(CONFIG_BT_BFLB_BL616CL)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/ble)

  # Select controller variant
  if(CONFIG_BFLB_BL616CL_BLE_M2S1)
    set(BLE_VARIANT_LIB libbtblecontroller_bl616cl_m2s1.a)
  elseif(CONFIG_BFLB_BL616CL_BLE_M0B1)
    set(BLE_VARIANT_LIB libbtblecontroller_bl616cl_m0b1.a)
  endif()

  message(STATUS "BL616CL BLE controller: ${BLE_VARIANT_LIB}")
endif()

if(CONFIG_WIFI_BFLB)
  # Select MACSW variant
  if(CONFIG_WIFI_BFLB_WIFI6_MACSW_ACK)
    set(MACSW_CFG_LIB libmacsw_config_bl616cl_ack.a)
  else()
    set(MACSW_CFG_LIB libmacsw_config_bl616cl_default.a)
  endif()

  message(STATUS "BL61x WiFi MACSW: ${MACSW_LIB}")
endif()

if((CONFIG_BT_BFLB_BL616CL OR CONFIG_WIFI_BFLB) AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  # phyrf is shared between BLE and WiFi
  zephyr_link_libraries(${BLOBS_DIR}/libbl616cl_phyrf.a)

  if(CONFIG_BT_BFLB_BL616CL)
    zephyr_link_libraries(${BLOBS_DIR}/${BLE_VARIANT_LIB})
  endif()

  if(CONFIG_WIFI_BFLB)
    # WiFi blobs are fat LTO objects with incompatible IR; disable LTO at link time
    zephyr_link_libraries($<TARGET_PROPERTY:compiler,prohibit_lto>)

    zephyr_code_relocate(FILES
      ${ZEPHYR_BASE}/kernel/timeout.c
    LOCATION RAM NOKEEP)


    zephyr_link_libraries(
      ${BLOBS_DIR}/libwl80211_bl616cl.a
      ${BLOBS_DIR}/libmacsw_bl616cl.a
      ${BLOBS_DIR}/${MACSW_CFG_LIB}
    )
  endif()

endif()
