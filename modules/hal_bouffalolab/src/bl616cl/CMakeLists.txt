# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

include("../../cmake/blob_objcopy.cmake")

set(BLOBS_DIR ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/zephyr/blobs/lib/${CONFIG_SOC_SERIES})

if(CONFIG_BT_BFLB_BL616CL)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${CONFIG_SOC_SERIES}/ble)

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

  message(STATUS "BL61x WiFi MACSW: ${MACSW_CFG_LIB}")
endif()

if((CONFIG_BT_BFLB_BL616CL OR CONFIG_WIFI_BFLB) AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)

  # Replace blob's printf with blob_printf
  set(BLOBS_OBJCOPY_ARGS --redefine-sym printf=blob_printf)

  # phyrf is shared between BLE and WiFi
  blob_objcopy(phyrf.a "${BLOBS_DIR}/libbl616cl_phyrf.a" ${BLOBS_OBJCOPY_ARGS})

  if(CONFIG_BT_BFLB_BL616CL)
    blob_objcopy(${BLE_VARIANT_LIB} "${BLOBS_DIR}/${BLE_VARIANT_LIB}" ${BLOBS_OBJCOPY_ARGS})
  endif()

  if(CONFIG_WIFI_BFLB)
    # WiFi blobs are fat LTO objects with incompatible IR; disable LTO at link time
    zephyr_link_libraries($<TARGET_PROPERTY:compiler,prohibit_lto>)

    zephyr_code_relocate(FILES
      ${ZEPHYR_BASE}/kernel/timeout.c
    LOCATION RAM NOKEEP)

    blob_objcopy(libwl80211.a "${BLOBS_DIR}/libwl80211_bl616cl.a" ${BLOBS_OBJCOPY_ARGS})
    blob_objcopy(libmacsw.a "${BLOBS_DIR}/libmacsw_bl616cl.a" ${BLOBS_OBJCOPY_ARGS})
    blob_objcopy(${MACSW_CFG_LIB} "${BLOBS_DIR}/${MACSW_CFG_LIB}" ${BLOBS_OBJCOPY_ARGS})
  endif()

endif()
