# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Add the swapped app to the build
ExternalZephyrProject_Add(
  APPLICATION swapped_app
  SOURCE_DIR ${APP_DIR}/swapped_app
)

add_dependencies(${DEFAULT_IMAGE} swapped_app)
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} swapped_app)

# Forward the FWU component ID
set_config_int(${DEFAULT_IMAGE} CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID
  ${SB_CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID})
set_config_int(swapped_app CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID
  ${SB_CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID})

# Forward the image encryption/signature configuration
foreach(image ${DEFAULT_IMAGE} swapped_app)
  set_config_string(${image} CONFIG_TFM_MCUBOOT_SIGNATURE_TYPE
    "${SB_CONFIG_SAMPLE_TFM_MCUBOOT_SIGNATURE_TYPE}")
  set_config_bool(${image} CONFIG_TFM_MCUBOOT_ENCRYPTION_NONE
    "${SB_CONFIG_SAMPLE_TFM_MCUBOOT_ENCRYPTION_NONE}")
  set_config_bool(${image} CONFIG_TFM_MCUBOOT_ENCRYPTION_RSA_OAEP
    "${SB_CONFIG_SAMPLE_TFM_MCUBOOT_ENCRYPTION_RSA_OAEP}")
  set_config_bool(${image} CONFIG_TFM_MCUBOOT_ENCRYPTION_KEY_LEN_128
    "${SB_CONFIG_SAMPLE_TFM_MCUBOOT_ENCRYPTION_KEY_LEN_128}")
  set_config_bool(${image} CONFIG_TFM_MCUBOOT_ENCRYPTION_KEY_LEN_256
    "${SB_CONFIG_SAMPLE_TFM_MCUBOOT_ENCRYPTION_KEY_LEN_256}")
endforeach()

# Validate how the embedded swapped application is stored: encrypted images must
# not contain its plaintext strings, unencrypted images must contain them.
if(SB_CONFIG_SAMPLE_TFM_MCUBOOT_ENCRYPTION_NONE)
  set(swapped_app_expect present)
else()
  set(swapped_app_expect absent)
endif()

ExternalProject_Get_Property(${DEFAULT_IMAGE} BINARY_DIR)
add_custom_target(check_swapped_app_encryption ALL
  COMMAND ${CMAKE_COMMAND}
    -DBIN_FILE=${BINARY_DIR}/zephyr/zephyr.elf
    "-DSEARCH_STRING=Swapped application booted"
    -DEXPECT=${swapped_app_expect}
    -P ${CMAKE_CURRENT_LIST_DIR}/check_string_presence.cmake
  VERBATIM
)
add_dependencies(check_swapped_app_encryption ${DEFAULT_IMAGE})
