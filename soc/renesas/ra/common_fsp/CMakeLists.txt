# Copyright (c) 2025-2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_RENESAS_RA_BATTERY_BACKUP_MANUAL_CONFIGURE
  battery_backup.c
  cold_start.c
)
