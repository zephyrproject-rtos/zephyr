# Copyright (c) 2026 Cirrus Logic, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L26 cs40l26.c cs40l26-firmware.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L26_I2C cs40l26-i2c.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L26_SPI cs40l26-spi.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L5X cs40l5x.c cs40l5x-i2c.c cs40l5x-spi.c)
