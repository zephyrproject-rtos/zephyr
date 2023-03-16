# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# This sysbuild CMake file sets the sysbuild controlled settings as properties
# on Zephyr images.

get_target_property(image_board ${image} BOARD)
if((NOT image_board) OR ("${image_BOARD}" STREQUAL "${BOARD}"))
  get_target_property(${image}_APP_TYPE ${image} APP_TYPE)
  if(NOT "${${image}_APP_TYPE}" STREQUAL "BOOTLOADER")
    set_config_bool(${image} CONFIG_BOOTLOADER_MCUBOOT "${SB_CONFIG_BOOTLOADER_MCUBOOT}")
    set_config_string(${image} CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
                               "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}"
    )
  endif()

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    if("${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
      set_config_bool(${image} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE y)
    else()
      set_config_bool(${image} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE n)
    endif()
  endif()
endif()
