/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

/* Emit option setting sections only when standalone app OR MCUboot itself */
#if !defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_MCUBOOT)

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_osis))
SECTION_DATA_PROLOGUE(.option_setting_osis, DT_REG_ADDR(DT_NODELABEL(option_setting_osis)),)
{
	KEEP(*(.option_setting_osis))
} GROUP_LINK_IN(OFS_OSIS_MEMORY)
#endif

#endif
