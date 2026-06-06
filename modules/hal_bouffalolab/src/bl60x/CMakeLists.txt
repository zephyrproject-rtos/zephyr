# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

set(BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/${SOC_SERIES})

if(CONFIG_BT_BFLB_BL60X)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/ble)

  if(CONFIG_BFLB_BL60X_BLE_M1S1)
    set(BLE_VARIANT_LIB libblecontroller_bl602_m1s1.a)
  elseif(CONFIG_BFLB_BL60X_BLE_M8S1)
    set(BLE_VARIANT_LIB libblecontroller_bl602_m8s1.a)
  endif()

  message(STATUS "BL60x BLE controller: ${BLE_VARIANT_LIB}")

  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    zephyr_link_libraries(
      ${BLOBS_DIR}/${BLE_VARIANT_LIB}
      ${BLOBS_DIR}/libbl602_phyrf.a
    )
  endif()
endif()

if(CONFIG_WIFI_BFLB AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  zephyr_link_libraries(
    ${BLOBS_DIR}/libbl602_firmware.a
    ${BLOBS_DIR}/libbl602_phyrf.a
    -Wl,--wrap=rfc_init
    -Wl,--wrap=rf_pri_pm_pwr_avg
    -Wl,--wrap=wifi_main
    -Wl,--wrap=hal_machw_gen_handler
    -Wl,--wrap=rxu_cntrl_frame_handle
    -Wl,--wrap=txl_cntrl_push
    -Wl,--wrap=ipc_emb_wait
    -Wl,--wrap=ipc_emb_notify
    -Wl,--wrap=printf
  )
endif()
