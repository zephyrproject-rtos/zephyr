# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BT_BFLB_BL60X)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/ble)

  if(CONFIG_BFLB_BL60X_BLE_M1S1)
    set(BLE_VARIANT_LIB libblecontroller_bl602_m1s1.a)
  elseif(CONFIG_BFLB_BL60X_BLE_M8S1)
    set(BLE_VARIANT_LIB libblecontroller_bl602_m8s1.a)
  endif()

  message(STATUS "BL60x BLE controller: ${BLE_VARIANT_LIB}")
  set(BLE_BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/${SOC_SERIES})

  set(PHYRF_LIB libbl602_phyrf.a)

  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    zephyr_link_libraries(
      ${BLE_BLOBS_DIR}/${BLE_VARIANT_LIB}
      ${BLE_BLOBS_DIR}/${PHYRF_LIB}
    )
  endif()
endif()
