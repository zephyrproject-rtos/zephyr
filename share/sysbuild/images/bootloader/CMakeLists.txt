# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Include MCUboot if enabled.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set(image mcuboot)
  if(DEFINED BOARD_REVISION)
    set(board_str ${BOARD}@${BOARD_REVISION})
  else()
    set(board_str ${BOARD})
  endif()
  ExternalZephyrProject_Add(
    APPLICATION ${image}
    SOURCE_DIR ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/
    APP_TYPE BOOTLOADER
    BOARD ${board_str}
  )
  # Place MCUBoot first in list to ensure it is configured and flashed before other images.
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${image})
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} ${image})

  set_config_string(${image} CONFIG_BOOT_SIGNATURE_KEY_FILE "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  set_config_bool(${image} CONFIG_BOOT_ENCRYPT_IMAGE "${SB_CONFIG_BOOT_ENCRYPTION}")
  if(SB_CONFIG_BOOT_ENCRYPTION)
    set_config_string(${image} CONFIG_BOOT_ENCRYPTION_KEY_FILE "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")
  endif()
endif()
