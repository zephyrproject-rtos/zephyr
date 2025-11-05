/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int main(void)
{
	__asm__(
		/**
		 * Load up a bunch of known values into registers
		 * and expect them to show up in the core dump.
		 * Value is register ABI name kinda spelled out,
		 * followed by zeros to pad to 32 bits,
		 * followed by FF00, followed by hex number of the register,
		 * follwed by the "hex-coded-decismal" number of the register.
		 */

		/* "RA" -> "DA".  Kind of a stretch, but okay. */
		"li x1, 0xDADA0000FF000101\n\t"

		/* Skip stack pointer because it can mess stuff up. */
		/* "li x2, 0\n\t" */

		/* T0 -> D0.  Kinda close in pronunciation. */
		"li x5, 0xD0FF0000FF000505\n\t"
		"li x6, 0xD1FF0000FF000606\n\t"
		"li x7, 0xD2FF0000FF000707\n\t"
		/* S0 -> C0.  Kinda close in pronunciation. */
		"li x8, 0xC0FF0000FF000808\n\t"
		"li x9, 0xC1FF0000FF000909\n\t"
		/* A0 -> A0.  Actual match! */
		"li x10, 0xA0FF0000FF000A10\n\t"
		"li x11, 0xA1FF0000FF000B11\n\t"
		"li x12, 0xA2FF0000FF000C12\n\t"
		"li x13, 0xA3FF0000FF000D13\n\t"
		"li x14, 0xA4FF0000FF000E14\n\t"
		"li x15, 0xA5FF0000FF000F15\n\t"
		"li x16, 0xA6FF0000FF001016\n\t"
		"li x17, 0xA7FF0000FF001117\n\t"
		"li x18, 0xC2FF0000FF001218\n\t"
		"li x19, 0xC3FF0000FF001319\n\t"
		"li x20, 0xC4FF0000FF001420\n\t"
		"li x21, 0xC5FF0000FF001521\n\t"
		"li x22, 0xC6FF0000FF001622\n\t"
		"li x23, 0xC7FF0000FF001723\n\t"
		"li x24, 0xC8FF0000FF001824\n\t"
		"li x25, 0xC9FF0000FF001925\n\t"
		"li x26, 0xC10FF000FF001A26\n\t"
		"li x27, 0xC11FF000FF001B27\n\t"
		"li x28, 0xD3FF0000FF001C28\n\t"
		"li x29, 0xD4FF0000FF001D29\n\t"
		"li x30, 0xD5FF0000FF001E30\n\t"
		"li x31, 0xD6FF0000FF001F31\n\t"
		/* K_ERR_KERNEL_PANIC */
		"li a0, 4\n\t"
		/* RV_ECALL_RUNTIME_EXCEPT */
		"li t0, 0\n\t"
		"ecall\n");

	return 0;
}
