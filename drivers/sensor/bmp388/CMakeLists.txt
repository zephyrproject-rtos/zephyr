# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BMP388 bmp388.c)
zephyr_library_sources_ifdef(CONFIG_BMP388 bmp388_i2c.c)
