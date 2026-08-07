# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ccs811.c)
zephyr_library_sources_ifdef(CONFIG_CCS811_TRIGGER ccs811_trigger.c)
