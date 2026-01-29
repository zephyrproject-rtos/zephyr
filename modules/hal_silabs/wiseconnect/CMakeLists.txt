# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

set(SISDK       ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk)
set(WISECONNECT ${ZEPHYR_HAL_SILABS_MODULE_DIR}/wiseconnect)

function(wiseconnect_get_ram_config node_var out_var)
  # Get reg property from device tree node
  dt_nodelabel(node_path NODELABEL "${node_var}")
  dt_prop(reg_property PATH "${node_path}" PROPERTY "reg")
  list(GET reg_property 1 size_bytes)
  math(EXPR size_kb "${size_bytes} / 1024")

  # Set output variable based on size
  if(size_kb EQUAL 195)
    set(${out_var} 1 PARENT_SCOPE)
  elseif(size_kb EQUAL 255)
    set(${out_var} 2 PARENT_SCOPE)
  elseif(size_kb EQUAL 319)
    set(${out_var} 3 PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Unsupported RAM size config: ${size_kb}KB")
  endif()
endfunction()

# Keep these values sync with
# components/device/silabs/si91x/mcu/core/chip/component/siwg917*.slcc
zephyr_compile_definitions(
  SL_SUPPRESS_DEPRECATION_WARNINGS_WISECONNECT_3_5
  SL_SI91X_ENABLE_LITTLE_ENDIAN
  SLI_CODE_CLASSIFICATION_DISABLE
  SLI_SI91X_MCU_COMMON_FLASH_MODE
  SLI_SI91X_MCU_CONFIG_RADIO_BOARD_VER2
  SLI_SI91X_MCU_CONFIG_RADIO_BOARD_BASE_VER
  SLI_SI91X_MCU_ENABLE_FLASH_BASED_EXECUTION
  SLI_SI91X_MCU_ENABLE_IPMU_APIS
  SLI_SI91X_MCU_INTERFACE
  SLI_SI917
  SLI_SI917B0
)

zephyr_include_directories(
  ${SISDK}/platform/common/config
  ${SISDK}/platform/common/inc
  ${SISDK}/platform/emlib/inc
  ${SISDK}/platform/service/mem_pool/inc
  ${SISDK}/platform/service/sleeptimer/config
  ${SISDK}/platform/service/sleeptimer/inc
  # Wiseconnect do not provide generic RTE_Device_917.h. However, all the boards share more-or-less
  # the same definitions. So we could take any of them. In addition, this file is only required for
  # the compilation, but none the symbols are normally used by Zephyr (it is required to compile
  # CMSIS API which is not not used).
  ${WISECONNECT}/components/board/silabs/config/brd4342a
  ${WISECONNECT}/components/common/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/config
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/common/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/config
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/cmsis_driver
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/cmsis_driver/CMSIS/Driver/Include
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/cmsis_driver/config
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/rom_driver/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/clock_manager/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/power_manager/config
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/power_manager/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_api/config
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_api/config/sl_i2s_config
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_api/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/inc
  ${WISECONNECT}/components/device/silabs/si91x/mcu/hal/inc
  ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/inc
  ${WISECONNECT}/components/device/silabs/si91x/wireless/asynchronous_socket/inc
  ${WISECONNECT}/components/device/silabs/si91x/wireless/ble/inc
  ${WISECONNECT}/components/device/silabs/si91x/wireless/firmware_upgrade
  ${WISECONNECT}/components/device/silabs/si91x/wireless/inc
  ${WISECONNECT}/components/device/silabs/si91x/wireless/sl_net/inc
  ${WISECONNECT}/components/device/silabs/si91x/wireless/socket/inc
  ${WISECONNECT}/components/protocol/wifi/inc
  ${WISECONNECT}/components/service/network_manager/inc
  ${WISECONNECT}/components/sli_wifi_command_engine/inc
  ${WISECONNECT}/resources/defaults
)

zephyr_library_sources(
  ${SISDK}/platform/common/src/sl_core_cortexm.c
  ${SISDK}/platform/service/mem_pool/src/sl_mem_pool.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/src/rsi_deepsleep_soc.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/src/system_si91x.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/clock_update.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/rom_driver/src/rsi_rom_table_si91x.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ipmu.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/hal/src/sl_si91x_hal_soc_soft_reset.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/ipmu_apis.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/rsi_system_config_917.c
)

zephyr_library_sources_ifdef(CONFIG_ADC_SILABS_SIWX91X
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/aux_reference_volt_config.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_bod.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_dac.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_adc.c
)

zephyr_compile_definitions_ifdef(CONFIG_BT_SILABS_SIWX91X
  SLI_SI91X_ENABLE_BLE
)
zephyr_library_sources_ifdef(CONFIG_BT_SILABS_SIWX91X
  ${WISECONNECT}/components/device/silabs/si91x/wireless/ble/src/rsi_bt_ble.c
  ${WISECONNECT}/components/device/silabs/si91x/wireless/ble/src/rsi_common_apis.c
  ${WISECONNECT}/components/device/silabs/si91x/wireless/ble/src/rsi_utils.c
  ${WISECONNECT}/components/device/silabs/si91x/wireless/ble/src/sl_si91x_ble.c
)

# FIXME: Replace sli_si91x_clock_manager.c by native Zephyr access to the clocks
if(CONFIG_CLOCK_CONTROL_SILABS_SIWX91X OR CONFIG_SOC_SIWX91X_PM_BACKEND_PMGR)
  zephyr_compile_definitions(
    ULPSS_CLOCK_ROMDRIVER_PRESENT # replace rsi_ulpss_clk.c
    CLOCK_ROMDRIVER_PRESENT # replace rsi_pll.c
  )
  zephyr_library_sources(
    # Some functions are not provided by ULPSS_CLOCK_ROMDRIVER_PRESENT
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ulpss_clk.c
    # Some functions are not provided by CLOCK_ROMDRIVER_PRESENT
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_pll.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/clock_manager/src/sl_si91x_clock_manager.c
  )
endif()

# FIXME: Only silabs,rtc-siwx91x should access to sysrtc
if(CONFIG_CLOCK_CONTROL_SILABS_SIWX91X OR CONFIG_RTC_SILABS_SIWX91X)
  zephyr_library_sources(
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_rtc.c
  )
endif()

# FIXME: Only silabs,rtc-siwx91x should access to sysrtc
if(CONFIG_CLOCK_CONTROL_SILABS_SIWX91X OR CONFIG_SILABS_SISDK_SLEEPTIMER)
  zephyr_library_sources(
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_sysrtc.c
  )
endif()

# FIXME: These services should rely in Zephyr RTC API
if(CONFIG_CLOCK_CONTROL_SILABS_SIWX91X OR CONFIG_RTC_SILABS_SIWX91X OR
   CONFIG_SILABS_SISDK_SLEEPTIMER OR CONFIG_SOC_SIWX91X_PM_BACKEND_PMGR)
  zephyr_library_sources(
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_time_period.c
  )
endif()

zephyr_compile_definitions_ifdef(CONFIG_DMA_SILABS_SIWX91X
  UDMA_ROMDRIVER_PRESENT # replace rsi_udma.c and rsi_udma_wrapper.c
)

zephyr_compile_definitions_ifdef(CONFIG_DMA_SILABS_SIWX91X_GPDMA
  GPDMA_ROMDRIVER_PRESENT # replace rsi_gpdma.c
)

zephyr_compile_definitions_ifdef(CONFIG_ENTROPY_SILABS_SIWX91X
  RNG_ROMDRIVER_PRESENT # replace rsi_rng.c
)

zephyr_library_sources_ifdef(CONFIG_GPIO_SILABS_SIWX91X
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_driver_gpio.c
)

zephyr_compile_definitions_ifdef(CONFIG_MEMC_SILABS_SIWX91X_QSPI
  QSPI_ROMDRIVER_PRESENT # replace rsi_qspi.c
)
zephyr_library_sources_ifdef(CONFIG_MEMC_SILABS_SIWX91X_QSPI
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_psram.c
)

zephyr_library_sources_ifdef(CONFIG_PWM_SILABS_SIWX91X
  # Do not use PWM_ROMDRIVER_PRESENT since ROM of siwx917b0 is buggy
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_pwm.c
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_pwm.c
)

if(CONFIG_PINCTRL_SILABS_SIWX91X OR CONFIG_GPIO_SILABS_SIWX91X)
  # FIXME: split it between PINCTRL and GPIO
  zephyr_library_sources(
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/src/sl_si91x_peripheral_gpio.c
  )
endif()

zephyr_library_sources_ifdef(CONFIG_RTC_SILABS_SIWX91X
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_calendar.c
)

zephyr_library_sources_ifdef(CONFIG_WDT_SILABS_SIWX91X
  ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_wwdt.c
)

if(CONFIG_WIFI_SILABS_SIWX91X)
  zephyr_library_sources(
    ${WISECONNECT}/components/device/silabs/si91x/wireless/sl_net/src/sl_si91x_net_internal_stack.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/sl_net/src/sl_net_si91x_integration_handler.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/sl_net/src/sl_net_rsi_utility.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/sl_net/src/sli_net_si91x_utility.c
    ${WISECONNECT}/components/protocol/wifi/src/sl_wifi_basic_credentials.c
    ${WISECONNECT}/components/service/network_manager/src/sl_net_basic_profiles.c
    ${WISECONNECT}/components/service/network_manager/src/sl_net_credentials.c
  )
  zephyr_compile_definitions(
    SLI_SI91X_CONFIG_WIFI6_PARAMS
    SLI_SI91X_ENABLE_TWT_FEATURE
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
    ${WISECONNECT}/components/device/silabs/si91x/wireless/socket/src/sl_si91x_socket_utility.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/asynchronous_socket/src/sl_si91x_socket.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/sl_net/src/sl_net_si91x_callback_framework.c
    ${WISECONNECT}/components/service/network_manager/src/sl_net.c
  )
endif() # CONFIG_WIFI_SILABS_SIWX91X


if(CONFIG_SILABS_SIWX91X_NWP)
  wiseconnect_get_ram_config("sram0" RAM_MEM_CONFIG)
  zephyr_compile_definitions(
    SLI_SI91X_ENABLE_OS
    SL_SI91X_SI917_RAM_MEM_CONFIG=${RAM_MEM_CONFIG}
    SL_WIFI_COMPONENT_INCLUDED # Depite de the name, required for everything
  )
  zephyr_include_directories(
    # FIXME: find why this directory is not included when CMSIS_RTOS_V2=y
    ${ZEPHYR_BASE}/include/zephyr/portability
  )
  zephyr_library_sources(
    nwp_fw_version.c
    ${WISECONNECT}/components/common/src/sl_utility.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_ram.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_rom.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/sli_siwx917_soc.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/sl_platform.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/sl_platform_wireless.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/sl_si91x_bus.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/host_mcu/si91x/siwx917_soc_ncp_host.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/memory/mem_pool_buffer_quota.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/src/sl_rsi_utility.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/src/sl_si91x_driver.c
    ${WISECONNECT}/components/protocol/wifi/si91x/sl_wifi.c
    ${WISECONNECT}/components/protocol/wifi/src/sl_wifi_callback_framework.c
    ${WISECONNECT}/components/service/network_manager/src/sli_net_common_utility.c
    ${WISECONNECT}/components/sli_si91x_wifi_event_handler/src/sli_si91x_wifi_event_handler.c
    ${WISECONNECT}/components/sli_wifi_command_engine/src/sli_wifi_command_engine.c
  )
  zephyr_library_sources_ifdef(CONFIG_SIWX91X_FIRMWARE_UPGRADE
    ${WISECONNECT}/components/device/silabs/si91x/wireless/firmware_upgrade/firmware_upgradation.c
  )
  zephyr_include_directories(.)
endif() # CONFIG_SILABS_SIWX91X_NWP

if(CONFIG_SILABS_SISDK_SLEEPTIMER)
  zephyr_include_directories(
    ${SISDK}/platform/service/sleeptimer/src
  )
  zephyr_library_sources(
    ${SISDK}/platform/service/sleeptimer/src/sl_sleeptimer.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/sleeptimer/src/sl_sleeptimer_hal_si91x_sysrtc.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_SLEEPTIMER_PRESENT
  )
  zephyr_code_relocate(FILES
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/sleeptimer/src/sl_sleeptimer_hal_si91x_sysrtc.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_sysrtc.c
    ${SISDK}/platform/service/sleeptimer/src/sl_sleeptimer.c
    ${ZEPHYR_BASE}/arch/arm/core/cortex_m/isr_wrapper.c
    LOCATION RAM
  )
endif() # CONFIG_SILABS_SISDK_SLEEPTIMER

if(CONFIG_SOC_SIWX91X_PM_BACKEND_PMGR)
  zephyr_library_sources(
    ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/src/rsi_ps_ram_func.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/sl_si91x_m4_ps.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/power_manager/src/sl_si91x_power_manager.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/power_manager/src/sli_si91x_power_manager.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/clock_manager/src/sli_si91x_clock_manager.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_temp_sensor.c
    ${SISDK}/platform/common/src/sl_slist.c
  )
  zephyr_compile_definitions(
    SL_SI91X_TICKLESS_MODE
    SL_SLEEP_TIMER
    SLI_WIRELESS_COMPONENT_PRESENT
  )
  zephyr_code_relocate(FILES
    ${WISECONNECT}/components/device/silabs/si91x/mcu/core/chip/src/rsi_deepsleep_soc.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/power_manager/src/sl_si91x_power_manager.c
    ${WISECONNECT}/components/device/silabs/si91x/mcu/drivers/service/power_manager/src/sli_si91x_power_manager.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_ram.c
    ${WISECONNECT}/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_rom.c
    ${ZEPHYR_BASE}/drivers/gpio/gpio_silabs_siwx91x.c
    ${ZEPHYR_BASE}/drivers/gpio/gpio_silabs_siwx91x_uulp.c
    LOCATION RAM
  )
endif() # CONFIG_SOC_SIWX91X_PM_BACKEND_PMGR
