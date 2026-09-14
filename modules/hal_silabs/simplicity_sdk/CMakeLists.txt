# CMake integration for Simplicity SDK
#
# Copyright (c) 2017, Christian Taedcke
# Copyright (c) 2021, Safran Passenger Innovations Germany GmbH
# Copyright (c) 2022, Antmicro <www.antmicro.com>
# Copyright (c) 2024, Silicon Laboratories Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(simplicity_sdk)

set(emlib_dir      ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/emlib)
set(emdrv_dir      ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/emdrv)
set(common_dir     ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/common)
set(device_dir     ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/Device)
set(driver_dir     ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/driver)
set(radio_dir      ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/radio)
set(security_dir   ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/security)
set(service_dir    ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/service)
set(peripheral_dir ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/platform/peripheral)
set(bluetooth_dir  ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/protocol/bluetooth)
set(crypto_dir     ${ZEPHYR_CURRENT_MODULE_DIR}/simplicity_sdk/util/third_party/crypto_ip)
set(blobs_dir      ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/blobs/simplicity_sdk)

# Translate the SoC name and part number into the gecko device and cpu name
# respectively.
string(TOUPPER ${CONFIG_SOC_SERIES} device_family)
string(TOUPPER ${CONFIG_SOC} device_part_number)

# Get SoC series number, i.e. translate efr32bg22 -> 22, mgm240p -> 24, simg301 -> 301
if(CONFIG_SOC_SERIES MATCHES "^ef[rm]32")
  string(SUBSTRING ${CONFIG_SOC_SERIES} 7 2 family_no)
  set(device_prefix efr32)
elseif(CONFIG_SOC_SERIES MATCHES "^si")
  string(SUBSTRING ${CONFIG_SOC_SERIES} 4 3 family_no)
  set(device_prefix si)
elseif(CONFIG_SOC_SERIES MATCHES "^.gm")
  string(SUBSTRING ${CONFIG_SOC_SERIES} 3 2 family_no)
  set(device_prefix efr32)
else()
  message(FATAL_ERROR "unknown part")
endif()

