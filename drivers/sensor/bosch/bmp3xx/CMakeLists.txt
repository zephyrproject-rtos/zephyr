#
# Copyright (c) 2020 Facebook, Inc. and its affiliates
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(bmp3xx.c bmp3xx_i2c.c bmp3xx_spi.c)
zephyr_library_sources_ifdef(CONFIG_BMP3XX_TRIGGER bmp3xx_trigger.c)
