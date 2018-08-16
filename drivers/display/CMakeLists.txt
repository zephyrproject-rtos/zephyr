zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_GROVE_LCD_RGB}             grove_lcd_rgb.c
            IFDEF:${CONFIG_MICROBIT_DISPLAY}          mb_display.c
                                                      mb_font.c
            IFDEF:${CONFIG_ILI9340}                   display_ili9340.c
            IFDEF:${CONFIG_ILI9340_LCD_ADAFRUIT_1480} display_ili9340_adafruit_1480.c
            IFDEF:${CONFIG_SSD1306}                   ssd1306.c
            IFDEF:${CONFIG_SSD1673}                   ssd1673.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
