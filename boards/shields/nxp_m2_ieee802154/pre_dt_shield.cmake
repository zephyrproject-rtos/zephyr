# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0
#
# The nxp_m2_2el_2ll_ieee802154 shield requires one of the nxp_m2_wifi_bt
# base shields to be present for BT UART H4 and Wi-Fi SDIO support.

if(NOT (
    "nxp_m2_2el_wifi_bt" IN_LIST SHIELD_AS_LIST OR
    "nxp_m2_2ll_wifi_bt" IN_LIST SHIELD_AS_LIST
))
  message(FATAL_ERROR
    "Shield 'nxp_m2_2el_2ll_ieee802154' requires one of the following base shields:\n"
    "  - nxp_m2_2el_wifi_bt (IW612 Murata 2EL)\n"
    "  - nxp_m2_2ll_wifi_bt (IW610 Murata 2LL)\n"
    "Example: --shield \"nxp_m2_2el_wifi_bt nxp_m2_2el_2ll_ieee802154\""
  )
endif()
