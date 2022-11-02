#
# Copyright (c) 2021 Linaro, Limited
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_HAS_MCUX OR CONFIG_HAS_IMX_HAL OR CONFIG_HAS_S32_HAL)
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} hal_nxp)
  add_subdirectory_ifdef(CONFIG_USB_DEVICE_DRIVER usb)

  zephyr_include_directories(.)
endif()
