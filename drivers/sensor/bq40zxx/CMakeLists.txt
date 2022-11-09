# Copyright (c) 2020 ZedBlox
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BQ40ZXX bq40zxx.c)
