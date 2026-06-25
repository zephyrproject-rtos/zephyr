# Makefile - Gecko SDK
#
# Copyright (c) 2017, Christian Taedcke
# Copyright (c) 2021, Safran Passenger Innovations Germany GmbH
# Copyright (c) 2022, Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#

set(EMLIB_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/emlib)
set(COMMON_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/common)
set(DEVICE_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/Device)
set(RADIO_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/platform/radio)
set(BLOBS_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/zephyr/blobs/gecko)
set(SERVICE_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/platform/service)
set(BLUETOOTH_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/protocol/bluetooth)
set(SECURITY_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/platform/security)

# Translate the SoC name and part number into the gecko device and cpu name
# respectively.
string(TOUPPER ${CONFIG_SOC_SERIES} SILABS_GECKO_DEVICE)
string(TOUPPER ${CONFIG_SOC} SILABS_GECKO_PART_NUMBER)

# Get SoC series number, i.e. translate e.g. efr32bg22 -> 22, efr32fg1p -> 1
string(SUBSTRING "${CONFIG_SOC_SERIES}" 7 2 GECKO_SERIES_NUMBER)
# Strip any non-numeric characters from the series number
string(REGEX REPLACE "[^0-9]" "" GECKO_SERIES_NUMBER "${GECKO_SERIES_NUMBER}")
# Get major series number for directory mapping
string(REGEX MATCH "^[0-9]" GECKO_MAJOR_SERIES_NUMBER "${GECKO_SERIES_NUMBER}")

function(add_prebuilt_library lib_name prebuilt_path)
  add_library(${lib_name} STATIC IMPORTED GLOBAL)
  set_target_properties(${lib_name} PROPERTIES
    IMPORTED_LOCATION ${BLOBS_DIR}/${prebuilt_path}
  )
  zephyr_link_libraries(${lib_name})
endfunction()

if(${CONFIG_SOC_GECKO_HAS_RADIO})
  if(${CONFIG_SOC_FAMILY_SILABS_S1})
    zephyr_include_directories(
      ${RADIO_DIR}/rail_lib/chip/efr32/efr32xg1x
    )
  endif()

  zephyr_compile_definitions(
    SL_RAIL_UTIL_PA_CONFIG_HEADER="sl_rail_util_pa_config.h"
  )

  zephyr_include_directories(
    ${RADIO_DIR}/rail_lib/common
    ${RADIO_DIR}/rail_lib/plugin/pa-conversions
  )

  # sl_protocol_crypto
  zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_PROTOCOL_CRYPTO
    ${EMLIB_DIR}/src/em_crypto.c
    ${SECURITY_DIR}/sl_component/sl_protocol_crypto/src/sli_protocol_crypto_crypto.c
    ${SECURITY_DIR}/sl_component/sl_psa_driver/src/crypto_management.c
  )
  zephyr_include_directories_ifdef(CONFIG_SILABS_GECKO_PROTOCOL_CRYPTO
      ${SECURITY_DIR}/sl_component/sl_psa_driver/inc
  )

  if(CONFIG_BT_SILABS_EFR32)
    zephyr_include_directories(
      ${BLUETOOTH_DIR}/bgstack/ll/inc
      btstack
    )
    zephyr_library_sources(btstack/sl_bluetooth_controller.c)

    # prebuilt libs
    add_prebuilt_library(libbluetooth_controller protocol/bluetooth/bgstack/ll/lib/libbluetooth_controller_efr32xg${GECKO_SERIES_NUMBER}_gcc_release.a)
    add_prebuilt_library(libbgcommon protocol/bluetooth/bgcommon/lib/libbgcommon_efr32xg${GECKO_SERIES_NUMBER}_gcc_release.a)

    if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
      # There is a circular dependency between the libs: libbgcommon depends on the symbol
      # bg_pool_pools while libbluetooth_controller depends on many symbols in libbgcommon
      target_link_libraries(libbgcommon INTERFACE libbluetooth_controller)
      target_link_libraries(libbluetooth_controller INTERFACE libbgcommon)

      # These use deprecated mbedtls APIs, so we just implement them ourselves.
      target_link_options(libbgcommon INTERFACE -Wl,--wrap=bg_random_init)
      target_link_options(libbgcommon INTERFACE -Wl,--wrap=bg_random_deinit)
      target_link_options(libbgcommon INTERFACE -Wl,--wrap=bg_random_get_data)
    endif()
  endif()

  if(CONFIG_SOC_GECKO_USE_RAIL)
    # rail
    zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c)
    zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/pa-conversions/pa_curves_efr32.c)
    zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/rail_util_sequencer/sl_rail_util_sequencer.c)
    zephyr_include_directories(
      ${RADIO_DIR}/rail_lib/plugin/rail_util_sequencer/config/efr32xg${GECKO_SERIES_NUMBER}
    )

    # prebuilt libs
    if(CONFIG_SILABS_GECKO_RAIL_MULTIPROTOCOL)
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=1)
      add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_multiprotocol_efr32xg${GECKO_SERIES_NUMBER}_gcc_release.a)

    else()
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=0)
      add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_efr32xg${GECKO_SERIES_NUMBER}_gcc_release.a)
    endif()

    if(CONFIG_SOC_GECKO_CUSTOM_RADIO_PHY)
      zephyr_include_directories(
        ${RADIO_DIR}/rail_lib/plugin/rail_util_init/config/proprietary
        ${RADIO_DIR}/rail_lib/plugin/rail_util_callbacks
        ${RADIO_DIR}/rail_lib/plugin/rail_util_callbacks/config
        ${RADIO_DIR}/rail_lib/plugin/rail_util_protocol
        ${RADIO_DIR}/rail_lib/plugin/rail_util_protocol/config/efr32xg${GECKO_MAJOR_SERIES_NUMBER}x/
        ${RADIO_DIR}/rail_lib/protocol/ble
        ${RADIO_DIR}/rail_lib/protocol/ieee802154
        ${RADIO_DIR}/rail_lib/protocol/zwave
      )
      zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/rail_util_protocol/sl_rail_util_protocol.c)
    endif()

  endif()
