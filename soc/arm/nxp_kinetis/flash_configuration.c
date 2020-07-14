/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <linker/sections.h>

uint8_t __kinetis_flash_config_section __kinetis_flash_config[] = {
	/* Backdoor Comparison Key (unused) */
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	/* Program flash protection; 1 bit/region - 0=protected, 1=unprotected
	 */
	0xFF, 0xFF, 0xFF, 0xFF,

	/* Flash security register (FSEC) enables/disables backdoor key access,
	 * mass erase, factory access, and flash security
	 */
	CONFIG_KINETIS_FLASH_CONFIG_FSEC,

	/* Flash nonvolatile option register (FOPT) enables/disables NMI,
	 * EzPort, and boot options
	 */
	CONFIG_KINETIS_FLASH_CONFIG_FOPT,

	/* EEPROM protection register (FEPROT) for FlexNVM devices */
	CONFIG_KINETIS_FLASH_CONFIG_FEPROT,

	/* Data flash protection register (FDPROT) for FlexNVM devices */
	CONFIG_KINETIS_FLASH_CONFIG_FDPROT,
};
