# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

include("../../cmake/blob_objcopy.cmake")

set(BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/${CONFIG_SOC_SERIES})

if(CONFIG_BT_BFLB_BL60X)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${CONFIG_SOC_SERIES}/ble)

  if(CONFIG_BFLB_BL60X_BLE_M1S1)
    set(BLE_VARIANT_LIB libblecontroller_bl602_m1s1.a)
  elseif(CONFIG_BFLB_BL60X_BLE_M8S1)
    set(BLE_VARIANT_LIB libblecontroller_bl602_m8s1.a)
  endif()

  message(STATUS "BL60x BLE controller: ${BLE_VARIANT_LIB}")

  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    zephyr_link_libraries(
      ${BLOBS_DIR}/${BLE_VARIANT_LIB}
      -Wl,--wrap=rfc_init
    )
  endif()
endif()

if(CONFIG_WIFI_BFLB AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  # The wraps intercept calls and chain to __real_, which objcopy cannot
  # express since it also renames the archive's internal references
  zephyr_link_libraries(
    ${BLOBS_DIR}/libbl602_firmware.a
    -Wl,--wrap=rfc_init
    -Wl,--wrap=ipc_emb_wait
    -Wl,--wrap=ipc_emb_notify
    -Wl,--wrap=assert_err
    -Wl,--wrap=rc_update_counters
    -Wl,--wrap=sta_mgmt_unregister
  )
endif()

if((CONFIG_BT_BFLB_BL60X OR CONFIG_WIFI_BFLB) AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  # Replace blob's printf with blob_printf; linked after the firmware
  # blob, which references phyrf symbols
  blob_objcopy(libbl602_phyrf.a "${BLOBS_DIR}/libbl602_phyrf.a" --redefine-sym printf=blob_printf)
endif()
