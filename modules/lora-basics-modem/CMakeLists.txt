# Copyright (c) 2025 Embeint Inc
# Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ZEPHYR_LORA_BASICS_MODEM_MODULE)
  # Radio drivers and HAL (for porting tests and basic radio functionality)
  if(CONFIG_LORA_MODULE_BACKEND_LORA_BASICS_MODEM)
    set(LORA_BASICS_MODEM_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
    set(LBM_LIB_DIR ${LORA_BASICS_MODEM_DIR}/lbm_lib)
    set(LBM_LIB_SMTC_MODEM_CORE_DIR ${LBM_LIB_DIR}/smtc_modem_core)
    set(LBM_LIB_SMTC_MODEM_HAL_DIR ${LBM_LIB_DIR}/smtc_modem_hal)
    set(LBM_LIB_RADIO_DRIVERS_DIR ${LBM_LIB_DIR}/smtc_modem_core/radio_drivers)

    # Zephyr HAL implementation
    set(LBM_SMTC_MODEM_HAL_DIR ${CMAKE_CURRENT_LIST_DIR}/smtc_modem_hal)

    if(TARGET lora-basics-modem)
      set(ZEPHYR_CURRENT_LIBRARY lora-basics-modem)
    else()
      zephyr_library_named(lora-basics-modem)
    endif()

    # RAL/RALF and HAL include directories
    zephyr_library_include_directories(
      ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_ral/src
      ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_ralf/src
      ${LBM_LIB_SMTC_MODEM_HAL_DIR}
      ${LBM_SMTC_MODEM_HAL_DIR}
    )

    # HAL implementation
    if(CONFIG_LORA_BASICS_MODEM_HAL)
      zephyr_library_sources(${LBM_SMTC_MODEM_HAL_DIR}/smtc_modem_hal.c)
    endif()

    # Radio drivers
    if(CONFIG_LORA_SX126X)
      include(${CMAKE_CURRENT_LIST_DIR}/sx126x.cmake)
    elseif(CONFIG_LORA_SX127X)
      include(${CMAKE_CURRENT_LIST_DIR}/sx127x.cmake)
    endif()

    # Full LBM LoRaWAN stack (only when LORAWAN subsystem is enabled)
    if(CONFIG_LORAWAN)
      include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
    endif()
  endif()
endif()
