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
  # Place MCUBoot first in list to ensure it is configured and flashed before other images.
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${image})
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} ${image})

  set_config_string(${image} CONFIG_BOOT_SIGNATURE_KEY_FILE "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  set_config_bool(${image} CONFIG_BOOT_ENCRYPT_IMAGE "${SB_CONFIG_BOOT_ENCRYPTION}")
  if(SB_CONFIG_BOOT_ENCRYPTION)
    set_config_string(${image} CONFIG_BOOT_ENCRYPTION_KEY_FILE "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")
  endif()

# Include VPR laucher if enabled.
elseif(SB_CONFIG_BOOTLOADER_VPR_LAUNCHER)
  if((${BOARD} STREQUAL "nrf54h20dk") AND (${BOARD_QUALIFIERS} STREQUAL "/nrf54h20/cpuppr"))
    set(core nrf54h20dk/nrf54h20/cpuapp)
  else()
    message(FATAL_ERROR "Unsupported target for VPR launcher")
  endif()

  set(image vpr_launcher)
  ExternalZephyrProject_Add(
    APPLICATION ${image}
    SOURCE_DIR ${ZEPHYR_BASE}/samples/basic/minimal
    APP_TYPE BOOTLOADER
    BOARD ${core}
  )

  if(SB_CONFIG_BOOT_XIP)
    sysbuild_cache_set(VAR ${image}_SNIPPET APPEND REMOVE_DUPLICATES nordic-ppr-xip)
  else()
    sysbuild_cache_set(VAR ${image}_SNIPPET APPEND REMOVE_DUPLICATES nordic-ppr)
  endif()
endif()
