/*
 * Copyright (c) 2019 Foundries.io Ltd
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Respect for CONFIG_ROM_START_OFFSET is mandatory
 * for MCUboot support, so .reset.* and .exception.*
 * must come after that offset from ROM_BASE.
 */

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	/*
	 * For CONFIG_BOOTLOADER_MCUBOOT, the vector table is located at the
	 * end of the image header of the MCUboot. After the target image is
	 * boot, the register Machine Trap-Vector Base Address (MTVEC) is
	 * set with the value of _vector_start in the reset handler.
	 */
	_vector_start = .;
	KEEP(*(.vectors.*))
	_vector_end = .;
	. = ALIGN(4);
#endif

KEEP(*(.reset.*))
KEEP(*(".exception.entry.*")) /* contains _isr_wrapper */
*(".exception.other.*")

KEEP(*(.openocd_debug))
KEEP(*(".openocd_debug.*"))
