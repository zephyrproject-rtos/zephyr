# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(itds.c)
zephyr_library_sources_ifdef(CONFIG_ITDS_TRIGGER itds_trigger.c)
