# Copyright (c) 2025 Embeint Inc
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USE_LORA_BASICS_MODEM_DRIVERS)

  set(LORA_BASICS_MODEM_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
  set(LBM_LIB_DIR ${LORA_BASICS_MODEM_DIR}/lbm_lib)
  set(LBM_SMTC_MODEM_CORE_DIR ${LBM_LIB_DIR}/smtc_modem_core)
  set(LBM_RADIO_DRIVERS_DIR ${LBM_LIB_DIR}/smtc_modem_core/radio_drivers)

  if(TARGET lora_basics_modem)
    set(ZEPHYR_CURRENT_LIBRARY lora_basics_modem)
  else()
    zephyr_library_named(lora_basics_modem)
  endif()

  zephyr_library_include_directories(${LBM_SMTC_MODEM_CORE_DIR}/smtc_ral/src)
  zephyr_library_include_directories(${LBM_SMTC_MODEM_CORE_DIR}/smtc_ralf/src)

  if(CONFIG_LORA_SX126X)
    include(sx126x.cmake)
  elseif(CONFIG_LORA_SX127X)
    include(sx127x.cmake)
  endif()
endif()
