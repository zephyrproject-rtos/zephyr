# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LIS2DH lis2dh.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DH_TRIGGER lis2dh_trigger.c)