function(add_prebuilt_library lib_name prebuilt_path)
  if(NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    add_library(${lib_name} STATIC IMPORTED GLOBAL)
    set_target_properties(${lib_name} PROPERTIES
      IMPORTED_LOCATION ${blobs_dir}/${prebuilt_path}
    )
    zephyr_link_libraries(${lib_name})
  endif()
endfunction()

if(CONFIG_SOC_VENDOR_SILABS_HAS_RADIO)
  zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
    ${radio_dir}/rail_lib/chip/efr32/efr32xg2x
  )
  zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
    ${radio_dir}/rail_lib/chip/efr32/sixg3xx
  )

  zephyr_compile_definitions(
    SL_RAIL_UTIL_PA_CONFIG_HEADER="sl_rail_util_pa_config.h"
  )

  zephyr_include_directories(
    ${radio_dir}/rail_lib/common
    ${radio_dir}/rail_lib/plugin/sl_rail_util_compatible_pa
    ${radio_dir}/rail_lib/plugin/sl_rail_util_load_devinfo
    ${radio_dir}/rail_lib/plugin/sl_rail_util_rf_path_switch
    ${radio_dir}/rail_lib/plugin/sl_rail_util_built_in_phys/${device_prefix}xg${family_no}
    ${bluetooth_dir}/bgstack/ll/inc
  )

  zephyr_include_directories_ifdef(CONFIG_SOC_VENDOR_SILABS_DEVICE_IS_MODULE
    ${radio_dir}/rail_lib/plugin/pa-conversions
  )
  zephyr_include_directories_ifndef(CONFIG_SOC_VENDOR_SILABS_DEVICE_IS_MODULE
    ${radio_dir}/rail_lib/plugin/sl_rail_util_pa_conversions
    ${radio_dir}/rail_lib/plugin/sl_rail_util_pa_tables
    ${radio_dir}/rail_lib/plugin/sl_rail_util_pa_tables/${device_prefix}xg${family_no}
  )

  # sl_protocol_crypto
  if(CONFIG_SILABS_SISDK_PROTOCOL_CRYPTO)
    zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
      ${security_dir}/sl_component/sl_protocol_crypto/src/sli_radioaes_management.c
      ${security_dir}/sl_component/sl_protocol_crypto/src/sli_protocol_crypto_radioaes.c
      ${security_dir}/sl_component/sli_crypto/src/sl_crypto_s2.c
    )
    zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
      ${security_dir}/sl_component/sli_crypto/src/sl_crypto_s3.c
      src/sl_sxsymcrypt_platform.c
    )
    zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
      ${crypto_dir}/sxsymcrypt/include
      ${crypto_dir}/sxsymcrypt/src/platform/silicon_labs
    )
    if(CONFIG_SOC_FAMILY_SILABS_S3)
      add_prebuilt_library(sxsymcrypt util/third_party/crypto_ip/libsxsymcrypt.a)
    endif()
    zephyr_library_compile_definitions(
      SL_CODE_COMPONENT_SLI_CRYPTO=sli_crypto
    )
  endif()

  if(CONFIG_BT_SILABS_EFR32)
    # prebuilt libs
    if(CONFIG_SOC_FAMILY_SILABS_S2)
      add_prebuilt_library(liblinklayer protocol/bluetooth/bgstack/ll/build/gcc/xg${family_no}/release/liblinklayer.a)
    elseif(CONFIG_SOC_FAMILY_SILABS_S3)
      add_prebuilt_library(liblinklayer protocol/bluetooth/bgstack/ll/build/gcc/x${family_no}/release/liblinklayer.a)
    endif()
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
        ${bluetooth_dir}/bgstack/ll/src/sl_btctrl_init.c
        ${bluetooth_dir}/bgstack/ll/src/sl_btctrl_init_tasklets.c
      )
    endif()
    zephyr_library_sources(src/sl_btctrl_hci_reset_shim.c)
    zephyr_include_directories(config/ll)

    zephyr_link_libraries_ifdef(CONFIG_MBEDTLS_BUILTIN mbedtls_iface)
  endif()

  if(CONFIG_SOC_GECKO_USE_RAIL)
    # rail
    zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
      ${radio_dir}/rail_lib/plugin/sl_rail_util_rf_path_switch/sl_rail_util_rf_path_switch.c
      ${radio_dir}/rail_lib/plugin/sl_rail_util_sequencer/sl_rail_util_sequencer.c
    )
    zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
      ${radio_dir}/rail_lib/plugin/sl_rail_util_load_devinfo/sl_rail_util_load_devinfo.c
    )

    if(NOT CONFIG_SOC_VENDOR_SILABS_DEVICE_IS_MODULE)
      zephyr_library_sources(
        ${radio_dir}/rail_lib/plugin/sl_rail_util_built_in_phys/sl_rail_util_built_in_phys.c
      )

      if(CONFIG_SOC_FAMILY_SILABS_S2_XG24 OR CONFIG_SOC_FAMILY_SILABS_S2_XG26 OR CONFIG_SOC_FAMILY_SILABS_S3_SIX301)
        dt_nodelabel(hfxo_node NODELABEL "hfxo" REQUIRED)
        dt_prop(hfxo_freq PATH "${hfxo_node}" PROPERTY "clock-frequency")
        if(hfxo_freq EQUAL "38400000")
          set(phy_suffix "_38M4Hz")
        elseif(hfxo_freq EQUAL "39000000")
          set(phy_suffix "_39MHz")
        elseif(hfxo_freq EQUAL "40000000")
          set(phy_suffix "_40MHz")
        else()
          message(FATAL_ERROR "Unknown HFXO frequency, not compatible with radio PHYs")
        endif()
      endif()
      set(phy_dir ${radio_dir}/rail_lib/plugin/sl_rail_util_built_in_phys/${device_prefix}xg${family_no})

      # sl_rail_util_built_in_phys.c provides strong definitions of the PHY pointers for
      # every protocol the device supports, overriding the weak fallbacks in the RAIL
      # library. Core RAIL objects reference those pointers regardless of which protocol
      # is enabled, so the configuration of every supported protocol must be compiled,
      # including protocols Zephyr has no support for. Unused configurations are
      # discarded by the linker.
      foreach(phy ble ieee802154 zwave sidewalk rfsense_ook)
        if(EXISTS ${phy_dir}/sl_rail_${phy}_config${phy_suffix}.c)
          zephyr_library_sources(${phy_dir}/sl_rail_${phy}_config${phy_suffix}.c)
        endif()
      endforeach()
    endif()

    # PA curve for the modules comes from their config archive - omit the compilation of the SoC curves on modules
    zephyr_library_sources_ifdef(CONFIG_SOC_VENDOR_SILABS_DEVICE_IS_MODULE
      ${radio_dir}/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c
    )
    zephyr_library_sources_ifndef(CONFIG_SOC_VENDOR_SILABS_DEVICE_IS_MODULE
      ${radio_dir}/rail_lib/plugin/sl_rail_util_pa_conversions/sl_rail_util_pa_conversions.c
      ${radio_dir}/rail_lib/plugin/sl_rail_util_pa_conversions/sl_rail_util_pa_tables.c
    )

    if(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=1)
    else()
      zephyr_compile_definitions(SL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=0)
    endif()

    if(CONFIG_SOC_VENDOR_SILABS_DEVICE_IS_MODULE)
      # RAIL lib and config for modules
      if(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_multiprotocol_module_${device_prefix}xg${family_no}_gcc_release.a)
      else()
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_module_${device_prefix}xg${family_no}_gcc_release.a)
      endif()

      add_prebuilt_library(librail_config platform/radio/rail_lib/autogen/librail_release/librail_config_${CONFIG_SOC}_gcc.a)
    else()
      # generic RAIL lib for SoCs
      if(CONFIG_SILABS_SISDK_RAIL_MULTIPROTOCOL)
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_multiprotocol_${device_prefix}xg${family_no}_gcc_release.a)
      else()
        add_prebuilt_library(librail platform/radio/rail_lib/autogen/librail_release/librail_${device_prefix}xg${family_no}_gcc_release.a)
      endif()
    endif()

    zephyr_include_directories_ifdef(CONFIG_SOC_GECKO_CUSTOM_RADIO_PHY
      ${radio_dir}/rail_lib/protocol/ble
      ${radio_dir}/rail_lib/protocol/ieee802154
      ${radio_dir}/rail_lib/protocol/zwave
      ${radio_dir}/rail_lib/protocol/sidewalk
      ${radio_dir}/rail_lib/plugin/sl_rail_util_protocol
      ${radio_dir}/rail_lib/plugin/sl_rail_util_protocol/config/${device_prefix}xg${family_no}
    )
    zephyr_library_sources_ifdef(CONFIG_SOC_GECKO_CUSTOM_RADIO_PHY
      ${radio_dir}/rail_lib/plugin/sl_rail_util_protocol/sl_rail_util_protocol.c
    )
    zephyr_include_directories_ifdef(CONFIG_IEEE802154_SILABS_EFR32
      ${radio_dir}/rail_lib/protocol/ieee802154
    )
  endif()

  if(CONFIG_IEEE802154_SILABS_EFR32 AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
    set(openthread_blob_path protocol/openthread/build/gcc/cortex-m33/sl-openthread-library/Release/libsl_openthread.a)
    if(NOT EXISTS "${blobs_dir}/${openthread_blob_path}")
      message(FATAL_ERROR
        "IEEE802154_SILABS_EFR32 requires the OpenThread PAL prebuilt library at:\n"
        "  ${blobs_dir}/${openthread_blob_path}\n"
        "Install the hal_silabs module blobs or use CONFIG_BUILD_ONLY_NO_BLOBS for stubbed builds.")
    endif()
    add_prebuilt_library(libsl_openthread ${openthread_blob_path})
  endif()
endif()

zephyr_include_directories(
  config
  inc
  ${device_dir}/SiliconLabs/${device_family}/Include
  ${common_dir}/config
  ${common_dir}/inc
  ${driver_dir}/gpio/inc
  ${driver_dir}/i2c/inc
  ${driver_dir}/i2c/src
  ${emdrv_dir}/common/inc
  ${peripheral_dir}/inc
  ${service_dir}/clock_manager/inc
  ${service_dir}/device_init/inc
  ${service_dir}/device_manager/inc
  ${service_dir}/dma_manager/config/8ch
  ${service_dir}/dma_manager/inc
  ${service_dir}/hfxo_manager/config
  ${service_dir}/hfxo_manager/inc
  ${service_dir}/hfxo_manager/src
  ${service_dir}/interrupt_manager/inc
  ${service_dir}/memory_manager/inc
  ${service_dir}/memory_manager/profiler/inc
  ${service_dir}/power_manager/inc
  ${service_dir}/power_manager/src/common
  ${service_dir}/power_manager/src/sleep_loop
  ${service_dir}/sleeptimer/config
  ${service_dir}/sleeptimer/inc
  ${service_dir}/sleeptimer/src
  ${service_dir}/udelay/inc
  ${security_dir}/sl_component/sl_protocol_crypto/src
  ${security_dir}/sl_component/sli_crypto/inc
  ${security_dir}/sl_component/sli_psec_common/inc
  ${security_dir}/sl_component/sli_psec_osal/inc
  ${BOARD_DIR}
)

zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
  ${emlib_dir}/inc
)

