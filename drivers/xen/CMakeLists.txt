# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021-2022 EPAM Systems

zephyr_sources(hvm.c)
zephyr_sources(events.c)
zephyr_sources_ifdef(CONFIG_XEN_GRANT_TABLE gnttab.c)
