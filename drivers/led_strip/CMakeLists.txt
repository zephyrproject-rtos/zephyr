# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_APA102_STRIP        apa102.c)
zephyr_library_sources_ifdef(CONFIG_LPD880X_STRIP       lpd880x.c)
zephyr_library_sources_ifdef(CONFIG_WS2812_STRIP_GPIO   ws2812_gpio.c)
zephyr_library_sources_ifdef(CONFIG_WS2812_STRIP_SPI    ws2812_spi.c)
zephyr_library_sources_ifdef(CONFIG_WS2812_STRIP_I2S    ws2812_i2s.c)
zephyr_library_sources_ifdef(CONFIG_TLC5971_STRIP       tlc5971.c)
