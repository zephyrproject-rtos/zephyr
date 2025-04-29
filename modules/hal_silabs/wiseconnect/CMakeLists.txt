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
  ${WISECONNECT_DIR}/components/board/silabs/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/config
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/inc
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/config
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
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_pwm.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_sysrtc.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ipmu.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_pll.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ulpss_clk.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_wwdt.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/service/clock_manager/src/sl_si91x_clock_manager.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_driver_gpio.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_pwm.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/src/sl_si91x_peripheral_gpio.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/ipmu_apis.c
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/rsi_system_config_917.c
)

zephyr_library_sources_ifdef(CONFIG_ENTROPY_SILABS_SIWX91X
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_rng.c
)

zephyr_compile_definitions_ifdef(CONFIG_DMA_SILABS_SIWX91X
  UDMA_ROMDRIVER_PRESENT
)

zephyr_library_sources_ifdef(CONFIG_DMA_SILABS_SIWX91X
  ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/rom_driver/src/rsi_rom_table_si91x.c
)

if(CONFIG_WIFI_SILABS_SIWX91X)
  zephyr_library_sources(
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/sl_net/src/sl_si91x_net_credentials.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/sl_net/src/sl_si91x_net_internal_stack.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/sl_net/src/sl_net_si91x_integration_handler.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/sl_net/src/sl_net_rsi_utility.c
    ${WISECONNECT_DIR}/components/protocol/wifi/src/sl_wifi_basic_credentials.c
    ${WISECONNECT_DIR}/components/service/network_manager/si91x/sl_net_si91x.c
    ${WISECONNECT_DIR}/components/service/network_manager/src/sl_net_basic_profiles.c
    ${WISECONNECT_DIR}/components/service/network_manager/src/sl_net_credentials.c
  )
  zephyr_compile_definitions_ifdef(CONFIG_NET_IPV6
    SLI_SI91X_ENABLE_IPV6
  )
  zephyr_compile_definitions_ifdef(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
    SLI_SI91X_OFFLOAD_NETWORK_STACK
    SLI_SI91X_SOCKETS
  )
  zephyr_include_directories_ifdef(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
    # Needed for <sys/socket.h>
    ${ZEPHYR_BASE}/include/zephyr/posix
  )
  zephyr_library_sources_ifdef(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/socket/src/sl_si91x_socket_utility.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/asynchronous_socket/src/sl_si91x_socket.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/sl_net/src/sl_net_si91x_callback_framework.c
    ${WISECONNECT_DIR}/components/service/network_manager/src/sl_net.c
  )
endif() # CONFIG_WIFI_SILABS_SIWX91X

if(CONFIG_BT_SILABS_SIWX91X)
  zephyr_compile_definitions(
    SLI_SI91X_ENABLE_BLE
  )
  zephyr_include_directories(
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ble/inc
  )
  zephyr_library_sources(
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ble/src/rsi_bt_ble.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ble/src/rsi_common_apis.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ble/src/rsi_utils.c
  )
endif() # CONFIG_BT_SILABS_SIWX91X

if(CONFIG_WISECONNECT_NETWORK_STACK)
  zephyr_compile_definitions(
    SLI_SI91X_ENABLE_OS
    SL_SI91X_SI917_RAM_MEM_CONFIG=1
    SL_WIFI_COMPONENT_INCLUDED # Depite de the name, required for everything
  )
  zephyr_include_directories(
    # FIXME: find why this directory is not included when CMSIS_RTOS_V2=y
    ${ZEPHYR_BASE}/include/zephyr/portability
    ${SISDK_DIR}/platform/emlib/inc
    ${WISECONNECT_DIR}/resources/defaults
    ${WISECONNECT_DIR}/components/common/inc
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/inc
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/sl_net/inc
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/socket/inc
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/asynchronous_socket/inc
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ahb_interface/inc
    ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/core/chip/config
    ${WISECONNECT_DIR}/components/protocol/wifi/inc
    ${WISECONNECT_DIR}/components/service/network_manager/inc
  )
  zephyr_library_sources(
    ${WISECONNECT_DIR}/components/common/src/sl_utility.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_ram.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_rom.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ahb_interface/src/sli_siwx917_soc.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ahb_interface/src/sl_platform.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ahb_interface/src/sl_platform_wireless.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/ahb_interface/src/sl_si91x_bus.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/host_mcu/si91x/siwx917_soc_ncp_host.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/memory/malloc_buffers.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/src/sl_rsi_utility.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/src/sl_si91x_driver.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/wireless/threading/sli_si91x_multithreaded.c
    ${WISECONNECT_DIR}/components/protocol/wifi/si91x/sl_wifi.c
    ${WISECONNECT_DIR}/components/protocol/wifi/src/sl_wifi_callback_framework.c
  )
endif() # CONFIG_WISECONNECT_NETWORK_STACK

if(CONFIG_SOC_SILABS_SLEEPTIMER)
  zephyr_include_directories(
    ${SISDK_DIR}/platform/service/sleeptimer/inc
    ${SISDK_DIR}/platform/service/sleeptimer/src
    ${SISDK_DIR}/platform/service/sleeptimer/config
  )

  zephyr_library_sources(
    ${SISDK_DIR}/platform/service/sleeptimer/src/sl_sleeptimer.c
    ${WISECONNECT_DIR}/components/device/silabs/si91x/mcu/drivers/service/sleeptimer/src/sl_sleeptimer_hal_si91x_sysrtc.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_SLEEPTIMER_PRESENT
    SL_CODE_COMPONENT_SLEEPTIMER=sleeptimer
    SL_CODE_COMPONENT_HAL_SYSRTC=hal_sysrtc
  )
endif() # CONFIG_SOC_SILABS_SLEEPTIMER

zephyr_linker_sources(ROM_SECTIONS linker/code_classification_text.ld)
zephyr_linker_sources(RAMFUNC_SECTION linker/code_classification_ramfunc.ld)
