# Copyright (c) 2026 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tsl2522.c)
zephyr_library_sources_ifdef(CONFIG_TSL2522_TRIGGER tsl2522_trigger.c)
