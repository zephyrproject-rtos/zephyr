zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LPD880X_STRIP} lpd880x.c
            IFDEF:${CONFIG_WS2812_STRIP}  ws2812.c
            IFDEF:${CONFIG_WS2812B_SW}    ws2812b_sw.c
            IFDEF:${CONFIG_APA102_STRIP}  apa102.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
