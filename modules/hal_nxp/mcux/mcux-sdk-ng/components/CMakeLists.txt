# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/osa
  hal_nxp/components/osa
  )

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/wifi_bt_module
  hal_nxp/components/wifi_bt_module
  )

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/conn_fwloader
  hal_nxp/components/conn_fwloader
  )

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/lists
  hal_nxp/components/lists
  )

if(CONFIG_BOARD_RD_RW612_BGA)
  set(drv_folder rdrw612bga)
endif()

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/flash/mflash/rdrw612bga
  hal_nxp/components/flash
  )
