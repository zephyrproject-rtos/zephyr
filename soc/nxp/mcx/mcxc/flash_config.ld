/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/devicetree.h>


. = DT_PROP(DT_NODELABEL(ftfa), config_field_offset);
KEEP(*(.kinetis_flash_config))
KEEP(*(".kinetis_flash_config.*"))
