# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ACS71020 acs71020.c)
zephyr_library_sources_ifdef(CONFIG_ACS71020_TRIGGER acs71020_trigger.c)
