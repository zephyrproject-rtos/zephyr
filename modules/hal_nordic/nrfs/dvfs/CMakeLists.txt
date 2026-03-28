# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NRFS_DVFS_LOCAL_DOMAIN ld_dvfs.c
                                                           ld_dvfs_handler.c)
