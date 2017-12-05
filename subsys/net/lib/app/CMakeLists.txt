zephyr_library()
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources_ifdef(CONFIG_NET_APP init.c)
zephyr_library_sources_ifdef(CONFIG_NET_APP_SERVER net_app.c server.c)
zephyr_library_sources_ifdef(CONFIG_NET_APP_CLIENT net_app.c client.c)

if(CONFIG_NET_APP_SETTINGS)
  zephyr_library_sources_ifdef(
    CONFIG_NET_L2_IEEE802154
    ieee802154_settings.c
    )

  zephyr_library_sources_ifdef(CONFIG_NET_L2_BT bt_settings.c)
endif()

zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
