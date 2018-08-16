
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LPS22HB} lps22hb.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
