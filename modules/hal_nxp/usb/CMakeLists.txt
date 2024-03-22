#
# Copyright (c) 2021,2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_include_directories(.)
if(CONFIG_USB_UDC_NXP_MCUX)
if(CONFIG_USB_UDC_NXP_PHY)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/phy)
endif()
endif()
