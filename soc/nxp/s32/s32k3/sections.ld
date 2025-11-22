/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Emit IVT section only when XIP and (standalone app OR MCUboot itself) */
#if defined(CONFIG_XIP) && (!defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_MCUBOOT))

/* IVT at flash base */
.ivt_header ORIGIN(IVT_HEADER) : ALIGN(4)
{
	/* Using a wildcard makes it more robust */
	KEEP(*(.ivt_header*));
} > IVT_HEADER

#else

#define _NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE (1 << 17)
#define _RO_Msk (7 << 24)

/* ROM region size order for MPU configuration */
_rom_region_order = ((LOG2CEIL(__rom_region_end - __rom_region_start) - 1) << 1);
_rom_attr = (_NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE | _rom_region_order | _RO_Msk);

#endif
