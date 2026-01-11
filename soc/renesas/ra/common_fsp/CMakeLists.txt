# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_RENESAS_RA_BATTERY_BACKUP_MANUAL_CONFIGURE battery_backup.c)
zephyr_sources(cold_start.c)
