# Copyright (c) 2026 Muhammed Asif P
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MCHP_EVSYS_G1 mchp_evsys_g1.c)
