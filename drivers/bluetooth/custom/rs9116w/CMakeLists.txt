# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BT_RS9116W)
    zephyr_library_sources(
        rs9116w_ble_core.c
        rs9116w_ble_gap.c
        rs9116w_ble_gatt.c
        rs9116w_ble_conn.c
    )
    zephyr_sources_ifdef(CONFIG_RSI_BLE_SHELL rs9116w_ble_shell.c)
    zephyr_sources_ifdef(CONFIG_BT_SMP rs9116w_ble_smp.c rs9116w_ble_keys.c)
endif()
