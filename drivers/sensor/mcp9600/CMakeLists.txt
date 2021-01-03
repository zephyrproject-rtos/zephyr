# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MCP9600 mcp9600.c)
zephyr_library_sources_ifdef(CONFIG_MCP9600_TRIGGER mcp9600_trigger.c)
