# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/osa hal_nxp/components/osa)

add_subdirectory_ifdef(CONFIG_WIFI_NXP 
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/wifi_bt_module
  hal_nxp/components/wifi_bt_module
  )

add_subdirectory_ifdef(CONFIG_NXP_FW_LOADER 
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/conn_fwloader
  hal_nxp/components/conn_fwloader
  )
