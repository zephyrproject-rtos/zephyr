# Makefile - Gecko SDK
#
# Copyright (c) 2017, Christian Taedcke
# Copyright (c) 2021, Safran Passenger Innovations Germany GmbH
# Copyright (c) 2022, Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#

set(emlib_dir   ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/emlib)
set(common_dir  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/common)
set(device_dir  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/Device)
set(radio_dir   ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/platform/radio)
set(blobs_dir   ${ZEPHYR_HAL_SILABS_MODULE_DIR}/zephyr/blobs/gecko)
set(service_dir ${ZEPHYR_HAL_SILABS_MODULE_DIR}/gecko/platform/service)

# Translate the SoC name and part number into the gecko device and cpu name
# respectively.
string(TOUPPER ${CONFIG_SOC_SERIES} device_family)
string(TOUPPER ${CONFIG_SOC} device_part_number)

# Get SoC series number, i.e. translate e.g. efr32bg22 -> 22, efr32fg1p -> 1
string(SUBSTRING "${CONFIG_SOC_SERIES}" 7 2 series_number)
# Strip any non-numeric characters from the series number
string(REGEX REPLACE "[^0-9]" "" series_number "${series_number}")
# Get major series number for directory mapping
string(REGEX MATCH "^[0-9]" major_series_number "${series_number}")

function(add_prebuilt_library lib_name prebuilt_path)
  add_library(${lib_name} STATIC IMPORTED GLOBAL)
  set_target_properties(${lib_name} PROPERTIES
    IMPORTED_LOCATION ${blobs_dir}/${prebuilt_path}
  )
  zephyr_link_libraries(${lib_name})
endfunction()

if(CONFIG_SOC_VENDOR_SILABS_HAS_RADIO)
  if(CONFIG_SOC_FAMILY_SILABS_S1)
    zephyr_include_directories(
      ${radio_dir}/rail_lib/plugin/pa-conversions/efr32xg1x/config
      ${radio_dir}/rail_lib/chip/efr32/efr32xg1x
    )
  endif()

  zephyr_include_directories(
    ${radio_dir}/rail_lib/common
    ${radio_dir}/rail_lib/plugin/pa-conversions
  )

  if(CONFIG_SOC_GECKO_USE_RAIL)
    # rail
    zephyr_library_sources(${radio_dir}/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c)
    zephyr_library_sources(${radio_dir}/rail_lib/plugin/pa-conversions/pa_curves_efr32.c)

    # prebuilt libs
    if(CONFIG_SILABS_GECKO_RAIL_MULTIPROTOCOL)
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=1)
      add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_multiprotocol_efr32xg${series_number}_gcc_release.a)

    else()
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=0)
      add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_efr32xg${series_number}_gcc_release.a)
    endif()

    if(CONFIG_SOC_GECKO_CUSTOM_RADIO_PHY)
      zephyr_include_directories(
        ${radio_dir}/rail_lib/plugin/rail_util_init/config/proprietary
        ${radio_dir}/rail_lib/plugin/rail_util_callbacks
        ${radio_dir}/rail_lib/plugin/rail_util_callbacks/config
        ${radio_dir}/rail_lib/plugin/rail_util_protocol
        ${radio_dir}/rail_lib/plugin/rail_util_protocol/config/efr32xg${major_series_number}x/
        ${radio_dir}/rail_lib/protocol/ble
        ${radio_dir}/rail_lib/protocol/ieee802154
        ${radio_dir}/rail_lib/protocol/zwave
      )
      zephyr_library_sources(${radio_dir}/rail_lib/plugin/rail_util_protocol/sl_rail_util_protocol.c)
    endif()

  endif()
endif()

zephyr_include_directories(
  ${device_dir}/SiliconLabs/${device_family}/Include
  ${common_dir}/inc
  ${emlib_dir}/inc
  ${service_dir}/power_manager/config
  ${service_dir}/power_manager/inc
  ${service_dir}/sleeptimer/config
  ${service_dir}/sleeptimer/inc
  ${service_dir}/sleeptimer/src
  ${BOARD_DIR}
)

# The gecko SDK uses the cpu name to include the matching device header.
# See Device/SiliconLabs/$(device_family)/Include/em_device.h for an example.
zephyr_compile_definitions(
  ${device_part_number}
)

# Sleeptimer
if(CONFIG_SILABS_GECKO_SLEEPTIMER)
  zephyr_library_sources(
    ${service_dir}/sleeptimer/src/sl_sleeptimer_hal_rtcc.c
    ${service_dir}/sleeptimer/src/sl_sleeptimer.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_SLEEPTIMER_PRESENT
  )
endif()

# Power Manager
if(CONFIG_SILABS_GECKO_POWER_MANAGER)
  zephyr_library_sources(
    ${service_dir}/power_manager/src/sl_power_manager.c
    ${service_dir}/power_manager/src/sl_power_manager_hal_s0_s1.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_POWER_MANAGER_PRESENT
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_SLIST ${common_dir}/src/sl_slist.c)

zephyr_library_sources(                                    ${emlib_dir}/src/em_system.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_ADC       ${emlib_dir}/src/em_adc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_BURTC     ${emlib_dir}/src/em_burtc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_CMU       ${emlib_dir}/src/em_cmu.c)

zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_CORE      ${emlib_dir}/src/em_core.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_CRYOTIMER ${emlib_dir}/src/em_cryotimer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_EMU       ${emlib_dir}/src/em_emu.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_GPIO      ${emlib_dir}/src/em_gpio.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_I2C       ${emlib_dir}/src/em_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_LETIMER   ${emlib_dir}/src/em_letimer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_LEUART    ${emlib_dir}/src/em_leuart.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_MSC       ${emlib_dir}/src/em_msc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_PRS       ${emlib_dir}/src/em_prs.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_RMU       ${emlib_dir}/src/em_rmu.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_RTC       ${emlib_dir}/src/em_rtc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_RTCC      ${emlib_dir}/src/em_rtcc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_TIMER     ${emlib_dir}/src/em_timer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_USART     ${emlib_dir}/src/em_usart.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_GECKO_WDOG      ${emlib_dir}/src/em_wdog.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32TG     ${device_dir}/SiliconLabs/EFM32TG/Source/system_efm32tg.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32WG     ${device_dir}/SiliconLabs/EFM32WG/Source/system_efm32wg.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32BG13P  ${device_dir}/SiliconLabs/EFR32BG13P/Source/system_efr32bg13p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32FG1P   ${device_dir}/SiliconLabs/EFR32FG1P/Source/system_efr32fg1p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32FG13P  ${device_dir}/SiliconLabs/EFR32FG13P/Source/system_efr32fg13p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32HG     ${device_dir}/SiliconLabs/EFM32HG/Source/system_efm32hg.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32MG12P  ${device_dir}/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFR32MG13P  ${device_dir}/SiliconLabs/EFR32MG13P/Source/system_efr32mg13p.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32PG12B  ${device_dir}/SiliconLabs/EFM32PG12B/Source/system_efm32pg12b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32GG11B  ${device_dir}/SiliconLabs/EFM32GG11B/Source/system_efm32gg11b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32GG12B  ${device_dir}/SiliconLabs/EFM32GG12B/Source/system_efm32gg12b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32JG12B  ${device_dir}/SiliconLabs/EFM32JG12B/Source/system_efm32jg12b.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_EFM32PG1B   ${device_dir}/SiliconLabs/EFM32PG1B/Source/system_efm32pg1b.c)
