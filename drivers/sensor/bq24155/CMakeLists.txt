# Copyright (c) 2021 Greyp Bikes d.o.o
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BQ24155 bq24155.c)
