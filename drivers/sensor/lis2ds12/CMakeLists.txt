# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LIS2DS12            lis2ds12)
zephyr_library_sources_ifdef(CONFIG_LIS2DS12            lis2ds12_i2c.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DS12            lis2ds12_spi.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DS12_TRIGGER    lis2ds12_trigger.c)
