# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

set(MCUX_SDK_PROJECT_NAME ${ZEPHYR_CURRENT_LIBRARY})

# Translate the SoC name and part number into the mcux device and cpu
# name respectively.
# When this code completes, the following variables will be defined:
# MCUX_DEVICE: SOC name, suffixed by core name when using a dual core part.
#              Example: MIMXRT595S_cm33, or LPC55S36
# MCUX_CPU: "CPU"+ SOC part number, followed by core name when using a dual core part.
#              Example: CPU_MIMXRT595SFAWC_cm33, or  CPU_LPC55S36JBD100
# MCU_DEVICE_PATH: SOC name without core suffix. Must match the name of the
#              folder in MCUX HAL. IE MIMXRT595S, or LPC55S36

if(NOT HWMv2)
  # Include HWMv1 logic for MCUX variables
  include(${CMAKE_CURRENT_LIST_DIR}/hwmv1.cmake)
else()
  string(TOUPPER ${CONFIG_SOC} MCUX_DEVICE_PATH)
  string(TOUPPER ${CONFIG_SOC} MCUX_DEVICE)
  set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER})

  if(DEFINED CONFIG_MCUX_CORE_SUFFIX)
    string(APPEND MCUX_DEVICE ${CONFIG_MCUX_CORE_SUFFIX})
    string(APPEND MCUX_CPU ${CONFIG_MCUX_CORE_SUFFIX})
  endif()
endif()

if(DEFINED CONFIG_SOC_SDKNG_UNSUPPORTED)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH})
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers)

  # include CMSIS of mcux-sdk for Cortex-A
  if(CONFIG_CPU_CORTEX_A)
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/CMSIS/Core_AArch64/Include)
  endif()

  # The mcux uses the cpu name to expose SoC-specific features of a
  # given peripheral. For example, the UART peripheral may be
  # instantiated with/without a hardware FIFO, and the size of that FIFO
  # may be different for each instance in a given SoC. See
  # fsl_device_registers.h and ${MCUX_DEVICE}_features.h
  zephyr_compile_definitions(${MCUX_CPU})

  # Build mcux device-specific objects. Although it is not normal
  # practice, drilling down like this avoids the need for repetitive
  # build scripts for every mcux device.
  zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers/fsl_clock.c)

  if(${MCUX_DEVICE} MATCHES "LPC")
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers/fsl_power.c)
    zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers/fsl_reset.c)
  endif()

  zephyr_library_sources_ifdef(CONFIG_HAS_MCUX_AUDIOMIX
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers/fsl_audiomix.c
    )
  zephyr_library_sources_ifdef(CONFIG_SOC_LPC54114_M4
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/gcc/startup_LPC54114_cm4.S
    )

  # Include middleware/usb
  if(CONFIG_UDC_DRIVER OR CONFIG_USB_DEVICE_DRIVER)
    if(CONFIG_DT_HAS_NXP_USBPHY_ENABLED)
      zephyr_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/phy)
    endif()
    # Include usb required header file.
    zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/mcux_sdk_ng/middleware)
  endif()

  # Include middleware/bt_controller.cmake
  if(CONFIG_BT_H4_NXP_CTLR)
    include(${CMAKE_CURRENT_LIST_DIR}/mcux_sdk_ng/middleware/bt_controller.cmake)
  endif()

  add_subdirectory(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/components hal_nxp/components)

  # Include Entry cmake component
  add_subdirectory(mcux-sdk)

else() # CONFIG_SOC_SDKNG_UNSUPPORTED

 if(CONFIG_CPU_CORTEX_A)
   include_driver_ifdef(CONFIG_HAS_MCUX_CACHE  cache/armv8-a   driver_cache_armv8a)
 endif()

 if(CONFIG_NXP_RF_IMU AND CONFIG_SOC_SERIES_MCXW)
  zephyr_compile_definitions(HAL_RPMSG_SELECT_ROLE=0U)
 endif()

 if((${MCUX_DEVICE} MATCHES "RW61") AND (NOT DEFINED CONFIG_MINIMAL_LIBC))
  # Whenever building for RW61x without minimal LIBC, use optimized memcpy.
  # This will avoid issues with unaligned access to peripheral RAM regions
  # caused by the memcpy implementation in newlib
  zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/utilities/misc_utilities/fsl_memcpy.S)
 endif()

 add_subdirectory(mcux-sdk-ng)

endif() # CONFIG_SOC_SDKNG_UNSUPPORTED

enable_language(C ASM)

zephyr_linker_sources(RWDATA
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/quick_access_data.ld
  )

zephyr_linker_sources_ifdef(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
  RAMFUNC_SECTION
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/quick_access_code.ld
  )

zephyr_linker_sources_ifdef(CONFIG_NOCACHE_MEMORY
  NOCACHE_SECTION
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/nocache.ld
  )

zephyr_library_compile_definitions_ifdef(CONFIG_NOCACHE_MEMORY
  __STARTUP_INITIALIZE_NONCACHEDATA
  )

# MCUX_SDK and MCUX_SDK_NG share wireless/framework_5.3.3, wifi_nxp
add_subdirectory_ifdef(
  CONFIG_IEEE802154_KW41Z
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/wireless/framework_5.3.3
  hal_nxp/middleware/wireless/framework
  )

add_subdirectory_ifdef(
  CONFIG_WIFI_NXP
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/wifi_nxp
  hal_nxp/middleware/wifi_nxp
  )

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware
  hal_nxp/middleware
  )
