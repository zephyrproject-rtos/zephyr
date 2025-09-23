# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

include(../mcux-sdk-ng/basic.cmake)

# The mcux uses the cpu name to expose SoC-specific features
# of a given peripheral. For example, the UART peripheral may be
# instantiated with/without a hardware FIFO, and the size of that
# FIFO may be different for each instance in a given SoC. See
# fsl_device_registers.h and ${MCUX_DEVICE}_features.h
zephyr_compile_definitions(${MCUX_CPU})

list(APPEND CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers
)

zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH})
zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers)

if(CONFIG_CPU_CORTEX_A)
    list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/CMSIS/Core_AArch64/Include
    )

    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/CMSIS/Core_AArch64/Include)
endif()

function(include_driver_ifdef feature_toggle directory module)
  if(${${feature_toggle}})
    list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/${directory}
    )
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/${directory})
    include(${module})
  endif()
endfunction()

message("Load components for ${MCUX_DEVICE}:")

# Required by all SCFW-based SoCs
if (CONFIG_SOC_MIMX8QM6_ADSP OR CONFIG_SOC_MIMX8QX6_ADSP)
    list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/scfw_api
    )
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/scfw_api)
    include(driver_scfw_api)
endif()

add_subdirectory(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/components ${CMAKE_CURRENT_BINARY_DIR}/components)

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

#Include system_xxx file
#This can be extended to other SoC series if needed
if (DEFINED CONFIG_SOC_RESET_HOOK)
  if (CONFIG_SOC_LPC54114_M4)
    include(device_system_LPC54114_cm4)
  elseif (CONFIG_SOC_LPC54114_M0)
    include(device_system_LPC54114_cm0plus)
  else()
    include(device_system)
  endif()
endif()

include(../mcux-sdk-ng/middleware/middleware.cmake)
include(../mcux-sdk-ng/components/components.cmake)
include(../mcux-sdk-ng/drivers/drivers.cmake)

include(../mcux-sdk-ng/fixup.cmake)
