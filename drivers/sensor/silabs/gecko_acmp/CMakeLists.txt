# Copyright 2025 Kickmaker
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_GECKO_ACMP gecko_acmp.c)
