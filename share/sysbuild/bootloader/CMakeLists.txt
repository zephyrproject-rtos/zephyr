# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Include MCUboot if enabled.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  ExternalZephyrProject_Add(
    APPLICATION mcuboot
    SOURCE_DIR ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/
  )
endif()

# MCUboot should be flashed first, in case the user configures it with
# CONFIG_ZEPHYR_TRY_MASS_ERASE=y. Add a 'mass_erase' group to account for this.
# It can be removed once that config is removed (it is already deprecated).
ExternalZephyrProject_Group(mass_erase INCLUDE mcuboot)
sysbuild_add_dependencies(GROUP FLASH all mass_erase)
