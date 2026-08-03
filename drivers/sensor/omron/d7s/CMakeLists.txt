# Copyright (c) 2026 RAKwireless Technology Limited
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(d7s.c)
zephyr_library_sources_ifdef(CONFIG_D7S_TRIGGER d7s_trigger.c)
zephyr_library_sources_ifdef(CONFIG_EMUL d7s_emul.c)
