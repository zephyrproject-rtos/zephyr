/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = CONFIG_MCXC_FLASH_CONFIG_OFFSET;
KEEP(*(.kinetis_flash_config))
KEEP(*(".kinetis_flash_config.*"))
