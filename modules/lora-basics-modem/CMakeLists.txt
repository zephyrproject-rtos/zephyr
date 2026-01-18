# Copyright (c) 2025 Embeint Inc
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USE_LORA_BASICS_MODEM_DRIVERS)

  set(LORA_BASICS_MODEM_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  set(LBM_LIB_DIR ${LORA_BASICS_MODEM_DIR}/lbm_lib)
  set(LBM_LIB_SMTC_MODEM_CORE_DIR ${LBM_LIB_DIR}/smtc_modem_core)
  set(LBM_LIB_SMTC_MODEM_HAL_DIR ${LBM_LIB_DIR}/smtc_modem_hal)
  set(LBM_LIB_RADIO_DRIVERS_DIR ${LBM_LIB_DIR}/smtc_modem_core/radio_drivers)

  set(LBM_SMTC_MODEM_HAL_DIR ${CMAKE_CURRENT_LIST_DIR}/smtc_modem_hal)

  if(TARGET lora-basics-modem)
    set(ZEPHYR_CURRENT_LIBRARY lora-basics-modem)
  else()
    zephyr_library_named(lora-basics-modem)
  endif()

  zephyr_include_directories(${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_ral/src)
  zephyr_include_directories(${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_ralf/src)
  zephyr_include_directories(${LBM_LIB_SMTC_MODEM_HAL_DIR})

  zephyr_include_directories(${LBM_SMTC_MODEM_HAL_DIR})

  if(CONFIG_LORA_BASICS_MODEM_HAL)
    zephyr_library_sources(${LBM_SMTC_MODEM_HAL_DIR}/smtc_modem_hal.c)
  endif()

  if(CONFIG_LORA_SX126X)
    include(sx126x.cmake)
  elseif(CONFIG_LORA_SX127X)
    include(sx127x.cmake)
  endif()
endif()
