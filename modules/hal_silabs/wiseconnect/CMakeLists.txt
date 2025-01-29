# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

set(SISDK_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk)
set(WISECONNECT_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/wiseconnect)

# Keep these values sync with
# components/device/silabs/si91x/mcu/core/chip/component/siwg917*.slcc
zephyr_compile_definitions(
  SL_SI91X_ENABLE_LITTLE_ENDIAN
  SLI_SI91X_MCU_COMMON_FLASH_MODE
  SLI_SI91X_MCU_CONFIG_RADIO_BOARD_VER2
  SLI_SI91X_MCU_CONFIG_RADIO_BOARD_BASE_VER
  SLI_SI91X_MCU_ENABLE_FLASH_BASED_EXECUTION
  SLI_SI91X_MCU_ENABLE_IPMU_APIS
  SLI_SI91X_MCU_INTERFACE
  SLI_SI917
  SLI_SI917B0
  CLOCK_ROMDRIVER_PRESENT
)

zephyr_include_directories(
  ${SISDK_DIR}/platform/common/inc
  ${SISDK_DIR}/platform/common/config
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/config
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/rom_driver/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/service/clock_manager/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/unified_api/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/cmsis_driver/config
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/cmsis_driver
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/cmsis_driver/CMSIS/Driver/Include
)

zephyr_library_sources(
  ${SISDK_DIR}/platform/common/src/sl_core_cortexm.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/src/rsi_deepsleep_soc.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/src/system_si91x.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/clock_update.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ipmu.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_pll.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ulpss_clk.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/service/clock_manager/src/sl_si91x_clock_manager.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_driver_gpio.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/src/sl_si91x_peripheral_gpio.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/ipmu_apis.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/rsi_system_config_917.c
)

zephyr_linker_sources(ROM_SECTIONS linker/code_classification_text.ld)
zephyr_linker_sources(RAMFUNC_SECTION linker/code_classification_ramfunc.ld)
