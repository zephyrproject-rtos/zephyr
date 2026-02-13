# Copyright (c) 2020 Linumiz
# Copyright (c) 2022 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_XMCLIB OR CONFIG_SOC_FAMILY_PSOC6_LEGACY OR CONFIG_SOC_FAMILY_INFINEON_CAT1)
  zephyr_library_named(modules_hal_infineon)
  zephyr_library_compile_options($<TARGET_PROPERTY:compiler,warning_no_array_bounds>)
endif()

## Add PDL sources for XMC devices
if (CONFIG_HAS_XMCLIB)
  add_subdirectory(${ZEPHYR_HAL_INFINEON_MODULE_DIR}/XMCLib XMCLib)
endif()

if (CONFIG_SOC_FAMILY_INFINEON_CAT1 OR CONFIG_SOC_FAMILY_PSOC6_LEGACY)
  ## Add core-lib sources for CAT1 devices
  add_subdirectory(core-lib)
   ## Add mtb-hal-cat1 sources for CAT1 devices
  #add_subdirectory(mtb-hal-cat1)
  ## Add mtb-pdl-cat1 sources for CAT1 devices
  add_subdirectory(mtb-pdl-cat1)
  if(CONFIG_SOC_FAMILY_INFINEON_CAT1C)
    add_custom_command(TARGET modules_hal_infineon POST_BUILD
        COMMAND ${CMAKE_OBJCOPY}
          -W Cy_Srom_SetResponseHandler
          -W Cy_SysInt_Init
          -W Cy_SysInt_SetSystemIrqVector
          -W Cy_SysInt_GetSystemIrqVector
          $<TARGET_FILE:modules_hal_infineon>
        COMMENT "Set weak for PDL symbols to hook into Zephyr APIs"
    )
  endif()

  ## Add mtb-templates-cat1 sources for CAT1 devices
  add_subdirectory(mtb-template-cat1)
  if (CONFIG_SOC_SERIES_CYT4DN)
    add_subdirectory(serial-memory-ms)
    add_subdirectory(zephyr-ifx-cycfg)
    add_subdirectory(mtb-hal-t2gc2d6m)
  endif()
endif()

if (CONFIG_SOC_FAMILY_INFINEON_CAT1 AND NOT CONFIG_SOC_FAMILY_PSOC6_LEGACY)
  ## Add mtb-hal-cat1 sources for CAT1 devices
  if (NOT CONFIG_SOC_SERIES_CYT4DN)
    add_subdirectory(mtb-hal-cat1)
  endif()

  ## Add catcm0p sleep images for CM0 Devices
  if(CONFIG_SOC_PSOC6_CM0P_IMAGE_SLEEP OR CONFIG_SOC_CYT4DN_CM0P_IMAGE_SLEEP OR CONFIG_SOC_CYT2B7_CM0P_IMAGE_SLEEP OR CONFIG_SOC_CYT2BL_CM0P_IMAGE_SLEEP)
    add_subdirectory(cat1cm0p)
  endif()

  ## Add abstraction-rtos sources
  add_subdirectory(abstraction-rtos)

if (NOT CONFIG_SOC_SERIES_CYT4DN)
add_subdirectory(serial-flash)
endif()

endif()

## Add Wi-Fi assets for AIROC devices
if (CONFIG_WIFI_AIROC)
  add_subdirectory(wifi-host-driver)

  ## Add core-lib sources for CAT1 devices
  add_subdirectory_ifndef(CONFIG_SOC_FAMILY_INFINEON_CAT1 core-lib)

  ## Add abstraction-rtos sources
  add_subdirectory_ifndef(CONFIG_SOC_FAMILY_INFINEON_CAT1 abstraction-rtos)
endif()

## Add BT assets for AIROC devices
if (CONFIG_BT_AIROC)
  add_subdirectory(btstack-integration)
endif()

if (CONFIG_BT_PSOC6_BLESS)
add_subdirectory(bless)
endif()
