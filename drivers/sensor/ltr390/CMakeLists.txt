# Copyright (c) 2022 Luke Holt
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ltr390.c)
zephyr_library_sources_ifdef(CONFIG_LTR390_TRIGGER ltr390_trigger.c)
