# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NXP_RF_IMU)
  if(CONFIG_SOC_SERIES_RW6XX)
      list(APPEND CMAKE_MODULE_PATH
          ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/rpmsg
      )
    include(component_wireless_imu_adapter)
  elseif(CONFIG_SOC_SERIES_MCXW)
      zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/rpmsg)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/rpmsg/fsl_adapter_rpmsg.c)
      zephyr_compile_definitions(HAL_RPMSG_SELECT_ROLE=0U)
  endif()
endif()

# USB
if(CONFIG_UDC_DRIVER OR CONFIG_USB_DEVICE_DRIVER)
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/middleware/usb
    hal_nxp/middleware/usb
    )
  zephyr_include_directories(.)
endif()

# BT Controller
if(CONFIG_BT_H4_NXP_CTLR)
  include(${CMAKE_CURRENT_LIST_DIR}/bt_controller.cmake)
endif()
