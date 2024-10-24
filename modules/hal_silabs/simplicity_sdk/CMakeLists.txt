# CMake integration for Simplicity SDK
#
# Copyright (c) 2017, Christian Taedcke
# Copyright (c) 2021, Safran Passenger Innovations Germany GmbH
# Copyright (c) 2022, Antmicro <www.antmicro.com>
# Copyright (c) 2024, Silicon Laboratories Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(EMLIB_DIR      ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/emlib)
set(COMMON_DIR     ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/common)
set(DEVICE_DIR     ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/Device)
set(RADIO_DIR      ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/radio)
set(SECURITY_DIR   ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/security)
set(SERVICE_DIR    ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/service)
set(PERIPHERAL_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/peripheral)
set(BLUETOOTH_DIR  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/protocol/bluetooth)
set(BLOBS_DIR      ${ZEPHYR_HAL_SILABS_MODULE_DIR}/zephyr/blobs/simplicity_sdk)

# Translate the SoC name and part number into the gecko device and cpu name
# respectively.
string(TOUPPER ${CONFIG_SOC_SERIES} SILABS_DEVICE_FAMILY)

# Get SoC series number, i.e. translate e.g. efr32bg22 -> 22
string(SUBSTRING ${CONFIG_SOC_SERIES} 7 2 SILABS_DEVICE_FAMILY_NUMBER)

set(SILABS_DEVICE_PART_NUMBER ${CONFIG_SOC_PART_NUMBER})

function(add_prebuilt_library lib_name prebuilt_path)
  add_library(${lib_name} STATIC IMPORTED GLOBAL)
  set_target_properties(${lib_name} PROPERTIES
    IMPORTED_LOCATION ${BLOBS_DIR}/${prebuilt_path}
  )
  zephyr_link_libraries(${lib_name})
endfunction()

if(CONFIG_SOC_GECKO_HAS_RADIO)
  zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
    ${RADIO_DIR}/rail_lib/plugin/pa-conversions/efr32xg${SILABS_DEVICE_FAMILY_NUMBER}/config
    ${RADIO_DIR}/rail_lib/chip/efr32/efr32xg2x
  )

  zephyr_include_directories(
    ${RADIO_DIR}/rail_lib/common
    ${RADIO_DIR}/rail_lib/plugin/pa-conversions
    ${BLUETOOTH_DIR}/bgstack/ll/inc
  )

  # sl_protocol_crypto
  zephyr_library_sources_ifdef(CONFIG_BT_SILABS_EFR32
    ${SECURITY_DIR}/sl_component/sl_protocol_crypto/src/sli_radioaes_management.c
    ${SECURITY_DIR}/sl_component/sl_protocol_crypto/src/sli_protocol_crypto_radioaes.c
  )

  if(CONFIG_BT_SILABS_EFR32)
    # prebuilt libs
    add_prebuilt_library(liblinklayer protocol/bluetooth/bgstack/ll/lib/libbluetooth_controller_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}_gcc_release.a)
    add_prebuilt_library(libbgcommon  protocol/bluetooth/bgcommon/lib/build/gcc/cortex-m33/bgcommon/release/libbgcommon.a)

    # link mbedTLS
    if(CONFIG_MBEDTLS)
      zephyr_link_libraries(mbedTLS)
    endif()
  endif()

  if(CONFIG_SOC_GECKO_USE_RAIL)
    # rail
    zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/pa-conversions/pa_curves_efr32.c)
    zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c)

    # prebuilt libs
    add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}_gcc_release.a)

    zephyr_include_directories_ifdef(CONFIG_SOC_GECKO_CUSTOM_RADIO_PHY
      ${RADIO_DIR}/rail_lib/protocol/ble
      ${RADIO_DIR}/rail_lib/protocol/ieee802154
      ${RADIO_DIR}/rail_lib/protocol/zwave
      ${RADIO_DIR}/rail_lib/protocol/sidewalk
      ${RADIO_DIR}/rail_lib/plugin/rail_util_protocol
      ${RADIO_DIR}/rail_lib/plugin/rail_util_protocol/config/efr32xg${SILABS_DEVICE_FAMILY_NUMBER}
    )
    zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_CUSTOM_RADIO_PHY
      ${RADIO_DIR}/rail_lib/plugin/rail_util_protocol/sl_rail_util_protocol.c
    )
  endif()
endif()

zephyr_include_directories(
  config
  ${DEVICE_DIR}/SiliconLabs/${SILABS_DEVICE_FAMILY}/Include
  ${COMMON_DIR}/config
  ${COMMON_DIR}/inc
  ${EMLIB_DIR}/inc
  ${PERIPHERAL_DIR}/inc
  ${SERVICE_DIR}/clock_manager/inc
  ${SERVICE_DIR}/device_init/inc
  ${SERVICE_DIR}/device_manager/inc
  ${SERVICE_DIR}/hfxo_manager/config
  ${SERVICE_DIR}/hfxo_manager/inc
  ${SERVICE_DIR}/hfxo_manager/src
  ${SERVICE_DIR}/memory_manager/inc
  ${SERVICE_DIR}/memory_manager/profiler/inc
  ${SERVICE_DIR}/power_manager/config
  ${SERVICE_DIR}/power_manager/inc
  ${SERVICE_DIR}/power_manager/src
  ${SERVICE_DIR}/sleeptimer/config
  ${SERVICE_DIR}/sleeptimer/inc
  ${SERVICE_DIR}/sleeptimer/src
  ${SECURITY_DIR}/sl_component/sl_protocol_crypto/src
  ${BOARD_DIR}
)

