#
# Copyright (c) 2021,2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_include_directories(.)
if(CONFIG_DT_HAS_NXP_USBPHY_ENABLED)
zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/phy)
endif()
