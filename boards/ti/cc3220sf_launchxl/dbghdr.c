/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This debug header, located at the start of flash, indicates
 * to the bootloader that this is a debug image, allowing
 * debuggers and flash-loaders to access the chip over JTAG.
 * Also, on subsequent reboots, the bootloader skips the integrity
 * check, preventing the image from being mass erased.
 *
 * See section 21.10: "Debugging Flash User Application Using JTAG"
 * in the CC3220 TRM: http://www.ti.com/lit/ug/swru465/swru465.pdf
 */
#ifdef CONFIG_CC3220SF_DEBUG
__attribute__ ((section(".dbghdr")))
const unsigned long ulDebugHeader[] = {
	0x5AA5A55A,
	0x000FF800,
	0xEFA3247D
};
#endif
