# Copyright (c) 2026 Cirrus Logic, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L26 cs40l26.c cs40l26-firmware.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L5X cs40l5x.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40LXX cs40lxx.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40LXX_I2C cs40lxx-i2c.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40LXX_SPI cs40lxx-spi.c)
