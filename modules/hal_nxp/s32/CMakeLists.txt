# Copyright 2022-2025 NXP

# Set the SoC specific drivers and configuration to build
if(HWMv2)
  set(SOC_BASE ${CONFIG_SOC})
else()
  if(${CONFIG_SOC} STREQUAL "s32z27")
    set(SOC_BASE "s32z270")
  else()
    set(SOC_BASE ${CONFIG_SOC})
  endif()
endif()

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/s32/drivers/${SOC_SERIES} hal_nxp/s32/drivers)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/s32/soc/${SOC_BASE} hal_nxp/s32/soc)

if(CONFIG_HAS_MCUX)

  # This is an adaptation from hal_nxp/mcux/CMakeLists.txt entry CMake in order
  # to build MCUX drivers together with RTD drivers for NXP S32 devices.
  # MCUX don't have official support for NXP S32 devices but we are leveraging
  # the existing shim drivers in Zephyr for those hardware blocks which are
  # present in both NXP S32 and non NXP S32 devices.
  #
  # Glue code must be added for each device supported to build with MCUX. This
  # can be found in hal_nxp/s32/mcux/devices:
  # - s32/mcux/devices/${MCUX_DEVICE}/${MCUX_DEVICE}_device.h
  #   redefine RTD module's peripheral access layers and its register masks to be
  #   compatible with MCUX drivers for incompatible peripherals
  # - s32/mcux/devices/${MCUX_DEVICE}/${MCUX_DEVICE}_features.h
  #   define SoC module's features
  # - s32/mcux/devices/${MCUX_DEVICE}/${MCUX_DEVICE}_glue_mcux.h
  #   redefine RTD module's base addresses/pointers/interrupts to be compatible
  #   with MCUX drivers
  # - s32/mcux/devices/${MCUX_DEVICE}/fsl_device_registers.h
  #   expose device features to the MCUX drivers
  # - s32/mcux/devices/${MCUX_DEVICE}/drivers/fsl_clock.h
  #   required by mcux/mcux-sdk/drivers/common/fsl_common_arm.h (at least)
  # - s32/mcux/devices/${MCUX_DEVICE}/drivers/driver_reset.cmake
  #   required by mcux/mcux-sdk/drivers/common/drivers_common.cmake
  # - s32/mcux/devices/${MCUX_DEVICE}/device_system.cmake
  #   required by mcux/hal_nxp.cmake
  # - s32/mcux/devices/${MCUX_DEVICE}/device_CMSIS.cmake
  #   required by mcux/mcux-sdk/drivers/common/drivers_common.cmake for non DSP
  #   architectures

  function(include_mcux_driver_ifdef feature_toggle directory module)
    if(${${feature_toggle}})
      list(APPEND CMAKE_MODULE_PATH
          ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/${directory}
      )
      zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/${directory})
      include(${module})
      message(STATUS "directory ${directory} included")
    endif()
  endfunction()

  set(MCUX_SDK_PROJECT_NAME ${ZEPHYR_CURRENT_LIBRARY})

  # Translate the SoC name into the MCUX device
  string(TOUPPER ${SOC_BASE} MCUX_DEVICE)

  # This is normally done in mcux/hal_nxp.cmake, but we need to point to the
  # path on this directory instead
  list(APPEND CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}/s32/mcux/devices/${MCUX_DEVICE}
    ${ZEPHYR_CURRENT_MODULE_DIR}/s32/mcux/devices/${MCUX_DEVICE}/drivers
  )

  # MCUX uses the CPU name to expose SoC-specific features of a given peripheral
  zephyr_compile_definitions(CPU_${MCUX_DEVICE})

  # Clock control is supported through RTD, so disable it in the MCUX drivers
  zephyr_compile_definitions(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL=1)

  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/s32/mcux/devices/${MCUX_DEVICE})
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/s32/mcux/devices/${MCUX_DEVICE}/drivers)

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

  # Entry CMake component for MCUX
  include(${CMAKE_CURRENT_LIST_DIR}/../mcux/mcux-sdk/CMakeLists.txt)

  if(${MCUX_DEVICE} MATCHES "S32K1")
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/port)
    include_mcux_driver_ifdef(CONFIG_PINCTRL port driver_port)

    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/sysmpu)
    include_mcux_driver_ifdef(CONFIG_ARM_MPU sysmpu driver_sysmpu)

    include_mcux_driver_ifdef(CONFIG_HAS_MCUX_CACHE cache/lmem driver_cache_lmem)
  endif()

endif()
