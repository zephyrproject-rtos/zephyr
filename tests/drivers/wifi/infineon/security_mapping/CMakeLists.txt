# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(airoc_wifi_security_mapping_test)

set(airoc_dir ${ZEPHYR_BASE}/drivers/wifi/infineon)
set(hal_infineon_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR})

target_sources(testbinary PRIVATE
  ${airoc_dir}/airoc_wifi_security.c
  src/main.c
)

target_include_directories(testbinary PRIVATE
  ${airoc_dir}
  ${hal_infineon_dir}/whd-expansion/WHD/COMPONENT_WIFI6/inc
  ${hal_infineon_dir}/whd-expansion/WHD/COMMON/inc
  ${hal_infineon_dir}/core-lib/include
)

# Mirrors drivers/wifi/infineon/CMakeLists.txt's defines for this file, set
# directly since a plain unittest build has no Kconfig to generate them:
# WHD_USE_CUSTOM_HAL_IMPL skips whd_types.h's cyhal/mtb_hal includes;
# CONFIG_AIROC_WIFI_BUS_SDIO selects the interface type cybsp.h expects.
target_compile_definitions(testbinary PRIVATE
  WHD_USE_CUSTOM_HAL_IMPL
  CONFIG_AIROC_WIFI_BUS_SDIO=1
)
