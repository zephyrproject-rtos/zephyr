# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_HTTP_PARSER_STRICT)
zephyr_library_compile_definitions(HTTP_PARSER_STRICT)
endif()

zephyr_library_sources_if_kconfig(http_parser.c)
zephyr_library_sources_if_kconfig(http_parser_url.c)
