# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ICM20600 icm20600.c)
zephyr_library_sources_ifdef(CONFIG_ICM20600_TRIGGER icm20600_trigger.c)
