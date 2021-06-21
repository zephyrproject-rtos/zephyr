# Copyright 2020 Matija Tudan
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MAX1726X max1726x.c)
