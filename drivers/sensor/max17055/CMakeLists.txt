# Copyright 2020 Google LLC
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MAX17055 max17055.c)
