# Copyright (c) 2024 Semtech Corporation
# SPDX-License-Identifier: Apache-2.0

# Used by LBM
zephyr_library_compile_definitions(SX127X)
zephyr_library_compile_definitions(SX127X_TRANSCEIVER)

if(CONFIG_DT_HAS_SEMTECH_SX1272_ENABLED)
  zephyr_library_compile_definitions(SX1272)
endif()
if(CONFIG_DT_HAS_SEMTECH_SX1276_ENABLED)
  zephyr_library_compile_definitions(SX1276)
endif()

# Allow modem options
set(ALLOW_CSMA_BUILD true)

set(LBM_SX127X_LIB_DIR ${LBM_RADIO_DRIVERS_DIR}/sx127x_driver/src)
zephyr_include_directories(${LBM_SX127X_LIB_DIR})

#-----------------------------------------------------------------------------
# Radio specific sources
#-----------------------------------------------------------------------------
zephyr_library_sources(
  ${LBM_SX127X_LIB_DIR}/sx127x.c
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ral/src/ral_sx127x.c
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ralf/src/ralf_sx127x.c
)
