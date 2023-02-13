/*
 * Copyright (c) 2019 NXP
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = CONFIG_KINETIS_FLASH_CONFIG_OFFSET;
KEEP(*(.kinetis_flash_config))
KEEP(*(".kinetis_flash_config.*"))
