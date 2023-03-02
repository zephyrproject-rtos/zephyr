# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mcp98xx.c)
zephyr_library_sources_ifdef(CONFIG_MCP98XX_TRIGGER mcp98xx_trigger.c)