zephyr_compile_definitions(
  ${SILABS_DEVICE_PART_NUMBER}
)

zephyr_library_sources(
  ${DEVICE_DIR}/SiliconLabs/${SILABS_DEVICE_FAMILY}/Source/system_${CONFIG_SOC_SERIES}.c
  ${EMLIB_DIR}/src/em_system.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager_hal_s2.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager_init.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager_init_hal_s2.c
  ${SERVICE_DIR}/device_manager/devices/sl_device_peripheral_hal_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}.c
  ${SERVICE_DIR}/device_manager/gpios/sl_device_gpio_common.c
  ${SERVICE_DIR}/device_manager/src/sl_device_clock.c
  ${SERVICE_DIR}/device_manager/src/sl_device_gpio.c
  ${SERVICE_DIR}/device_manager/src/sl_device_peripheral.c
  ${SERVICE_DIR}/memory_manager/profiler/src/sli_memory_profiler_stubs.c
)

if(NOT SILABS_DEVICE_FAMILY_NUMBER EQUAL "21")
  zephyr_library_sources(
    ${SERVICE_DIR}/device_manager/clocks/sl_device_clock_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}.c
  )
endif()

# Sleeptimer
if(CONFIG_SOC_SILABS_SLEEPTIMER)
  zephyr_library_sources(
    ${PERIPHERAL_DIR}/src/sl_hal_sysrtc.c
    ${SERVICE_DIR}/sleeptimer/src/sl_sleeptimer_hal_rtcc.c
    ${SERVICE_DIR}/sleeptimer/src/sl_sleeptimer_hal_sysrtc.c
    ${SERVICE_DIR}/sleeptimer/src/sl_sleeptimer.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_SLEEPTIMER_PRESENT
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_IADC         ${EMLIB_DIR}/src/em_iadc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_BURTC        ${EMLIB_DIR}/src/em_burtc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_CMU          ${EMLIB_DIR}/src/em_cmu.c)

# Device Init
if(CONFIG_SOC_GECKO_DEV_INIT)
  zephyr_library_sources_ifdef(CONFIG_DT_HAS_SILABS_SERIES2_DCDC_ENABLED
    ${SERVICE_DIR}/device_init/src/sl_device_init_dcdc_s2.c
  )
endif()

# Power Manager
if(CONFIG_SOC_GECKO_PM_BACKEND_PMGR)
  zephyr_library_sources(
    ${SERVICE_DIR}/power_manager/src/sl_power_manager.c
    ${SERVICE_DIR}/power_manager/src/sl_power_manager_hal_s2.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_POWER_MANAGER_PRESENT
  )
  zephyr_compile_definitions_ifdef(CONFIG_SOC_GECKO_RTCC
    SL_CATALOG_POWER_MANAGER_DEEPSLEEP_BLOCKING_HFXO_RESTORE_PRESENT
  )
endif()

# HFXO Manager
if(CONFIG_SOC_SILABS_HFXO_MANAGER)
  zephyr_library_sources(
    ${SERVICE_DIR}/hfxo_manager/src/sl_hfxo_manager.c
    ${SERVICE_DIR}/hfxo_manager/src/sl_hfxo_manager_hal_s2.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_HFXO_MANAGER_PRESENT
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_DEV_INIT     ${COMMON_DIR}/src/sl_slist.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_CORE
  ${EMLIB_DIR}/src/em_core.c
  ${COMMON_DIR}/src/sl_core_cortexm.c
)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_CRYOTIMER    ${EMLIB_DIR}/src/em_cryotimer.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_EMU          ${EMLIB_DIR}/src/em_emu.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_GPIO         ${EMLIB_DIR}/src/em_gpio.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_I2C          ${EMLIB_DIR}/src/em_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_LETIMER      ${EMLIB_DIR}/src/em_letimer.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_LEUART       ${EMLIB_DIR}/src/em_leuart.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_MSC          ${EMLIB_DIR}/src/em_msc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_PRS          ${EMLIB_DIR}/src/em_prs.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_RMU          ${EMLIB_DIR}/src/em_rmu.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_RTC          ${EMLIB_DIR}/src/em_rtc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_RTCC         ${EMLIB_DIR}/src/em_rtcc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_EUSART       ${EMLIB_DIR}/src/em_eusart.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_TIMER        ${EMLIB_DIR}/src/em_timer.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_USART        ${EMLIB_DIR}/src/em_usart.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_WDOG         ${EMLIB_DIR}/src/em_wdog.c)

zephyr_include_directories_ifdef(CONFIG_SOC_GECKO_SE
  ${SECURITY_DIR}/sl_component/se_manager/src
  ${SECURITY_DIR}/sl_component/se_manager/inc
)

zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_SE
  ${EMLIB_DIR}/src/em_se.c
  ${SECURITY_DIR}/sl_component/se_manager/src/sl_se_manager.c
  ${SECURITY_DIR}/sl_component/se_manager/src/sl_se_manager_util.c
  ${SECURITY_DIR}/sl_component/se_manager/src/sli_se_manager_mailbox.c
)

zephyr_library_sources_ifdef(CONFIG_ENTROPY_GECKO_SE
  ${SECURITY_DIR}/sl_component/se_manager/src/sl_se_manager_entropy.c
)

zephyr_library_sources(src/sl_memory_manager_shim.c)
