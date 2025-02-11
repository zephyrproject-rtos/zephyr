# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(adxl362.c)
zephyr_library_sources_ifdef(CONFIG_ADXL362_TRIGGER adxl362_trigger.c)
