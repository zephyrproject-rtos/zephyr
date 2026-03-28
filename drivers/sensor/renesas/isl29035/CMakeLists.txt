# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(isl29035.c)
zephyr_library_sources_ifdef(CONFIG_ISL29035_TRIGGER isl29035_trigger.c)
