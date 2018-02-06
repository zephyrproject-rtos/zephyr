zephyr_library()

if(CONFIG_HTTP_PARSER_STRICT)
zephyr_library_compile_definitions(HTTP_PARSER_STRICT)
endif()

zephyr_library_sources_if_kconfig(http_parser.c)
zephyr_library_sources_if_kconfig(http_parser_url.c)

if(CONFIG_HTTP_APP)
  zephyr_library_sources(http_app.c)

  zephyr_library_sources_ifdef(CONFIG_HTTP_SERVER http_app_server.c)
  zephyr_library_sources_ifdef(CONFIG_HTTP_CLIENT http_app_client.c)
else()
  zephyr_library_sources_ifdef(CONFIG_HTTP_SERVER http_server.c)
  zephyr_library_sources_ifdef(CONFIG_HTTP_CLIENT http_client.c)
endif()

zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