zephyr_compile_definitions(
  ${device_part_number}
  SL_COMPONENT_CATALOG_PRESENT=1
  SL_CODE_COMPONENT_DEVICE_PERIPHERAL=peripheral
  SL_CODE_COMPONENT_HAL_COMMON=hal_common
  SL_CODE_COMPONENT_SYSTEM=system
)

zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
  ${service_dir}/device_manager/devices/sl_device_peripheral_hal_${device_prefix}xg${family_no}.c
  ${service_dir}/device_manager/dma/sl_device_dma_s2.c
  ${emlib_dir}/src/em_system.c
)
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
  ${service_dir}/device_manager/devices/sl_device_peripheral_hal_${device_prefix}xx${family_no}.c
  ${service_dir}/device_manager/dma/sl_device_dma_s3.c
)

if(CONFIG_SILABS_SISDK_SYSCFG)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
    ${peripheral_dir}/src/sl_hal_syscfg.c
  )
endif()

if(CONFIG_SILABS_SISDK_SYSTEM)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
    ${peripheral_dir}/src/sl_hal_system.c
  )
endif()

zephyr_library_sources(
  ${device_dir}/SiliconLabs/${device_family}/Source/system_${CONFIG_SOC_SERIES}.c
  ${service_dir}/device_manager/src/sl_device_clock.c
  ${service_dir}/device_manager/src/sl_device_dma.c
  ${service_dir}/device_manager/src/sl_device_gpio.c
  ${service_dir}/device_manager/src/sl_device_peripheral.c
  ${service_dir}/memory_manager/profiler/src/sli_memory_profiler_stubs.c
)

