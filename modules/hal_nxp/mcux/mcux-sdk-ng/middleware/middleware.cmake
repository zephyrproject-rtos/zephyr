# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USB_DEVICE_DRIVER)
  zephyr_include_directories(middleware)

  set(CONFIG_MCUX_COMPONENT_middleware.usb.common_header ON)
  set(CONFIG_MCUX_COMPONENT_middleware.usb.device.common_header ON)
  set(CONFIG_MCUX_COMPONENT_middleware.usb.phy ON)

  set_variable_ifdef(CONFIG_USB_DC_NXP_EHCI CONFIG_MCUX_COMPONENT_middleware.usb.device.ehci)
  set_variable_ifdef(CONFIG_USB_DC_NXP_LPCIP3511 CONFIG_MCUX_COMPONENT_middleware.usb.device.ip3511fs)

  # For soc.c build pass
  zephyr_include_directories(.)
  zephyr_include_directories(${MCUX_SDK_NG_DIR}/middleware/usb/device)
  zephyr_include_directories(${MCUX_SDK_NG_DIR}/middleware/usb/phy)
  zephyr_include_directories(${MCUX_SDK_NG_DIR}/middleware/usb/include)
endif()

if(CONFIG_UDC_DRIVER)
  zephyr_include_directories(middleware)

  set(CONFIG_MCUX_COMPONENT_middleware.usb.common_header ON)
  set(CONFIG_MCUX_COMPONENT_middleware.usb.device.common_header ON)

  set_variable_ifdef(CONFIG_DT_HAS_NXP_USBPHY_ENABLED CONFIG_MCUX_COMPONENT_middleware.usb.phy)
  set_variable_ifdef(CONFIG_UDC_NXP_EHCI CONFIG_MCUX_COMPONENT_middleware.usb.device.ehci)
  set_variable_ifdef(CONFIG_UDC_NXP_IP3511 CONFIG_MCUX_COMPONENT_middleware.usb.device.ip3511fs)

  # For soc.c build pass
  zephyr_include_directories(.)
  zephyr_include_directories(${MCUX_SDK_NG_DIR}/middleware/usb/device)
  zephyr_include_directories(${MCUX_SDK_NG_DIR}/middleware/usb/phy)
  zephyr_include_directories(${MCUX_SDK_NG_DIR}/middleware/usb/include)
endif()

add_subdirectory(${MCUX_SDK_NG_DIR}/middleware/usb
  ${CMAKE_CURRENT_BINARY_DIR}/usb
)

if(CONFIG_BT_H4_NXP_CTLR)
  add_subdirectory(bt_controller)
endif()
