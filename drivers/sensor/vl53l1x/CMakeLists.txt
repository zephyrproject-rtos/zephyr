# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
	vl53l1x.c
	vl53l1_platform.c
)

zephyr_library_sources_ifdef(CONFIG_VL53L1X_TRIGGER vl53l1x_trigger.c)