if(CONFIG_SOC_FAMILY_SILABS_S2 AND NOT family_no EQUAL "21")
  zephyr_library_sources(
    ${service_dir}/device_manager/clocks/sl_device_clock_${device_prefix}xg${family_no}.c
  )
elseif(CONFIG_SOC_FAMILY_SILABS_S3)
  zephyr_library_sources(
    ${service_dir}/device_manager/clocks/sl_device_clock_${device_prefix}xx${family_no}.c
  )
endif()

# Sleeptimer
if(CONFIG_SILABS_SISDK_SLEEPTIMER)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
    ${service_dir}/sleeptimer/src/sl_sleeptimer_hal_rtcc.c
  )
  zephyr_library_sources(
    ${peripheral_dir}/src/sl_hal_sysrtc.c
    ${peripheral_dir}/src/sl_hal_sysrtc_subsystem.c
    ${service_dir}/sleeptimer/src/sl_sleeptimer_hal_sysrtc.c
    ${service_dir}/sleeptimer/src/sl_sleeptimer.c
  )
  zephyr_compile_definitions(
    SL_CODE_COMPONENT_SLEEPTIMER=sleeptimer
    SL_CODE_COMPONENT_HAL_SYSRTC=hal_sysrtc
  )
endif()

# Clock Manager
if(CONFIG_SILABS_SISDK_CLOCK_MANAGER)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
    ${service_dir}/clock_manager/src/sl_clock_manager_hal_s2.c
    ${service_dir}/clock_manager/src/sl_clock_manager_init_hal_s2.c
    ${emlib_dir}/src/em_cmu.c
  )
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
    ${service_dir}/clock_manager/src/sl_clock_manager_hal_s3.c
    ${service_dir}/clock_manager/src/sl_clock_manager_init_hal_s3.c
  )
  zephyr_library_sources(
    ${service_dir}/clock_manager/src/sl_clock_manager.c
    ${service_dir}/clock_manager/src/sl_clock_manager_init.c
  )
  zephyr_compile_definitions(
    SL_CODE_COMPONENT_CLOCK_MANAGER=clock_manager
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_SYSTEM         ${peripheral_dir}/src/sl_hal_system.c)

