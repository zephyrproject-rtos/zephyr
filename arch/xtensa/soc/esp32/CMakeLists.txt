zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc.c
            esp32-mp.c
)

target_sources(arch_xtensa PRIVATE ${PRIVATE_SOURCES})
