# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

dt_chosen(NBU_PATH PROPERTY "zephyr,nbu")
if(DEFINED NBU_PATH)
    dt_prop(MAX_TX_POWER PATH "${NBU_PATH}" PROPERTY "max-tx-power-dbm")
    if(DEFINED MAX_TX_POWER)
        #gAppMaxTxPowerDbm_c macro is used by hal code
        zephyr_compile_definitions(gAppMaxTxPowerDbm_c=${MAX_TX_POWER})
    endif()
endif()
