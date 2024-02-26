# Copyright (c) 2024 Gustavo Silva
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(ens160.c)
zephyr_library_sources(ens160_i2c.c)
zephyr_library_sources(ens160_spi.c)
zephyr_library_sources_ifdef(CONFIG_ENS160_TRIGGER ens160_trigger.c)
