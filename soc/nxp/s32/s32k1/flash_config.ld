/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = CONFIG_NXP_S32_FLASH_CONFIG_OFFSET;
KEEP(*(.kinetis_flash_config))
KEEP(*(".kinetis_flash_config.*"))
