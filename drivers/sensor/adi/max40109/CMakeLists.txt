#
#  Copyright (c) 2026 Analog Devices Inc.
#
#  SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(max40109.c)
zephyr_library_sources_ifdef(CONFIG_MAX40109_TRIGGER max40109_trigger.c)
