# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

set(MCUX_SDK_PROJECT_NAME ${ZEPHYR_CURRENT_LIBRARY})

if(CONFIG_SOC_LPC11U66
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
    OR CONFIG_SOC_MIMX9596_M7
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
    OR CONFIG_SOC_MIMX9131
    )

    set (CONFIG_SOC_SDKNG_UNSUPPORTED ON)

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

  # The mcux uses the cpu name to expose SoC-specific features
  # of a given peripheral. For example, the UART peripheral may be
  # instantiated with/without a hardware FIFO, and the size of that
  # FIFO may be different for each instance in a given SoC. See
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

  add_subdirectory(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/components ${CMAKE_CURRENT_BINARY_DIR}/components)

  # Include Entry cmake component
  add_subdirectory(mcux-sdk)

else() # CONFIG_SOC_SDKNG_UNSUPPORTED

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

if(NOT CONFIG_ASSERT OR CONFIG_FORCE_NO_ASSERT)
  zephyr_compile_definitions(NDEBUG) # squelch fsl_flexcan.c warning
endif()

zephyr_compile_definitions_ifdef(
  CONFIG_PTP_CLOCK_NXP_ENET
  ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
  )

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_MIMX9596
  FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL=1
  )

# note: if FSL_IRQSTEER_ENABLE_MASTER_INT is not
# defined then it will automatically be defined
# and set to 1 via fsl_irqsteer.h
zephyr_library_compile_definitions_ifdef(CONFIG_NXP_IRQSTEER
  FSL_IRQSTEER_ENABLE_MASTER_INT=0
  )

zephyr_library_compile_definitions_ifdef(CONFIG_DAI_NXP_SAI
  MCUX_SDK_SAI_ALLOW_NULL_FIFO_WATERMARK=1
  )

zephyr_library_compile_definitions_ifdef(CONFIG_DAI_NXP_SAI
  MCUX_SDK_SAI_DISABLE_IMPLICIT_CHAN_CONFIG=1
  )

zephyr_library_compile_definitions_ifdef(CONFIG_NOCACHE_MEMORY
  __STARTUP_INITIALIZE_NONCACHEDATA
  )

zephyr_library_compile_definitions_ifdef(CONFIG_HAS_MCUX_CACHE
  FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL
  )

# MCUX_SDK and MCUX_SDK_NG share wireless/framework_5.3.3, wifi_nxp
add_subdirectory_ifdef(CONFIG_IEEE802154_KW41Z
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/wireless/framework_5.3.3
  ${CMAKE_CURRENT_BINARY_DIR}/middleware/wireless/framework
  )

add_subdirectory_ifdef(CONFIG_WIFI_NXP
  ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/wifi_nxp
  ${CMAKE_CURRENT_BINARY_DIR}/middleware/wifi_nxp
  )

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware
  ${CMAKE_CURRENT_BINARY_DIR}/middleware
  )
