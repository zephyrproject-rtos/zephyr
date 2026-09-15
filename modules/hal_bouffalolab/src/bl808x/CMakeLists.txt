# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BT_BFLB_BL808)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/ble)

  set(BLE_BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/bl808x)

  message(STATUS "BL808 BLE: on-chip controller blobs")

  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    zephyr_link_libraries(
      ${BLE_BLOBS_DIR}/libbl606p_btblecontroller_prebuild.a
      ${BLE_BLOBS_DIR}/libbl606p_phyrf.a
    )
  endif()
endif()
