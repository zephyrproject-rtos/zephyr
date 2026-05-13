# Copyright (c) 2021 Teslabs Engineering S.L.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_SOC_FAMILY_GD_GD32_HAS_BACKUP_DOMAIN_PROTECTION gd32_backup_domain.c)
