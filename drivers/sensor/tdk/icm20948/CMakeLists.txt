# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(icm20948.c ak09916.c icm20948_bus.c)
zephyr_library_sources_ifdef(CONFIG_ICM20948_TRIGGER icm20948_trigger.c)
