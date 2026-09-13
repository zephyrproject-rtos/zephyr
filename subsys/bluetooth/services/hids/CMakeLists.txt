# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_BT_HIDS_SHELL shell)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BT_HIDS hids.c)
