# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tsl2591.c)
zephyr_library_sources_ifdef(CONFIG_TSL2591_TRIGGER tsl2591_trigger.c)