# Keep em_iadc.c for compatibility for now. Not used anymore.
if(CONFIG_SOC_FAMILY_SILABS_S2)
  zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_IADC         ${emlib_dir}/src/em_iadc.c)
endif()
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_IADC         ${peripheral_dir}/src/sl_hal_iadc.c)

# Keep em_burtc.c becaused used in gecko_burtc_timer.c
if(CONFIG_SOC_FAMILY_SILABS_S2)
  zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_BURTC     ${emlib_dir}/src/em_burtc.c)
endif()
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_BURTC     ${peripheral_dir}/src/sl_hal_burtc.c)

# Device Init
if(CONFIG_SILABS_SISDK_DEVICE_INIT)
  zephyr_library_sources_ifdef(CONFIG_DT_HAS_SILABS_SERIES2_DCDC_ENABLED
    ${service_dir}/device_init/src/sl_device_init_dcdc_s2.c
  )
endif()

# Power Manager
if(CONFIG_SILABS_SISDK_POWER_MANAGER)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2
    ${service_dir}/power_manager/src/sleep_loop/sl_power_manager_hal_s2.c
  )
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3_SIX301
    ${service_dir}/power_manager/src/sleep_loop/sl_power_manager_hal_sixg301.c
  )
  zephyr_library_sources(
    ${service_dir}/power_manager/src/common/sl_power_manager_common.c
    ${service_dir}/power_manager/src/common/sl_power_manager_em4.c
    ${service_dir}/power_manager/src/sleep_loop/sl_power_manager.c
  )
  zephyr_compile_definitions(
    SL_CODE_COMPONENT_POWER_MANAGER=power_manager
  )
endif()
zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_SILABS_S3_SIX301
  ${service_dir}/power_manager/config/SIXG301
)
zephyr_include_directories_ifndef(CONFIG_SOC_FAMILY_SILABS_S3_SIX301
  ${service_dir}/power_manager/config
)

# HFXO Manager
if(CONFIG_SILABS_SISDK_HFXO_MANAGER)
  zephyr_library_sources(
    ${service_dir}/hfxo_manager/src/sl_hfxo_manager.c
    ${service_dir}/hfxo_manager/src/sl_hfxo_manager_hal_s2.c
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_SLIST ${common_dir}/src/sl_slist.c)

if(CONFIG_SILABS_SISDK_CORE)
  zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_CORE
    ${common_dir}/src/sl_core_cortexm.c
  )
  zephyr_compile_definitions(
    SL_CODE_COMPONENT_CORE=core
  )
endif()

if(CONFIG_SILABS_SISDK_ACMP)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_acmp.c)
  zephyr_library_sources(${peripheral_dir}/src/sl_hal_acmp.c)
endif()

if(CONFIG_SILABS_SISDK_EMU)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_emu.c)
  zephyr_library_sources(${peripheral_dir}/src/sl_hal_emu.c)
endif()

if(CONFIG_SILABS_SISDK_GPIO)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_gpio.c)
  zephyr_library_sources(
    ${driver_dir}/gpio/src/sl_gpio.c
    ${peripheral_dir}/src/sl_hal_gpio.c
  )
  zephyr_library_compile_definitions(
    SL_CODE_COMPONENT_GPIO=gpio
    SL_CODE_COMPONENT_HAL_GPIO=hal_gpio
  )
