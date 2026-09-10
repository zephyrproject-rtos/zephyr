# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_PM power.c)
