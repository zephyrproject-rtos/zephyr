# Copyright (c) 2024 Vitrolife A/S
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TSIC_XX6 tsic_xx6.c)
