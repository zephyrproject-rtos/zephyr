#
# Copyright (c) 2020 Facebook, Inc. and its affiliates
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(bmp388.c bmp388_i2c.c bmp388_spi.c)
zephyr_library_sources_ifdef(CONFIG_BMP388_TRIGGER bmp388_trigger.c)
