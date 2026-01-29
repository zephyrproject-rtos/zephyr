# Copyright (c) 2024 Semtech Corporation - Félix Piédallu
# Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

#-----------------------------------------------------------------------------
# Regions
#-----------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/regions.cmake)

#-----------------------------------------------------------------------------
# Common
#-----------------------------------------------------------------------------

zephyr_library_compile_definitions(NUMBER_OF_STACKS=${CONFIG_LORA_BASICS_MODEM_NUMBER_OF_STACKS})

# Random delay configuration for TX collision avoidance
zephyr_library_compile_definitions(MODEM_MIN_RANDOM_DELAY_MS=${CONFIG_LORA_BASICS_MODEM_MIN_RANDOM_DELAY_MS})
zephyr_library_compile_definitions(MODEM_MAX_RANDOM_DELAY_MS=${CONFIG_LORA_BASICS_MODEM_MAX_RANDOM_DELAY_MS})

# SMTC_MODEM_CORE_C_SOURCES
zephyr_library_sources(
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_api/lorawan_api.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_test.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/modem_utilities/modem_event_utilities.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/modem_utilities/fifo_ctrl.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/modem_utilities/modem_core.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/modem_supervisor/modem_supervisor_light.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/modem_supervisor/modem_tx_protocol_manager.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_packages/lorawan_certification/lorawan_certification.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_manager/lorawan_join_management.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_manager/lorawan_send_management.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_manager/lorawan_cid_request_management.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_manager/lorawan_dwn_ack_management.c
)

# LR1MAC_C_SOURCES
zephyr_library_sources(
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1_stack_mac_layer.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1mac_core.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1mac_utilities.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/smtc_real/src/smtc_real.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/services/smtc_duty_cycle.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/services/smtc_lbt.c
)

# SMTC_MODEM_CRYPTO_C_SOURCES
zephyr_library_sources(
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/smtc_modem_crypto.c
)

# RADIO_PLANNER_C_SOURCES
zephyr_library_sources(
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/radio_planner/src/radio_planner.c
)

zephyr_library_include_directories(
  ${LBM_LIB_DIR}
  ${LBM_LIB_DIR}/smtc_modem_api
  ${LBM_LIB_DIR}/smtc_modem_hal
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/logging
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_api
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_manager
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_packages
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_packages/lorawan_certification
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/services
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/smtc_real/src
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/modem_supervisor
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/modem_utilities
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/radio_planner/src
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/smtc_secure_element
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_ral/src
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_ralf/src
)

#-----------------------------------------------------------------------------
# Crypto soft
#-----------------------------------------------------------------------------

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_SOFT
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/soft_secure_element/aes.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/soft_secure_element/cmac.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/soft_secure_element/soft_se.c
)
zephyr_library_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_SOFT
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/smtc_modem_crypto/soft_secure_element
)

#-----------------------------------------------------------------------------
# Class B
#-----------------------------------------------------------------------------

zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CLASS_B
  ADD_CLASS_B
)

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CLASS_B
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lorawan_manager/lorawan_class_b_management.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1mac_class_b/smtc_beacon_sniff.c
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1mac_class_b/smtc_ping_slot.c
)

zephyr_library_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_CLASS_B
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1mac_class_b
)

#-----------------------------------------------------------------------------
# Class C
#-----------------------------------------------------------------------------

zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CLASS_C
  ADD_CLASS_C
)

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CLASS_C
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1mac_class_c/lr1mac_class_c.c
)

zephyr_library_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_CLASS_C
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/lr1mac_class_c
)

#-----------------------------------------------------------------------------
# Multicast
#-----------------------------------------------------------------------------

zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_MULTICAST
  SMTC_MULTICAST
)

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_MULTICAST
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/services/smtc_multicast/smtc_multicast.c
)

zephyr_library_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_MULTICAST
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/services/smtc_multicast
)

#-----------------------------------------------------------------------------
# CSMA
#-----------------------------------------------------------------------------

zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CSMA
  ADD_CSMA
)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CSMA_BY_DEFAULT
  ENABLE_CSMA_BY_DEFAULT
)

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CSMA
  ${LBM_LIB_SMTC_MODEM_CORE_DIR}/lr1mac/src/services/smtc_lora_cad_bt.c
)

#-----------------------------------------------------------------------------
# Debug trace
#-----------------------------------------------------------------------------

if(CONFIG_LORA_BASICS_MODEM_DBG_TRACE)
  zephyr_library_compile_definitions(MODEM_HAL_DBG_TRACE=1)
else()
  zephyr_library_compile_definitions(MODEM_HAL_DBG_TRACE=0)
endif()

if(CONFIG_LORA_BASICS_MODEM_DEEP_DBG_TRACE)
  zephyr_library_compile_definitions(MODEM_HAL_DEEP_DBG_TRACE=1)
else()
  zephyr_library_compile_definitions(MODEM_HAL_DEEP_DBG_TRACE=0)
endif()
