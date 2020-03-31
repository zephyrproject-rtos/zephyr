# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BMI160 bm160.c)
zephyr_library_sources_ifdef(CONFIG_BMI160 bmi160_spi.c)

zephyr_library_sources_ifdef(CONFIG_BMX160 bm160.c)
zephyr_library_sources_ifdef(CONFIG_BMX160 bmx160_i2c.c)

zephyr_library_sources_ifdef(CONFIG_BMI160_TRIGGER bm160_trigger.c)
