/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_FLASH_MCUX_XSPI_XIP
/*
 * ITCM starts at 0x00000000, while the unwind tables reside in flash at
 * 0x60000000. This exceeds the +-1 GiB range of R_ARM_PREL31 relocations.
 * The clock-switching code must remain in ITCM while XSPI is unavailable.
 * Discard its unwind metadata before the generic ARM linker script collects
 * it; stack unwinding through ITCM code is not supported in XIP builds.
 */
SECTIONS
{
	/DISCARD/ : { *(.ARM.exidx.itcm*) *(.ARM.extab.itcm*) }
}
#endif

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
