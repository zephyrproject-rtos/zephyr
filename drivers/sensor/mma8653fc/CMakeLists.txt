#
# Copyright (c) 2018 Lars Knudsen
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MMA8653 mma8653fc.c)
