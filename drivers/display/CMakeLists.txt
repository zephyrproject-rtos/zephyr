zephyr_sources_ifdef(CONFIG_MICROBIT_DISPLAY
	mb_display.c
	mb_font.c
)
zephyr_sources_ifdef(CONFIG_ILI9340 display_ili9340.c)
zephyr_sources_ifdef(CONFIG_ILI9340_LCD_ADAFRUIT_1480
	display_ili9340_adafruit_1480.c
)
