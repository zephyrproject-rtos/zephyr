# CMakeLists - MAX30208 digital temperature sensor
#
# Copyright (c) 2021, Silvano Cortesi
# Copyright (c) 2021, ETH Zürich
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(max30208.c)
zephyr_library_sources_ifdef(CONFIG_MAX30208_TRIGGER max30208_trigger.c)
