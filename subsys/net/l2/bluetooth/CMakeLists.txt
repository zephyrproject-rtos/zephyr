zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_NET_L2_BT}       bluetooth.c
            IFDEF:${CONFIG_NET_L2_BT_SHELL} bluetooth_shell.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
