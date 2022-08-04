# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Include MCUboot if enabled.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  ExternalZephyrProject_Add(
    APPLICATION mcuboot
    SOURCE_DIR ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/
  )
  # MCUBoot default configuration is to perform a full chip erase.
  # Placing MCUBoot first in list to ensure it is flashed before other images.
  set(IMAGES "mcuboot" ${IMAGES} PARENT_SCOPE)
endif()
