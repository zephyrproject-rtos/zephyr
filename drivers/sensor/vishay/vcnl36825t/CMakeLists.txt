# Copyright (c) 2024 Juliane Schulze, deveritec Gmbh
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(vcnl36825t.c)
zephyr_library_sources_ifdef(CONFIG_VCNL36825T_TRIGGER vcnl36825t_trigger.c)
