# Copyright (c) 2024 Semtech Corporation - Félix Piédallu
# SPDX-License-Identifier: Apache-2.0

# Used in publicly-included headers
zephyr_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_RELAY_TX
  RELAY_TX
)

zephyr_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_RELAY_TX
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/common
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/relay_tx
  ${LBM_SMTC_MODEM_CORE_DIR}/modem_services/relay_service
)

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_RELAY_TX
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/common/wake_on_radio.c
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/common/relay_real.c
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/common/wake_on_radio_ral.c
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/common/relay_mac_parser.c
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/relay_tx/relay_tx_mac_parser.c
  ${LORA_BASICS_MODEM_CORE_DIR}/lr1mac/src/relay/relay_tx/relay_tx.c
  ${LBM_SMTC_MODEM_CORE_DIR}/modem_services/relay_service/lorawan_relay_tx_service.c
)
