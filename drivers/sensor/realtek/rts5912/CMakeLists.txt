# Copyright (c) 2025, Realtek, SIBG-SD7
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_TACH_RTS5912 rts5912.c)
