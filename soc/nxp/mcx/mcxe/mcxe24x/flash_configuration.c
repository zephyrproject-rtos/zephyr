/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/device.h>

uint8_t __kinetis_flash_config_section __kinetis_flash_config[] = {
	/* Backdoor Comparison Key */
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,

	/* Program flash protection; 1 bit/region - 0=protected, 1=unprotected
	 */
	0xFF,
	0xFF,
	0xFF,
	0xFF,

	/* Flash security register (FSEC) enables/disables backdoor key access,
	 * mass erase, factory access, and flash security
	 */
	DT_PROP_OR(DT_NODELABEL(ftfc), fsec, 0xFE),

	/* Flash nonvolatile option register (FOPT) enables/disables NMI,
	 * EzPort, and boot options
	 */
	DT_PROP_OR(DT_NODELABEL(ftfc), fopt, 0xFF),

	/* EEPROM protection register (FEPROT) */
	0xFF,

	/* Data flash protection register (FDPROT) */
	0XFF,
};
