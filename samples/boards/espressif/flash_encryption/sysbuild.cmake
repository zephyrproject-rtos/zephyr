# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# Apply the same flash layout (write-block-size 32 + partitions) to MCUboot.
# Do not use sysbuild/mcuboot/ for this: that directory becomes
# APPLICATION_CONFIG_DIR and requires a prj.conf that would replace MCUboot's.
# Image vars must be CACHE INTERNAL so sysbuild exports them to the image.

set(FLASH_ENCRYPTION_BOARD_DIR ${CMAKE_CURRENT_LIST_DIR}/boards)

if("${BOARD}" STREQUAL "esp32s3_devkitc")
  list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE
       ${FLASH_ENCRYPTION_BOARD_DIR}/esp32s3_devkitc_procpu_flash.dtsi
  )
elseif("${BOARD}" STREQUAL "esp32c5_devkitc")
  list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE
       ${FLASH_ENCRYPTION_BOARD_DIR}/esp32c5_devkitc_hpcore_flash.dtsi
  )
endif()

if(DEFINED mcuboot_EXTRA_DTC_OVERLAY_FILE)
  list(REMOVE_DUPLICATES mcuboot_EXTRA_DTC_OVERLAY_FILE)
  set(mcuboot_EXTRA_DTC_OVERLAY_FILE "${mcuboot_EXTRA_DTC_OVERLAY_FILE}"
      CACHE INTERNAL "Flash encryption DT overlay for MCUboot"
  )
endif()
