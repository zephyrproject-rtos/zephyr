# Copyright (c) 2024 Semtech Corporation - Félix Piédallu
# SPDX-License-Identifier: Apache-2.0

# Used by LBM
zephyr_library_compile_definitions(SX126X)
zephyr_library_compile_definitions(SX126X_TRANSCEIVER)
zephyr_library_compile_definitions(SX126X_DISABLE_WARNINGS)
zephyr_library_compile_definitions_ifdef(CONFIG_DT_HAS_SEMTECH_SX1261_NEW_ENABLED SX1261)
zephyr_library_compile_definitions_ifdef(CONFIG_DT_HAS_SEMTECH_SX1262_NEW_ENABLED SX1262)
zephyr_library_compile_definitions_ifdef(CONFIG_DT_HAS_SEMTECH_SX1268_NEW_ENABLED SX1268)

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
)

# Used later
set(LBM_RAL_SOURCES ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ral/src/ral_sx126x.c)
set(LBM_RALF_SOURCES ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ralf/src/ralf_sx126x.c)
