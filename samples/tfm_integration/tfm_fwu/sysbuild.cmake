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
