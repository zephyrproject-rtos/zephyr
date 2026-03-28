# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tsl2540.c)
zephyr_library_sources_ifdef(CONFIG_TSL2540_TRIGGER tsl2540_trigger.c)
