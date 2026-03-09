# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mcp_common.c)
zephyr_library_sources(mcp_server.c)
zephyr_library_sources(mcp_json.c)
zephyr_library_sources_ifdef(CONFIG_MCP_TRANSPORT_HTTP mcp_server_http.c)
zephyr_linker_sources_ifdef(CONFIG_MCP_TRANSPORT_HTTP SECTIONS mcp_http.ld)
zephyr_library_sources_ifdef(CONFIG_MCP_TRANSPORT_MOCK mcp_transport_mock.c)
