zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_NET_CONFIG_SETTINGS} init.c
)

if(CONFIG_NET_CONFIG_SETTINGS)
  zephyr_list(SOURCES APPEND
              OUTPUT PRIVATE_SOURCES
              IFDEF:${CONFIG_NET_L2_IEEE802154} ieee802154_settings.c
              IFDEF:${CONFIG_NET_L2_BT}         bt_settings.c
    )
endif()

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
