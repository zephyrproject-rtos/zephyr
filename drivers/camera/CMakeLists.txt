# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_MT9M114   mt9m114)

zephyr_sources(camera_dev.c)
zephyr_sources(image_sensor_dev.c)

zephyr_sources_ifdef(CONFIG_USERSPACE camera_handlers.c)
zephyr_sources_ifdef(CONFIG_USERSPACE image_sensor_handlers.c)
