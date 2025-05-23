# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    bma4xx.c
    bma4xx_common.c
    bma4xx_decoder.c
    bma4xx_rtio.c
)

zephyr_library_sources_ifdef(CONFIG_I2C bma4xx_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SPI bma4xx_spi.c)
zephyr_library_sources_ifdef(CONFIG_BMA4XX_STREAM bma4xx_rtio_stream.c)
zephyr_library_sources_ifdef(CONFIG_BMA4XX_STREAM bma4xx_interrupt.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_BMA4XX bma4xx_emul.c)
