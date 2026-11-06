# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(NOT "${SB_CONFIG_APP_UF2_APPLICATION}" STREQUAL "")
  ExternalZephyrProject_Add(
    APPLICATION application
    SOURCE_DIR ${SB_CONFIG_APP_UF2_APPLICATION}
  )

  # The application gets UF2 output to test the bootloader. A mapped partition
  # needs HEX for the conversion, and the application must be linked into slot0.
  set_config_bool(application CONFIG_BUILD_OUTPUT_UF2 y)
  set_config_bool(application CONFIG_BUILD_OUTPUT_HEX y)
  set_config_bool(application CONFIG_USE_DT_CODE_PARTITION y)

  if(NOT "${SB_CONFIG_APP_UF2_FAMILY_ID}" STREQUAL "")
    set_config_string(application CONFIG_BUILD_OUTPUT_UF2_FAMILY_ID
                      "${SB_CONFIG_APP_UF2_FAMILY_ID}")
  endif()

  # Flash the bootloader first, the application only occupies slot0
  sysbuild_add_dependencies(FLASH application ${DEFAULT_IMAGE})
endif()
