# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021-2025 EPAM Systems

zephyr_sources(hvm.c)
zephyr_sources_ifdef(CONFIG_XEN_EVENTS events.c)
zephyr_sources_ifdef(CONFIG_XEN_GRANT_TABLE gnttab.c)
zephyr_sources(memory.c)
zephyr_sources_ifdef(CONFIG_XEN_DMOP dmop.c)
zephyr_sources(version.c)

add_subdirectory_ifdef(CONFIG_XEN_DOM0 dom0)
