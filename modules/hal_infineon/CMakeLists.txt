# Copyright (c) 2020 Linumiz
# Copyright (c) 2022 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_XMCLIB
  OR CONFIG_SOC_FAMILY_PSOC6_LEGACY
  OR CONFIG_SOC_FAMILY_INFINEON_CAT1
  OR CONFIG_SOC_FAMILY_INFINEON_EDGE)

  zephyr_library_named(modules_hal_infineon)
  zephyr_library_compile_options($<TARGET_PROPERTY:compiler,warning_no_array_bounds>)

  zephyr_include_directories(.)
endif()

## Add PDL sources for XMC devices
if(CONFIG_HAS_XMCLIB)
  add_subdirectory(${ZEPHYR_HAL_INFINEON_MODULE_DIR}/XMCLib XMCLib)
endif()

if(CONFIG_SOC_FAMILY_INFINEON_CAT1
  OR CONFIG_SOC_FAMILY_PSOC6_LEGACY
  OR CONFIG_SOC_FAMILY_INFINEON_EDGE)

  ## Add core-lib sources for CAT1 devices
  add_subdirectory(core-lib)

  if(NOT CONFIG_SOC_FAMILY_INFINEON_EDGE)
    ## Add mtb-pdl-cat1 sources for CAT1 devices
    add_subdirectory(mtb-pdl-cat1)
  endif()

  ## Add mtb-templates-cat1 sources for CAT1 devices
  add_subdirectory(mtb-template-cat1)
endif()

if(CONFIG_SOC_FAMILY_INFINEON_CAT1
  AND NOT CONFIG_SOC_FAMILY_PSOC6_LEGACY
  AND NOT CONFIG_SOC_FAMILY_INFINEON_EDGE)

  ## Add mtb-hal-cat1 sources for CAT1 devices
  add_subdirectory(mtb-hal-cat1)

  ## Add catcm0p sleep images for CM0 Devices
  if(CONFIG_SOC_PSOC6_CM0P_IMAGE_SLEEP)
    add_subdirectory(cat1cm0p)
  endif()

  ## Add abstraction-rtos sources
  add_subdirectory(abstraction-rtos)

  add_subdirectory(serial-flash)

endif()

if(CONFIG_SOC_FAMILY_INFINEON_EDGE)
  if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
    # Use secure device in cy_device_headers
    zephyr_library_compile_definitions(COMPONENT_SECURE_DEVICE)
  endif()

  zephyr_library_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M33 COMPONENT_CM33)
  zephyr_library_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M33 CORE_NAME_CM33_0)
  zephyr_library_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M55 COMPONENT_CM55)
  zephyr_library_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M55 CORE_NAME_CM55_0)

  add_subdirectory(mtb-srf)
  add_subdirectory(serial-memory)
  add_subdirectory(zephyr-ifx-cycfg)
  add_subdirectory(mtb-dsl-pse8xxgp)
endif()

if(CONFIG_SOC_FAMILY_INFINEON_CAT1C)
  zephyr_library_compile_definitions_ifdef(CONFIG_SOC_XMC7200_CORE_NAME_M7_0 CORE_NAME_CM7_0)
  zephyr_library_compile_definitions_ifdef(CONFIG_SOC_XMC7200_CORE_NAME_M7_1 CORE_NAME_CM7_1)
endif()

if(CONFIG_SOC_SERIES_PSC3)
  add_subdirectory(zephyr-ifx-cycfg)
endif()

## Add Wi-Fi assets for AIROC devices
if(CONFIG_WIFI_AIROC)
  add_subdirectory(whd-expansion)

  ## Add core-lib sources for CAT1 devices
  add_subdirectory_ifndef(CONFIG_SOC_FAMILY_INFINEON_CAT1 core-lib)

  ## Add abstraction-rtos sources
  add_subdirectory_ifndef(CONFIG_SOC_FAMILY_INFINEON_CAT1 abstraction-rtos)
endif()

## Add BT assets for AIROC devices
if(CONFIG_BT_AIROC)
  add_subdirectory(btstack-integration)
endif()

if(CONFIG_BT_PSOC6_BLESS)
  add_subdirectory(bless)
endif()
