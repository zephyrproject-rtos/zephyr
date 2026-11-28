# Copyright (c) 2026 Dotcom IoT LLP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(sths34pf80.c)
zephyr_library_sources(sths34pf80_decoder.c)
zephyr_library_sources(sths34pf80_rtio.c)
zephyr_library_sources_ifdef(CONFIG_STHS34PF80_TRIGGER sths34pf80_trigger.c)

zephyr_library_include_directories(../stmemsc)
