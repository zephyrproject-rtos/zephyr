# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(sht3xd.c)
zephyr_library_sources_ifdef(CONFIG_SHT3XD_TRIGGER sht3xd_trigger.c)
