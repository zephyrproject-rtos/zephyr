zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            pinmux.c
            clock.c
)

target_sources(board PRIVATE ${PRIVATE_SOURCES})
target_include_directories(board PRIVATE ${ZEPHYR_BASE}/drivers)
