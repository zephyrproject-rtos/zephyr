# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MCUX_LPCMP mcux_lpcmp.c)
