zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LP3943}    lp3943.c
            IFDEF:${CONFIG_LP5562}    lp5562.c
            IFDEF:${CONFIG_PCA9633}   pca9633.c
            IFDEF:${CONFIG_USERSPACE} led_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
