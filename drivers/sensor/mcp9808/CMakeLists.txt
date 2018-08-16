
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_MCP9808}         mcp9808.c
            IFDEF:${CONFIG_MCP9808_TRIGGER} mcp9808_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
