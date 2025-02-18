# Copyright (c) 2025 Dipak Shetty <shetty.dipak@gmx.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_EVERLIGHT_ALS_PT19		als_pt19.c)
