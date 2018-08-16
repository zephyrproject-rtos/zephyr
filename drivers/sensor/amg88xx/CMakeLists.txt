
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_AMG88XX}         amg88xx.c
            IFDEF:${CONFIG_AMG88XX_TRIGGER} amg88xx_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
