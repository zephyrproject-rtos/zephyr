# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/wifi_nxp
  ${CMAKE_CURRENT_BINARY_DIR}/middleware/wifi_nxp
)

add_subdirectory(
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/osa
  ${CMAKE_CURRENT_BINARY_DIR}/components/osa
)

add_subdirectory(
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk-ng/components/wifi_bt_module
  ${CMAKE_CURRENT_BINARY_DIR}/components/wifi_bt_module
)

zephyr_compile_definitions_ifdef(CONFIG_NXP_WIFI_CUSTOM_HOST
  SDK_COMPONENT_DEPENDENCY_FSL_COMMON=0
)

set(CONFIG_MCUX_COMPONENT_component.wifi_bt_module.tx_pwr_limits ON)
set(CONFIG_MCUX_COMPONENT_component.osa_template_config ON)
set(CONFIG_MCUX_COMPONENT_component.osa_zephyr ON)
set(CONFIG_MCUX_COMPONENT_component.osa_interface ON)
