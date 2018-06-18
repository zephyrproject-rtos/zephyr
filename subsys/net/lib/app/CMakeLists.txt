zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_NET_APP_SERVER} net_app.c server.c
            IFDEF:${CONFIG_NET_APP_CLIENT} net_app.c client.c
)


target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
