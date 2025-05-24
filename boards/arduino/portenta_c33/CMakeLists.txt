# Copyright 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NETWORKING board.c)
