# Copyright (c) 2019 Thomas Schmid <tom@lfence.de>
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(ms5607.c)
zephyr_library_sources(ms5607_i2c.c)
zephyr_library_sources_ifdef(CONFIG_MS5607            ms5607_spi.c)
