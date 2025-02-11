# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mcp9808.c)
zephyr_library_sources_ifdef(CONFIG_MCP9808_TRIGGER mcp9808_trigger.c)
