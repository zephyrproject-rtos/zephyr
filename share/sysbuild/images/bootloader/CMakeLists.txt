# Copyright (c) 2022-2026 Nordic Semiconductor
# Copyright (c) 2026 Jamie McCrae
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

  set_config_string(${image} CONFIG_BOOT_SIGNATURE_KEY_FILE "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")

  if(SB_CONFIG_MCUBOOT_DIRECT_XIP_GENERATE_VARIANT)
    ExternalZephyrVariantProject_Add(
      APPLICATION ${DEFAULT_IMAGE}_slot1_variant
      SOURCE_APP ${DEFAULT_IMAGE}
      SNIPPET slot1-partition
    )
  endif()

  if(SB_CONFIG_STAGE1_BOOTLOADER_MCUBOOT)
    get_target_property(mcuboot_files ${image} IMAGE_CONF_SCRIPT)
    list(APPEND mcuboot_files "${CMAKE_SOURCE_DIR}/image_configurations/STAGE_2_MCUBOOT_image_default.cmake")
    set_target_properties(${image} PROPERTIES IMAGE_CONF_SCRIPT "${mcuboot_files}")

    ExternalZephyrProject_Add(
      APPLICATION mcuboot_stage1
      SOURCE_DIR ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/
    )

    sysbuild_add_dependencies(CONFIGURE ${image} mcuboot_stage1)
    sysbuild_add_dependencies(FLASH ${image} mcuboot_stage1)

    set_target_properties(mcuboot_stage1 PROPERTIES MCUBOOT_FOOTER_UPDATE_IMAGES ${image})
    set_config_bool(mcuboot_stage1 CONFIG_BOOT_DIRECT_XIP y)

    list(APPEND mcuboot_stage1_EXTRA_DTC_OVERLAY_FILE "${CMAKE_SOURCE_DIR}/image_configurations/STAGE_1_MCUBOOT_image_default.overlay")
    set(mcuboot_stage1_EXTRA_DTC_OVERLAY_FILE
      ${mcuboot_stage1_EXTRA_DTC_OVERLAY_FILE}
      CACHE INTERNAL "Application extra DTC overlay file" FORCE
    )

    if(SB_CONFIG_STAGE1_MCUBOOT_DIRECT_XIP_GENERATE_VARIANT)
      ExternalZephyrVariantProject_Add(
        APPLICATION ${image}_slot3_variant
        SOURCE_APP ${image}
        SNIPPET slot3-partition
      )
    endif()
  endif()
endif()
