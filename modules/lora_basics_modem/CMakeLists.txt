# Copyright (c) 2024 Semtech Corporation - Félix Piédallu
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ZEPHYR_LORA_BASICS_MODEM_MODULE)
  # Based on makefiles from Semtech

  set(LORA_BASICS_MODEM_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
  set(LBM_LIB_DIR ${LORA_BASICS_MODEM_DIR}/lbm_lib)
  set(LBM_SMTC_MODEM_CORE_DIR ${LBM_LIB_DIR}/smtc_modem_core)
  set(LBM_RADIO_DRIVERS_DIR ${LBM_LIB_DIR}/smtc_modem_core/radio_drivers)

  zephyr_library()

  if(CONFIG_SEMTECH_LR11XX)
    include(${CMAKE_CURRENT_LIST_DIR}/lr11xx.cmake)
  endif()

  if(CONFIG_SEMTECH_SX126X)
    include(${CMAKE_CURRENT_LIST_DIR}/sx126x.cmake)
  endif()

  if(CONFIG_LORA_BASICS_MODEM_DRIVERS_RAL_RALF)
    zephyr_include_directories(
      ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ral/src
      ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ralf/src
    )
    zephyr_library_sources(
      ${LBM_RAL_SOURCES}
      ${LBM_RALF_SOURCES}
    )
  endif()

  if(CONFIG_LORA_BASICS_MODEM)

    include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)

    zephyr_library_include_directories(
      ${CMAKE_CURRENT_LIST_DIR}/smtc_modem_hal
    )

    zephyr_library_sources(
      ${CMAKE_CURRENT_LIST_DIR}/smtc_modem_hal/smtc_modem_hal.c
      ${CMAKE_CURRENT_LIST_DIR}/smtc_modem_hal/smtc_modem_hal_storage.c
      ${CMAKE_CURRENT_LIST_DIR}/smtc_modem_hal/smtc_modem_hal_dbg_trace.c
    )

  endif()

endif()
