# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ISL29125 isl29125.c)
zephyr_library_sources_ifdef(CONFIG_ISL29125_TRIGGER isl29125_trigger.c)
