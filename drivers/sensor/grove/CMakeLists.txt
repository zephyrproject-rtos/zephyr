# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_GROVE_LIGHT_SENSOR		light_sensor.c)
zephyr_sources_ifdef(CONFIG_GROVE_TEMPERATURE_SENSOR	temperature_sensor.c)
