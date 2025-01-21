# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

set(MCUX_SDK_PROJECT_NAME ${ZEPHYR_CURRENT_LIBRARY})

if (CONFIG_SOC_LPC11U66
    OR CONFIG_SOC_LPC11U68
    OR CONFIG_SOC_LPC54114
    OR CONFIG_SOC_MIMX8ML8_A53
    OR CONFIG_SOC_MIMX8ML8_ADSP
    OR CONFIG_SOC_MIMX8MM6_A53
    OR CONFIG_SOC_MIMX8MN6_A53
    OR CONFIG_SOC_MIMX8QM6
    OR CONFIG_SOC_MIMX8QX6
    OR CONFIG_SOC_MIMX8UD7_ADSP
    OR CONFIG_SOC_MIMX9352_A55
    OR CONFIG_SOC_MIMX9596_A55
    OR CONFIG_SOC_MK22F12
    OR CONFIG_SOC_MK64F12
    OR CONFIG_SOC_MK66F18
    OR CONFIG_SOC_MK80F25615
    OR CONFIG_SOC_MK82F25615
    OR CONFIG_SOC_MKE14F16
    OR CONFIG_SOC_MKE16F16
    OR CONFIG_SOC_MKE18F16
    OR CONFIG_SOC_MKL25Z4
    OR CONFIG_SOC_MKV56F24
    OR CONFIG_SOC_MKV58F24
    OR CONFIG_SOC_MKW22D5
    OR CONFIG_SOC_MKW24D5
    OR CONFIG_SOC_MKW40Z4
    OR CONFIG_SOC_MKW41Z4
    )

  set (CONFIG_SOC_SDKNG_UNSUPPORTED 1)

endif()

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
  # build scripts for every mcux device.  zephyr_library_sources(mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers/fsl_clock.c)
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

  # Include Entry cmake component
  add_subdirectory(mcux-sdk)

  # Include middleware
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware hal_nxp/middleware)

  if(CONFIG_UDC_DRIVER OR CONFIG_USB_DEVICE_DRIVER)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/mcux-sdk-ng/middleware hal_nxp/middleware/usb)
  endif()

  if(CONFIG_BT_H4_NXP_CTLR)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/mcux-sdk-ng/middleware hal_nxp/middleware/bt_controller)
  endif()

else() # CONFIG_SOC_SDKNG_UNSUPPORTED

 add_subdirectory(mcux-sdk-ng)

 ##TODO: Add kconfig and cmakelists for cache/armv8-a driver, than can deal this via kconfig in kconfig.driver.
 if(CONFIG_CPU_CORTEX_A)
   include_driver_ifdef(CONFIG_HAS_MCUX_CACHE  cache/armv8-a   driver_cache_armv8a)
 endif()

 if(CONFIG_NXP_RF_IMU AND CONFIG_SOC_SERIES_MCXW)
  zephyr_compile_definitions(HAL_RPMSG_SELECT_ROLE=0U)
 endif()

 if((${MCUX_DEVICE} MATCHES "RW61") AND (NOT DEFINED CONFIG_MINIMAL_LIBC))
  # Whenever building for RW61x without minimal LIBC, use optimized memcpy.
  # This will avoid issues with unaligned access to peripheral RAM regions
  # caused by the memcpy implmentation in newlib
  zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/utilities/misc_utilities/fsl_memcpy.S)
 endif()

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
