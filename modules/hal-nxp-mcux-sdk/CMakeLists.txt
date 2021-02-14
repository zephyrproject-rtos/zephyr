# Copyright 2021 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MCUX_SDK)
  set(MCUX_SDK_PROJECT_NAME mcux_sdk)

  set(CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}
    ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr
  )

  string(TOUPPER ${CONFIG_SOC} MCUX_DEVICE)
  if("${MCUX_DEVICE}" STREQUAL "LPC54114")
    set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER}_cm4)
  elseif("${MCUX_DEVICE}" STREQUAL "LPC54114_M0")
    set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER}_cm0plus)
    set(MCUX_DEVICE LPC54114)
  elseif("${MCUX_DEVICE}" STREQUAL "LPC55S16")
    set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER})
    set(MCUX_DEVICE LPC55S16)
  elseif("${MCUX_DEVICE}" STREQUAL "LPC55S69_CPU0")
    set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER}_cm33_core0)
    set(MCUX_DEVICE LPC55S69_cm33_core0)
  elseif("${MCUX_DEVICE}" STREQUAL "LPC55S69_CPU1")
    set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER}_cm33_core1)
    set(MCUX_DEVICE LPC55S69_cm33_core1)
  elseif("${MCUX_DEVICE}" STREQUAL "MIMXRT1052")
    string(REGEX REPLACE "(.*)[AB]$" "CPU_\\1B" MCUX_CPU ${CONFIG_SOC_PART_NUMBER})
  elseif("${MCUX_DEVICE}" STREQUAL "MIMXRT685S_CM33")
    set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER}_cm33)
    set(MCUX_DEVICE MIMXRT685S)
  else()
    set(MCUX_CPU CPU_${CONFIG_SOC_PART_NUMBER})
  endif()

  message(STATUS "Using MCUXpresso SDK for device: ${MCUX_DEVICE}")
  # TODO: can we get the SDK version number as a cmake variable?

  zephyr_library_named(${MCUX_SDK_PROJECT_NAME})

  include(all_devices)
  include(all_drivers)

  zephyr_include_directories(
    ${CMAKE_MODULE_PATH}
  )

  zephyr_compile_definitions(
    ${MCUX_CPU}
  )

endif()
