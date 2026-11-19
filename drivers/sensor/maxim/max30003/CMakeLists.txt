# Copyright (c) 2026 Nainaiu Rakhaine
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(max30003.c)
zephyr_library_sources_ifdef(CONFIG_MAX30003_TRIGGER max30003_trigger.c)
