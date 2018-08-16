zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LPS25HB} lps25hb.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
