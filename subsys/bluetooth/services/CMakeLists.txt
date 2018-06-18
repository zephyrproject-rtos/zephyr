zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BT_GATT_DIS} dis.c
)

target_sources(subsys_bluetooth PRIVATE ${PRIVATE_SOURCES})
