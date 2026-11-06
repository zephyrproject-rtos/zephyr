# Copyright (c) 2026 RAKwireless Technology Limited
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(rak12035.c)
zephyr_library_sources_ifdef(CONFIG_EMUL rak12035_emul.c)
