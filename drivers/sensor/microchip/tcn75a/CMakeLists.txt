# Copyright 2023 Daniel DeGrasse <daniel@degrasse.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(tcn75a.c)
zephyr_library_sources_ifdef(CONFIG_TCN75A_TRIGGER tcn75a_trigger.c)
