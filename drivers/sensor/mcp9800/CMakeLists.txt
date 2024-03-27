# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mcp9800.c)
zephyr_library_sources_ifdef(CONFIG_MCP9800_TRIGGER mcp9800_trigger.c)
