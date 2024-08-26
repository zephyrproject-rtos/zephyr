# Copyright (c) 2024 Semtech Corporation
# SPDX-License-Identifier: Apache-2.0

set(LBM_LR11XX_DIR ${LBM_RADIO_DRIVERS_DIR}/lr11xx_driver/src)

#-----------------------------------------------------------------------------
# Radio specific sources
#-----------------------------------------------------------------------------

zephyr_library_sources(
  ${LBM_LR11XX_DIR}/lr11xx_bootloader.c
  ${LBM_LR11XX_DIR}/lr11xx_crypto_engine.c
  ${LBM_LR11XX_DIR}/lr11xx_driver_version.c
  ${LBM_LR11XX_DIR}/lr11xx_radio.c
  ${LBM_LR11XX_DIR}/lr11xx_regmem.c
  ${LBM_LR11XX_DIR}/lr11xx_system.c
  ${LBM_LR11XX_DIR}/lr11xx_lr_fhss.c
)

# LR1121 is the only one not supporting GNSS and WiFi
if(CONFIG_DT_HAS_SEMTECH_LR1110_ENABLED OR CONFIG_DT_HAS_SEMTECH_LR1120_ENABLED)
  zephyr_library_sources(
    ${LBM_LR11XX_DIR}/lr11xx_wifi.c
    ${LBM_LR11XX_DIR}/lr11xx_gnss.c
  )
endif()

set(LBM_RAL_SOURCES ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ral/src/ral_lr11xx.c)
set(LBM_RALF_SOURCES ${LBM_SMTC_MODEM_CORE_DIR}/smtc_ralf/src/ralf_lr11xx.c)

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/lr11xx_crypto_engine/lr11xx_ce.c
)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX_WITH_CREDENTIALS
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/lr11xx_crypto_engine/lr11xx_ce.c
)

#-----------------------------------------------------------------------------
# Includes
#-----------------------------------------------------------------------------

# Used in publicly-included headers
zephyr_include_directories(${LBM_LR11XX_DIR})

zephyr_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/lr11xx_crypto_engine
)
zephyr_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX_WITH_CREDENTIALS
  ${LBM_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/lr11xx_crypto_engine
)

#-----------------------------------------------------------------------------
# Radio specific compilation flags
#-----------------------------------------------------------------------------

zephyr_compile_definitions(LR11XX)

zephyr_library_compile_definitions(LR11XX_TRANSCEIVER LR11XX_DISABLE_WARNINGS)

# Used in publicly-included headers
zephyr_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX
  USE_LR11XX_CE)

zephyr_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX_WITH_CREDENTIALS
  USE_LR11XX_CE)

zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX_WITH_CREDENTIALS
  USE_PRE_PROVISIONED_FEATURES)
