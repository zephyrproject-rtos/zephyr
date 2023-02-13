# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_HTTP_PARSER_STRICT)
zephyr_library_compile_definitions(HTTP_PARSER_STRICT)
endif()

zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources_ifdef(CONFIG_HTTP_PARSER http_parser.c)
zephyr_library_sources_ifdef(CONFIG_HTTP_PARSER_URL http_parser_url.c)
zephyr_library_sources_ifdef(CONFIG_HTTP_CLIENT http_client.c)
