zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_INTEL_GNA} intel_gna.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

