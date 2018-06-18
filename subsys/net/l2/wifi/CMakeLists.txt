zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_NET_L2_WIFI_MGMT}  wifi_mgmt.c
            IFDEF:${CONFIG_NET_L2_WIFI_SHELL} wifi_shell.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
