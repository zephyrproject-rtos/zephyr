# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_LPD880X_STRIP       lpd880x.c)
zephyr_sources_ifdef(CONFIG_WS2812_STRIP_SPI    ws2812_spi.c)
zephyr_sources_ifdef(CONFIG_WS2812_STRIP_GPIO   ws2812_gpio.c)
zephyr_sources_ifdef(CONFIG_APA102_STRIP        apa102.c)
