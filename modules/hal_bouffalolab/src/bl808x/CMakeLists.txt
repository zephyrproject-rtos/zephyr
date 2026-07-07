# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

include("../../cmake/blob_objcopy.cmake")

if(CONFIG_BT_BFLB_BL808)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/ble)

  set(BLE_BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/bl808x)

  message(STATUS "BL808 BLE: on-chip controller blobs")

  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    # Replace blob's printf with blob_printf
    set(BLOBS_OBJCOPY_ARGS --redefine-sym printf=blob_printf)

    blob_objcopy(btblecontroller.a
      "${BLE_BLOBS_DIR}/libbl606p_btblecontroller_prebuild.a" ${BLOBS_OBJCOPY_ARGS})
    blob_objcopy(phyrf.a "${BLE_BLOBS_DIR}/libbl606p_phyrf.a" ${BLOBS_OBJCOPY_ARGS})
  endif()
endif()
