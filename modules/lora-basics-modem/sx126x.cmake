# Copyright (c) 2024 Semtech Corporation
# SPDX-License-Identifier: Apache-2.0

# Used by LBM
zephyr_library_compile_definitions(SX126X)
zephyr_library_compile_definitions(SX126X_TRANSCEIVER)

# Allow modem options
set(ALLOW_CSMA_BUILD true)

set(LBM_SX126X_LIB_DIR ${LBM_RADIO_DRIVERS_DIR}/sx126x_driver/src)
zephyr_include_directories(${LBM_SX126X_LIB_DIR})

#-----------------------------------------------------------------------------
# Radio specific sources
#-----------------------------------------------------------------------------
zephyr_library_sources(
  ${LBM_SX126X_LIB_DIR}/lr_fhss_mac.c
  ${LBM_SX126X_LIB_DIR}/sx126x.c
  ${LBM_SX126X_LIB_DIR}/sx126x_driver_version.c
  ${LBM_SX126X_LIB_DIR}/sx126x_lr_fhss.c
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ral/src/ral_sx126x.c
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ralf/src/ralf_sx126x.c
)
