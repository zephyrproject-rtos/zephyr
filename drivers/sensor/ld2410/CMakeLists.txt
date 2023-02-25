# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(hilink_ld2410.c)
zephyr_library_sources_ifdef(CONFIG_LD2410_TRIGGER  hilink_ld2410_trigger.c)
