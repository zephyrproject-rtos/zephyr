# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BT_BFLB_BL70X)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/ble)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/rf)

  # Select controller library based on Kconfig variant
  if(CONFIG_BFLB_BL70X_BLE_M0S1P)
    set(BLE_VARIANT_LIB libblecontroller_bl702_m0s1p.a)
  elseif(CONFIG_BFLB_BL70X_BLE_M0S1)
    set(BLE_VARIANT_LIB libblecontroller_bl702_m0s1.a)
  elseif(CONFIG_BFLB_BL70X_BLE_M0S1S)
    set(BLE_VARIANT_LIB libblecontroller_bl702_m0s1s.a)
  elseif(CONFIG_BFLB_BL70X_BLE_M0S1T10)
    set(BLE_VARIANT_LIB libblecontroller_bl702_m0s1t10.a)
  elseif(CONFIG_BFLB_BL70X_BLE_M0S0SP)
    set(BLE_VARIANT_LIB libblecontroller_bl702_m0s0sp.a)
  elseif(CONFIG_BFLB_BL70X_BLE_M1S1)
    set(BLE_VARIANT_LIB libblecontroller_bl702_m1s1.a)
  elseif(CONFIG_BFLB_BL70X_BLE_M16S1)
    set(BLE_VARIANT_LIB libblecontroller_bl702_m16s1.a)
  elseif(CONFIG_BFLB_BL70X_BLE_UARTHCI)
    set(BLE_VARIANT_LIB libblecontroller_bl702_uarthci.a)
  endif()

  message(STATUS "BL70x BLE controller variant: ${BLE_VARIANT_LIB}")

  set(BLE_BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/${SOC_SERIES})

  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    zephyr_link_libraries(
      ${BLE_BLOBS_DIR}/${BLE_VARIANT_LIB}
      ${BLE_BLOBS_DIR}/libbl702_rf.a
    )
  endif()
endif()
