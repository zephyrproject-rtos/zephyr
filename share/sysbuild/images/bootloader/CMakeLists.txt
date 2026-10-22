# Copyright (c) 2022-2026 Nordic Semiconductor
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

  # Link the footer size output of this MCUboot build with the main sysbuild image
  set_target_properties(${image} PROPERTIES MCUBOOT_FOOTER_UPDATE_IMAGES ${DEFAULT_IMAGE})

  sysbuild_mcuboot_resolve_signature_key_files(
    resolved_key_files "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}"
  )
  set_config_string(${image} CONFIG_BOOT_SIGNATURE_KEY_FILE "${resolved_key_files}")

  if(SB_CONFIG_MCUBOOT_DIRECT_XIP_GENERATE_VARIANT)
    ExternalZephyrVariantProject_Add(
      APPLICATION ${DEFAULT_IMAGE}_slot1_variant
      SOURCE_APP ${DEFAULT_IMAGE}
      SNIPPET slot1-partition
    )
  endif()
endif()
