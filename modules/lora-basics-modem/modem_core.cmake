# Copyright (c) 2026 RAKwireless Technology Limited
# SPDX-License-Identifier: Apache-2.0
#
# Sources and defines of the LoRa Basics Modem core.

set(LBM_CORE ${LBM_LIB_SMTC_MODEM_CORE_DIR})

zephyr_include_directories(
  ${LBM_LIB_DIR}
  ${LBM_CORE}
  ${LBM_CORE}/logging
  ${LBM_CORE}/lorawan_api
  ${LBM_CORE}/lorawan_manager
  ${LBM_CORE}/lorawan_packages/lorawan_certification
  ${LBM_CORE}/lr1mac
  ${LBM_CORE}/lr1mac/src
  ${LBM_CORE}/lr1mac/src/services
  ${LBM_CORE}/lr1mac/src/smtc_real/src
  ${LBM_CORE}/modem_supervisor
  ${LBM_CORE}/modem_utilities
  ${LBM_CORE}/radio_planner/src
  ${LBM_CORE}/smtc_modem_crypto
  ${LBM_CORE}/smtc_modem_crypto/smtc_secure_element
  ${LBM_LIB_DIR}/smtc_modem_api
)

zephyr_library_sources(
  ${LBM_CORE}/lorawan_api/lorawan_api.c
  ${LBM_CORE}/lorawan_manager/lorawan_cid_request_management.c
  ${LBM_CORE}/lorawan_manager/lorawan_dwn_ack_management.c
  ${LBM_CORE}/lorawan_manager/lorawan_join_management.c
  ${LBM_CORE}/lorawan_manager/lorawan_send_management.c
  ${LBM_CORE}/lorawan_packages/lorawan_certification/lorawan_certification.c
  ${LBM_CORE}/lr1mac/src/lr1_stack_mac_layer.c
  ${LBM_CORE}/lr1mac/src/lr1mac_core.c
  ${LBM_CORE}/lr1mac/src/lr1mac_utilities.c
  ${LBM_CORE}/lr1mac/src/services/smtc_duty_cycle.c
  ${LBM_CORE}/lr1mac/src/services/smtc_lbt.c
  ${LBM_CORE}/lr1mac/src/smtc_real/src/smtc_real.c
  ${LBM_CORE}/modem_supervisor/modem_supervisor_light.c
  ${LBM_CORE}/modem_supervisor/modem_tx_protocol_manager.c
  ${LBM_CORE}/modem_utilities/fifo_ctrl.c
  ${LBM_CORE}/modem_utilities/modem_core.c
  ${LBM_CORE}/modem_utilities/modem_event_utilities.c
  ${LBM_CORE}/radio_planner/src/radio_planner.c
  ${LBM_CORE}/smtc_modem.c
  ${LBM_CORE}/smtc_modem_test.c
  ${LBM_CORE}/smtc_modem_crypto/smtc_modem_crypto.c
  ${LBM_CORE}/smtc_modem_crypto/soft_secure_element/aes.c
  ${LBM_CORE}/smtc_modem_crypto/soft_secure_element/cmac.c
  ${LBM_CORE}/smtc_modem_crypto/soft_secure_element/soft_se.c
)

zephyr_library_compile_definitions(
  RP2_103
  NUMBER_OF_STACKS=1
  ADD_SMTC_PATCH_FILE
)

# One region source and one define per region the application selected.
function(lbm_region kconfig macro file)
  if(${kconfig})
    zephyr_library_compile_definitions(${macro})
    zephyr_library_sources(${LBM_CORE}/lr1mac/src/smtc_real/src/${file})
    set(lbm_region_selected TRUE PARENT_SCOPE)
  endif()
endfunction()

lbm_region(CONFIG_LORAWAN_REGION_AS923 REGION_AS_923 region_as_923.c)
lbm_region(CONFIG_LORAWAN_REGION_AU915 REGION_AU_915 region_au_915.c)
lbm_region(CONFIG_LORAWAN_REGION_CN470 REGION_CN_470 region_cn_470.c)
lbm_region(CONFIG_LORAWAN_REGION_EU868 REGION_EU_868 region_eu_868.c)
lbm_region(CONFIG_LORAWAN_REGION_IN865 REGION_IN_865 region_in_865.c)
lbm_region(CONFIG_LORAWAN_REGION_KR920 REGION_KR_920 region_kr_920.c)
lbm_region(CONFIG_LORAWAN_REGION_RU864 REGION_RU_864 region_ru_864.c)
lbm_region(CONFIG_LORAWAN_REGION_US915 REGION_US_915 region_us_915.c)

if(NOT lbm_region_selected)
  message(FATAL_ERROR "The modem core needs a region. Select one of the "
                      "LORAWAN_REGION_* options the modem supports.")
endif()
