# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

# Wirless framework
add_subdirectory_ifdef(
  CONFIG_IEEE802154_KW41Z
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/wireless/framework_5.3.3
  hal_nxp/middleware/wireless/framework
  )

# Wifi NXP
add_subdirectory_ifdef(CONFIG_WIFI_NXP
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/wifi_nxp
  hal_nxp/middleware/wifi_nxp
  )

# Connectivity framework and Multicore
if(CONFIG_BT OR CONFIG_NET_L2_OPENTHREAD)

    # Connectivity framework
    list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-connectivity-framework)
    include(connectivity_framework)

    # Multicore
    if(CONFIG_SOC_SERIES_MCXW)
        zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/mcmgr/src)

        zephyr_library_sources(
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/mcmgr/src/mcmgr_internal_core_api_mcxw716_ext.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/mcmgr/src/mcmgr_internal_core_api_mcxw716.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/mcmgr/src/mcmgr.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/mcmgr/src/mcmgr_imu_internal.c
        )

        zephyr_include_directories(
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/include
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/include/platform/kw45b41
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/include/environment/zephyr
        )

        zephyr_library_sources(
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/common/llist.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/virtio/virtqueue.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/rpmsg_lite/porting/platform/kw45b41/rpmsg_platform_ext.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/rpmsg_lite/porting/environment/rpmsg_env_zephyr.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/rpmsg_lite/porting/platform/kw45b41/rpmsg_platform.c
            ${ZEPHYR_CURRENT_MODULE_DIR}/middleware/mcux-sdk-middleware-multicore/rpmsg_lite/lib/rpmsg_lite/rpmsg_lite.c
        )
    endif()

endif()

# USB
if(CONFIG_UDC_DRIVER OR CONFIG_USB_DEVICE_DRIVER)

  if(DEFINED CONFIG_SOC_SDKNG_UNSUPPORTED)

    if(CONFIG_DT_HAS_NXP_USBPHY_ENABLED)
      zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/phy)
    endif()

  else() # CONFIG_SOC_SDKNG_UNSUPPORTED

    add_subdirectory_ifdef(CONFIG_DT_HAS_NXP_USBPHY_ENABLED 
     ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/middleware/usb
     hal_nxp/middleware/usb
     )

  endif() # CONFIG_SOC_SDKNG_UNSUPPORTED

  zephyr_include_directories(.)
endif()

# BT Controller
if(CONFIG_BT_H4_NXP_CTLR)

  set(hal_nxp_dir           ${ZEPHYR_HAL_NXP_MODULE_DIR})
  set(hal_nxp_blobs_dir     ${hal_nxp_dir}/zephyr/blobs)
  set(blob_gen_file         ${ZEPHYR_BINARY_DIR}/include/generated/bt_nxp_ctlr_fw.h)

  if(CONFIG_BT_NXP_NW612)
      set(blob_file ${hal_nxp_blobs_dir}/iw612/uart_nw61x_se.h)
  endif()

  if (NOT DEFINED blob_file)
      message(FATAL_ERROR "Unsupported controller. Please select a BT conntroller, refer to ./driver/bluetooth/hci/Kconfig.nxp")
  endif()

  zephyr_blobs_verify(FILES ${blob_file} REQUIRED)

  configure_file(${blob_file} ${blob_gen_file} COPYONLY)

endif()
