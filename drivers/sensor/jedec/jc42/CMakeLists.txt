# Copyright (c) 2024 Vogl Electronic GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(jc42.c)
zephyr_library_sources_ifdef(CONFIG_JC42_TRIGGER jc42_trigger.c)
