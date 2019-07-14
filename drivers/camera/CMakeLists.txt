# SPDX-License-Identifier: Apache-2.0

zephyr_sources(image_sensor_dev.c)

zephyr_sources_ifdef(CONFIG_USERSPACE image_sensor_handlers.c)
