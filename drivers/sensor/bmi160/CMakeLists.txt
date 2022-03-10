# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bmi160_common.c bmi160_spi_bus.c bmi160_i2c_bus.c)

# API sources
zephyr_library_sources_ifdef(CONFIG_SENSOR_VERSION_1 bmi160_v1.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_VERSION_2 bmi160_v2.c)

# Trigger sources
zephyr_library_sources_ifdef(CONFIG_BMI160_TRIGGER   bmi160_trigger.c)
