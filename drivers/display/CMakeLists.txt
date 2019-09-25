# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_DISPLAY_MCUX_ELCDIF	display_mcux_elcdif.c)
zephyr_sources_ifdef(CONFIG_GROVE_LCD_RGB	grove_lcd_rgb.c)
zephyr_sources_ifdef(CONFIG_SSD1306		ssd1306.c)
zephyr_sources_ifdef(CONFIG_SSD16XX		ssd16xx.c)
zephyr_sources_ifdef(CONFIG_SDL_DISPLAY		display_sdl.c)
zephyr_sources_ifdef(CONFIG_DUMMY_DISPLAY	display_dummy.c)
zephyr_sources_ifdef(CONFIG_FRAMEBUF_DISPLAY	display_framebuf.c)
zephyr_sources_ifdef(CONFIG_ILI9340		display_ili9340.c)
zephyr_sources_ifdef(CONFIG_ST7789V		display_st7789v.c)

zephyr_sources_ifdef(CONFIG_MICROBIT_DISPLAY
	mb_display.c
	mb_font.c
)
zephyr_sources_ifdef(CONFIG_ILI9340_LCD_ADAFRUIT_1480
	display_ili9340_adafruit_1480.c
)

zephyr_sources_ifdef(CONFIG_ILI9340_LCD_SEEED_TFTV2
	display_ili9340_seeed_tftv2.c
)

zephyr_sources_ifdef(CONFIG_ST7789V_LCD_TL019FQV01
	display_st7789v_tl019fqv01.c
)
