# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_OMRON_2SMPP03 omron_2smpp03.c)
