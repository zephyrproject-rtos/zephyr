zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc.c
            soc_config.c
)

target_sources(arch_arm PRIVATE ${PRIVATE_SOURCES})
