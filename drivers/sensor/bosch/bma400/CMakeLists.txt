# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    bma400.c
    bma400_decoder.c
    bma400_rtio.c
)

zephyr_library_sources_ifdef(CONFIG_SPI bma400_spi.c)
zephyr_library_sources_ifdef(CONFIG_BMA400_STREAM bma400_rtio_stream.c)
zephyr_library_sources_ifdef(CONFIG_BMA400_STREAM bma400_interrupt.c)
