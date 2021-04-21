# Copyright (c) 2020 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BQ274XX bq274xx.c)
