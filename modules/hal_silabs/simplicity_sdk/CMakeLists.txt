# CMake integration for Simplicity SDK
#
# Copyright (c) 2017, Christian Taedcke
# Copyright (c) 2021, Safran Passenger Innovations Germany GmbH
# Copyright (c) 2022, Antmicro <www.antmicro.com>
# Copyright (c) 2024, Silicon Laboratories Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(EMLIB_DIR      ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/emlib)
set(EMDRV_DIR      ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/emdrv)
set(COMMON_DIR     ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/common)
set(DEVICE_DIR     ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/Device)
set(DRIVER_DIR     ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/driver)
set(RADIO_DIR      ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/radio)
set(SECURITY_DIR   ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/security)
set(SERVICE_DIR    ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/service)
set(PERIPHERAL_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/peripheral)
set(BLUETOOTH_DIR  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/protocol/bluetooth)
set(BLOBS_DIR      ${ZEPHYR_HAL_SILABS_MODULE_DIR}/zephyr/blobs/simplicity_sdk)

# Translate the SoC name and part number into the gecko device and cpu name
# respectively.
string(TOUPPER ${CONFIG_SOC_SERIES} SILABS_DEVICE_FAMILY)
string(TOUPPER ${CONFIG_SOC} SILABS_DEVICE_PART_NUMBER)

# Get SoC series number, i.e. translate efr32bg22 -> 22, mgm240p -> 24, simg301 -> 301
if(CONFIG_SOC_SERIES MATCHES "^ef[rm]32")
  string(SUBSTRING ${CONFIG_SOC_SERIES} 7 2 SILABS_DEVICE_FAMILY_NUMBER)
elseif(CONFIG_SOC_SERIES MATCHES "^si")
  string(SUBSTRING ${CONFIG_SOC_SERIES} 4 3 SILABS_DEVICE_FAMILY_NUMBER)
elseif(CONFIG_SOC_SERIES MATCHES "^.gm")
  string(SUBSTRING ${CONFIG_SOC_SERIES} 3 2 SILABS_DEVICE_FAMILY_NUMBER)
else()
  message(FATAL_ERROR "unknown part")
endif()

function(add_prebuilt_library lib_name prebuilt_path)
  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    add_library(${lib_name} STATIC IMPORTED GLOBAL)
    set_target_properties(${lib_name} PROPERTIES
      IMPORTED_LOCATION ${BLOBS_DIR}/${prebuilt_path}
    )
    zephyr_link_libraries(${lib_name})
  endif()
endfunction()

