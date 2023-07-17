# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Include MCUboot if enabled.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set(image mcuboot)
  ExternalZephyrProject_Add(
    APPLICATION ${image}
    SOURCE_DIR ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/
    APP_TYPE BOOTLOADER
  )
  # MCUBoot default configuration is to perform a full chip erase.
  # Placing MCUBoot first in list to ensure it is flashed before other images.
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} ${image})

  set_config_string(${image} CONFIG_BOOT_SIGNATURE_KEY_FILE "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  set_config_bool(${image} CONFIG_BOOT_ENCRYPT_IMAGE "${SB_CONFIG_BOOT_ENCRYPTION}")
  if(SB_CONFIG_BOOT_ENCRYPTION)
    set_config_string(${image} CONFIG_BOOT_ENCRYPTION_KEY_FILE "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")
  endif()
endif()
