# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

include("../../cmake/blob_objcopy.cmake")

set(BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/bl808x)

# Replace the blobs' printf with blob_printf
set(BLOBS_OBJCOPY_ARGS --redefine-sym printf=blob_printf)

if(CONFIG_BT_BFLB_BL808)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${CONFIG_SOC_SERIES}/ble)

  message(STATUS "BL808 BLE: on-chip controller blobs")

  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    blob_objcopy(btblecontroller.a
      "${BLOBS_DIR}/libbl606p_btblecontroller_prebuild.a" ${BLOBS_OBJCOPY_ARGS})
  endif()
endif()

if(CONFIG_WIFI_BFLB AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  blob_objcopy(wifi_firmware.a "${BLOBS_DIR}/libbl606p_firmware_prebuild.a"
    ${BLOBS_OBJCOPY_ARGS}
    --redefine-sym puts=blob_puts
    --redefine-sym putchar=blob_putchar
  )
  # The wraps intercept calls and chain to __real_, which objcopy cannot
  # express since it also renames the archive's internal references
  zephyr_link_libraries(
    -Wl,--wrap=ipc_emb_wait
    -Wl,--wrap=ipc_emb_notify
    -Wl,--wrap=assert_err
    -Wl,--wrap=bl_sleep_schedule
    -Wl,--wrap=rc_update_counters
    -Wl,--wrap=sta_mgmt_unregister
  )
endif()

if((CONFIG_BT_BFLB_BL808 OR CONFIG_WIFI_BFLB) AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  # Linked after the controller and firmware blobs, which reference it
  blob_objcopy(phyrf.a "${BLOBS_DIR}/libbl606p_phyrf.a" ${BLOBS_OBJCOPY_ARGS})
  zephyr_link_libraries(-Wl,--wrap=rfc_init)
endif()
