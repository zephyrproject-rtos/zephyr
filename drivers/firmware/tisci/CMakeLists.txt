# Copyright (c) 2025, Texas Instruments
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_TISCI tisci.c)
