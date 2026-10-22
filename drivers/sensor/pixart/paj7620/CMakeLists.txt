# Copyright (c) 2025 Paul Timke <ptimkec@live.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(paj7620.c)
zephyr_library_sources_ifdef(CONFIG_PAJ7620_TRIGGER paj7620_trigger.c)