endif()

zephyr_include_directories(
  config
  ${DEVICE_DIR}/SiliconLabs/${SILABS_GECKO_DEVICE}/Include
  ${COMMON_DIR}/inc
  ${EMLIB_DIR}/inc
  ${SERVICE_DIR}/power_manager/config
  ${SERVICE_DIR}/power_manager/inc
  ${SERVICE_DIR}/sleeptimer/config
  ${SERVICE_DIR}/sleeptimer/inc
  ${SERVICE_DIR}/sleeptimer/src
  ${BOARD_DIR}
)

zephyr_library_sources(
  sl_memory_manager_shim.c
)

# The gecko SDK uses the cpu name to include the matching device header.
# See Device/SiliconLabs/$(SILABS_GECKO_DEVICE)/Include/em_device.h for an example.
zephyr_compile_definitions(
  ${SILABS_GECKO_PART_NUMBER}
)

# Sleeptimer
if(CONFIG_SILABS_GECKO_SLEEPTIMER)
  zephyr_library_sources(
    ${SERVICE_DIR}/sleeptimer/src/sl_sleeptimer_hal_rtcc.c
    ${SERVICE_DIR}/sleeptimer/src/sl_sleeptimer.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_SLEEPTIMER_PRESENT
  )
endif()

# Power Manager
if(CONFIG_SILABS_GECKO_POWER_MANAGER)
  zephyr_library_sources(
    ${SERVICE_DIR}/power_manager/src/sl_power_manager.c
    ${SERVICE_DIR}/power_manager/src/sl_power_manager_hal_s0_s1.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_POWER_MANAGER_PRESENT
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_SLIST ${COMMON_DIR}/src/sl_slist.c)

zephyr_library_sources(                                    ${EMLIB_DIR}/src/em_system.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_ADC       ${EMLIB_DIR}/src/em_adc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_BURTC     ${EMLIB_DIR}/src/em_burtc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_CMU       ${EMLIB_DIR}/src/em_cmu.c)

zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_CORE      ${EMLIB_DIR}/src/em_core.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_CRYOTIMER ${EMLIB_DIR}/src/em_cryotimer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_EMU       ${EMLIB_DIR}/src/em_emu.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_GPIO      ${EMLIB_DIR}/src/em_gpio.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_I2C       ${EMLIB_DIR}/src/em_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_LETIMER   ${EMLIB_DIR}/src/em_letimer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_LEUART    ${EMLIB_DIR}/src/em_leuart.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_MSC       ${EMLIB_DIR}/src/em_msc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_PRS       ${EMLIB_DIR}/src/em_prs.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_RMU       ${EMLIB_DIR}/src/em_rmu.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_RTC       ${EMLIB_DIR}/src/em_rtc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_RTCC      ${EMLIB_DIR}/src/em_rtcc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_TIMER     ${EMLIB_DIR}/src/em_timer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_USART     ${EMLIB_DIR}/src/em_usart.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_WDOG      ${EMLIB_DIR}/src/em_wdog.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32TG     ${DEVICE_DIR}/SiliconLabs/EFM32TG/Source/system_efm32tg.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32WG     ${DEVICE_DIR}/SiliconLabs/EFM32WG/Source/system_efm32wg.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32BG13P  ${DEVICE_DIR}/SiliconLabs/EFR32BG13P/Source/system_efr32bg13p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32FG1P   ${DEVICE_DIR}/SiliconLabs/EFR32FG1P/Source/system_efr32fg1p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32FG13P  ${DEVICE_DIR}/SiliconLabs/EFR32FG13P/Source/system_efr32fg13p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32HG     ${DEVICE_DIR}/SiliconLabs/EFM32HG/Source/system_efm32hg.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32MG12P  ${DEVICE_DIR}/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32MG13P  ${DEVICE_DIR}/SiliconLabs/EFR32MG13P/Source/system_efr32mg13p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32PG12B  ${DEVICE_DIR}/SiliconLabs/EFM32PG12B/Source/system_efm32pg12b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32GG11B  ${DEVICE_DIR}/SiliconLabs/EFM32GG11B/Source/system_efm32gg11b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32GG12B  ${DEVICE_DIR}/SiliconLabs/EFM32GG12B/Source/system_efm32gg12b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32JG12B  ${DEVICE_DIR}/SiliconLabs/EFM32JG12B/Source/system_efm32jg12b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32PG1B   ${DEVICE_DIR}/SiliconLabs/EFM32PG1B/Source/system_efm32pg1b.c)