endif()

if(CONFIG_SILABS_SISDK_I2C)
  zephyr_library_sources(
    ${driver_dir}/i2c/src/sl_i2c.c
    ${service_dir}/udelay/src/sl_udelay.c
    ${peripheral_dir}/src/sl_hal_i2c.c
  )
endif()

# GPCRC
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_GPCRC     ${peripheral_dir}/src/sl_hal_gpcrc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_LETIMER   ${peripheral_dir}/src/sl_hal_letimer.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_VDAC      ${peripheral_dir}/src/sl_hal_vdac.c)

if(CONFIG_SILABS_SISDK_TIMER)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_timer.c)
  zephyr_library_sources(${peripheral_dir}/src/sl_hal_timer.c)
endif()

if(CONFIG_SILABS_SISDK_LDMA)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_ldma.c)
  zephyr_library_sources(
    ${service_dir}/dma_manager/src/sl_dma_manager.c
    ${service_dir}/dma_manager/src/sl_dma_manager_hal_ldma.c
    ${peripheral_dir}/src/sl_hal_ldma.c
  )
endif()

zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_MSC       ${emlib_dir}/src/em_msc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_RMU       ${emlib_dir}/src/em_rmu.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_RTCC      ${emlib_dir}/src/em_rtcc.c)
zephyr_library_sources_ifdef(CONFIG_SILABS_SISDK_USART     ${emlib_dir}/src/em_usart.c)

if(CONFIG_SILABS_SISDK_EUSART)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_eusart.c)
  zephyr_library_sources(${peripheral_dir}/src/sl_hal_eusart.c)
endif()

if(CONFIG_SILABS_SISDK_PRS)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_prs.c)
  zephyr_library_sources(${peripheral_dir}/src/sl_hal_prs.c)
endif()

if(CONFIG_SILABS_SISDK_WDOG)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S2  ${emlib_dir}/src/em_wdog.c)
  zephyr_library_sources(${peripheral_dir}/src/sl_hal_wdog.c)
endif()

if(CONFIG_SILABS_SISDK_SE)
  zephyr_include_directories(
    ${security_dir}/sl_component/se_manager/src
    ${security_dir}/sl_component/se_manager/inc
  )

  zephyr_library_sources(
    ${security_dir}/sl_component/se_manager/src/sl_se_manager.c
    ${security_dir}/sl_component/se_manager/src/sl_se_manager_util.c
    ${security_dir}/sl_component/se_manager/src/sli_se_manager_mailbox.c
  )
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_SILABS_S3
    ${security_dir}/sl_component/se_manager/src/sl_se_manager_extmem.c
    ${security_dir}/sl_component/se_manager/src/sli_se_manager_device_data.c
  )

  zephyr_compile_definitions(
    SL_CODE_COMPONENT_SE_MANAGER=se_manager
    SL_CODE_COMPONENT_PSEC_OSAL=psec_osal
  )
endif()

zephyr_library_sources_ifdef(CONFIG_ENTROPY_GECKO_SE
  ${security_dir}/sl_component/se_manager/src/sl_se_manager_entropy.c
)

zephyr_library_sources(
  src/sl_interrupt_manager_shim.c
  src/sl_memory_manager_shim.c
  src/sl_memory_manager_pool_shim.c
)

zephyr_library_sources_ifdef(
  CONFIG_BUILD_ONLY_NO_BLOBS
  src/blob_stubs.c
)
if(CONFIG_SOC_VENDOR_SILABS_DEVICE_IS_MODULE)
  zephyr_library_sources_ifdef(
    CONFIG_BUILD_ONLY_NO_BLOBS
    src/blob_stubs_module.c
  )
endif()

if(CONFIG_SOC_FAMILY_SILABS_S2)
  zephyr_linker_sources(ROM_SECTIONS linker/code_classification_text.ld)
  zephyr_linker_sources(RAMFUNC_SECTION linker/code_classification_ramfunc.ld)
endif()
if(CONFIG_SOC_FAMILY_SILABS_S3)
  zephyr_linker_sources(RAMFUNC_SECTION linker/code_classification_s3.ld)
endif()