if(CONFIG_SOC_GECKO_HAS_RADIO)
  zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
    ${RADIO_DIR}/rail_lib/chip/efr32/efr32xg2x
  )

  zephyr_compile_definitions(
    SL_RAIL_UTIL_PA_CONFIG_HEADER="sl_rail_util_pa_config.h"
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
    ${SECURITY_DIR}/sl_component/sli_crypto/src/sl_crypto_s2.c
  )

  if(CONFIG_BT_SILABS_EFR32)
    # prebuilt libs
    add_prebuilt_library(liblinklayer protocol/bluetooth/bgstack/ll/build/gcc/xg${SILABS_DEVICE_FAMILY_NUMBER}/release/liblinklayer.a)
    add_prebuilt_library(libbgcommon  protocol/bluetooth/bgcommon/lib/build/gcc/cortex-m33/bgcommon/release/libbgcommon.a)
    if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
      # There is a circular dependency between the libs: libbgcommon depends on the symbol
      # bg_pool_pools while liblinklayer depends on many symbols in libbgcommon
      target_link_libraries(libbgcommon INTERFACE liblinklayer)
      target_link_libraries(liblinklayer INTERFACE libbgcommon)
      # Only two functions need to be stubbed if this is not included when no blobs are used.
      # The contents of the init/deinit functions are provided by the Link Layer from SiSDK,
      # and we should not care about their exact implementation and which calls they make.
      zephyr_library_sources(
        ${BLUETOOTH_DIR}/bgstack/ll/src/sl_btctrl_init.c
        ${BLUETOOTH_DIR}/bgstack/ll/src/sl_btctrl_init_tasklets.c
      )
    endif()
    zephyr_library_sources(src/sl_btctrl_hci_reset_shim.c)
    zephyr_include_directories(config/ll)

    # link mbedTLS
    if(CONFIG_MBEDTLS)
      zephyr_link_libraries(mbedTLS)
    endif()
  endif()

  if(CONFIG_SOC_GECKO_USE_RAIL)
    # rail
    # PA curve for the modules comes from their config archive - omit the compilation of the SoC curves on modules
    if(NOT CONFIG_SILABS_DEVICE_IS_MODULE)
      zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/pa-conversions/pa_curves_efr32.c)
    endif()
    zephyr_library_sources(${RADIO_DIR}/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c)

    if(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=1)
    else()
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=0)
    endif()

    if(CONFIG_SILABS_DEVICE_IS_MODULE)
      # RAIL lib and config for modules
      if(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_multiprotocol_module_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}_gcc_release.a)
      else()
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_module_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}_gcc_release.a)
      endif()

      add_prebuilt_library(librail_config platform/radio/rail_lib/autogen/librail_release/librail_config_${CONFIG_SOC}_gcc.a)
    else()
      # generic RAIL lib for SoCs
      if(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_multiprotocol_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}_gcc_release.a)
      else()
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}_gcc_release.a)
      endif()
    endif()

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
  ${DRIVER_DIR}/gpio/inc
  ${DRIVER_DIR}/i2c/inc
  ${DRIVER_DIR}/i2c/src
  ${EMDRV_DIR}/common/inc
  ${EMDRV_DIR}/dmadrv/config/s2_8ch/
  ${EMDRV_DIR}/dmadrv/inc
  ${EMDRV_DIR}/dmadrv/inc/s2_signals/
  ${EMLIB_DIR}/inc
  ${PERIPHERAL_DIR}/inc
  ${SERVICE_DIR}/clock_manager/inc
  ${SERVICE_DIR}/device_init/inc
  ${SERVICE_DIR}/device_manager/inc
  ${SERVICE_DIR}/hfxo_manager/config
  ${SERVICE_DIR}/hfxo_manager/inc
  ${SERVICE_DIR}/hfxo_manager/src
  ${SERVICE_DIR}/interrupt_manager/inc
  ${SERVICE_DIR}/memory_manager/inc
  ${SERVICE_DIR}/memory_manager/profiler/inc
  ${SERVICE_DIR}/power_manager/config
  ${SERVICE_DIR}/power_manager/inc
  ${SERVICE_DIR}/sleeptimer/config
  ${SERVICE_DIR}/sleeptimer/inc
  ${SERVICE_DIR}/sleeptimer/src
  ${SERVICE_DIR}/udelay/inc
  ${SECURITY_DIR}/sl_component/sl_protocol_crypto/src
  ${SECURITY_DIR}/sl_component/sli_crypto/inc
  ${BOARD_DIR}
)

zephyr_compile_definitions(
  ${SILABS_DEVICE_PART_NUMBER}
  SL_CODE_COMPONENT_CLOCK_MANAGER=clock_manager
  SL_CODE_COMPONENT_DEVICE_PERIPHERAL=peripheral
  SL_CODE_COMPONENT_HAL_COMMON=hal_common
  SL_CODE_COMPONENT_SYSTEM=system
)

zephyr_library_sources(
  ${DEVICE_DIR}/SiliconLabs/${SILABS_DEVICE_FAMILY}/Source/system_${CONFIG_SOC_SERIES}.c
  ${EMLIB_DIR}/src/em_system.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager_hal_s2.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager_init.c
  ${SERVICE_DIR}/clock_manager/src/sl_clock_manager_init_hal_s2.c
  ${SERVICE_DIR}/device_manager/devices/sl_device_peripheral_hal_efr32xg${SILABS_DEVICE_FAMILY_NUMBER}.c
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
    SL_CODE_COMPONENT_SLEEPTIMER=sleeptimer
    SL_CODE_COMPONENT_HAL_SYSRTC=hal_sysrtc
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
    ${SERVICE_DIR}/power_manager/src/common/sl_power_manager_common.c
    ${SERVICE_DIR}/power_manager/src/common/sl_power_manager_em4.c
    ${SERVICE_DIR}/power_manager/src/sleep_loop/sl_power_manager.c
    ${SERVICE_DIR}/power_manager/src/sleep_loop/sl_power_manager_hal_s2.c
  )
  zephyr_compile_definitions(
    SL_CATALOG_POWER_MANAGER_PRESENT
    SL_CODE_COMPONENT_POWER_MANAGER=power_manager
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
if(CONFIG_SOC_GECKO_CORE)
  zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_CORE
    ${COMMON_DIR}/src/sl_core_cortexm.c
  )
  zephyr_compile_definitions(
    SL_CODE_COMPONENT_CORE=core
  )
