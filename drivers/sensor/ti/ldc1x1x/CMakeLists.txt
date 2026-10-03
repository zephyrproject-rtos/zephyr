#
# Copyright (c) 2026 RAKwireless Technology Limited
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(ldc1x1x.c)
zephyr_library_sources_ifdef(CONFIG_LDC1X1X_TRIGGER ldc1x1x_trigger.c)
