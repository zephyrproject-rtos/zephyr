# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_DISPLAY_MCUX_ELCDIF	display_mcux_elcdif.c)
zephyr_sources_ifdef(CONFIG_GROVE_LCD_RGB	grove_lcd_rgb.c)
zephyr_sources_ifdef(CONFIG_SSD1306		ssd1306.c)
zephyr_sources_ifdef(CONFIG_SSD16XX		ssd16xx.c)
zephyr_sources_ifdef(CONFIG_SDL_DISPLAY		display_sdl.c)
zephyr_sources_ifdef(CONFIG_DUMMY_DISPLAY	display_dummy.c)
zephyr_sources_ifdef(CONFIG_FRAMEBUF_DISPLAY	display_framebuf.c)
zephyr_sources_ifdef(CONFIG_ST7789V		display_st7789v.c)
zephyr_sources_ifdef(CONFIG_GD7965		gd7965.c)

if (CONFIG_ILI9XXX)
  zephyr_sources(display_ili9xxx.c)
endif()
zephyr_sources_ifdef(CONFIG_ILI9340 display_ili9340.c)
zephyr_sources_ifdef(CONFIG_ILI9488 display_ili9488.c)

zephyr_sources_ifdef(CONFIG_MICROBIT_DISPLAY
	mb_display.c
	mb_font.c
)