endif()
zephyr_library_sources_ifdef(CONFIG_SOC_SILABS_ACMP        ${EMLIB_DIR}/src/em_acmp.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_CRYOTIMER    ${EMLIB_DIR}/src/em_cryotimer.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_EMU          ${EMLIB_DIR}/src/em_emu.c)
if(CONFIG_SOC_GECKO_GPIO)
  zephyr_library_sources(
    ${EMLIB_DIR}/src/em_gpio.c
    ${DRIVER_DIR}/gpio/src/sl_gpio.c
    ${PERIPHERAL_DIR}/src/sl_hal_gpio.c
  )
  zephyr_library_compile_definitions(
    SL_CATALOG_GPIO_PRESENT
    SL_CODE_COMPONENT_HAL_GPIO=hal_gpio
  )
endif()

if(CONFIG_SILABS_SISDK_GPIO)
  zephyr_library_sources(
    ${PERIPHERAL_DIR}/src/sl_hal_gpio.c
  )
  zephyr_library_compile_definitions(
    SL_CATALOG_GPIO_PRESENT
    SL_CODE_COMPONENT_HAL_GPIO=hal_gpio
  )
endif()

if(CONFIG_SILABS_SISDK_I2C)
  zephyr_library_sources(
    ${DRIVER_DIR}/i2c/src/sl_i2c.c
    ${SERVICE_DIR}/udelay/src/sl_udelay.c
    ${PERIPHERAL_DIR}/src/sl_hal_i2c.c
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_LETIMER   ${PERIPHERAL_DIR}/src/sl_hal_letimer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_TIMER     ${PERIPHERAL_DIR}/src/sl_hal_timer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_VDAC      ${PERIPHERAL_DIR}/src/sl_hal_vdac.c)

zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_LDMA         ${EMDRV_DIR}/dmadrv/src/dmadrv.c)

zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_I2C          ${EMLIB_DIR}/src/em_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_LEUART       ${EMLIB_DIR}/src/em_leuart.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_MSC          ${EMLIB_DIR}/src/em_msc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_LDMA         ${EMLIB_DIR}/src/em_ldma.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_PRS          ${EMLIB_DIR}/src/em_prs.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_RMU          ${EMLIB_DIR}/src/em_rmu.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_RTC          ${EMLIB_DIR}/src/em_rtc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_RTCC         ${EMLIB_DIR}/src/em_rtcc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_EUSART       ${EMLIB_DIR}/src/em_eusart.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_USART        ${EMLIB_DIR}/src/em_usart.c)
zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_WDOG         ${EMLIB_DIR}/src/em_wdog.c)

zephyr_include_directories_ifdef(CONFIG_SOC_GECKO_SE
  ${SECURITY_DIR}/sl_component/se_manager/src
  ${SECURITY_DIR}/sl_component/se_manager/inc
  ${SECURITY_DIR}/sl_component/sli_psec_osal/inc
)

zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_SE
  ${SECURITY_DIR}/sl_component/se_manager/src/sl_se_manager.c
  ${SECURITY_DIR}/sl_component/se_manager/src/sl_se_manager_util.c
  ${SECURITY_DIR}/sl_component/se_manager/src/sli_se_manager_mailbox.c
)

zephyr_compile_definitions_ifdef(CONFIG_SOC_GECKO_SE
  SL_CODE_COMPONENT_SE_MANAGER=se_manager
  SL_CODE_COMPONENT_PSEC_OSAL=psec_osal
)

zephyr_library_sources_ifdef(CONFIG_ENTROPY_GECKO_SE
  ${SECURITY_DIR}/sl_component/se_manager/src/sl_se_manager_entropy.c
)

zephyr_library_sources(
  src/sl_interrupt_manager_shim.c
  src/sl_memory_manager_shim.c
)

zephyr_library_sources_ifdef(
  CONFIG_BUILD_ONLY_NO_BLOBS
  src/blob_stubs.c
)

zephyr_linker_sources(ROM_SECTIONS linker/code_classification_text.ld)
zephyr_linker_sources(RAMFUNC_SECTION linker/code_classification_ramfunc.ld)
