/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define reg_load(reg, imm)  __asm__("li " STRINGIFY(reg) ", " STRINGIFY(imm))

/**
 * Load up a bunch of known values into registers
 * and expect them to show up in the core dump.
 * Value is register ABI name kinda spelled out,
 * followed by zeros to pad to 32 bits,
 * followed by FF00, followed by hex number of the register,
 * follwed by the "hex-coded-decimal" number of the register.
 */
int main(void)
{
	reg_load(ra, 0xDADA0000FF000101);

	/* reg_load(sp, 0x0000000000000000); skipped because it can mess stuff up */

#ifndef CONFIG_RISCV_GP
	reg_load(gp, 0xE1E10000FF000303);
#endif /* CONFIG_RISCV_GP */

#ifndef CONFIG_THREAD_LOCAL_STORAGE
	reg_load(tp, 0xE2E20000FF000404);
#endif /* CONFIG_THREAD_LOCAL_STORAGE */

	reg_load(t0, 0xD0FF0000FF000505);
	reg_load(t1, 0xD1FF0000FF000606);
	reg_load(t2, 0xD2FF0000FF000707);

#ifndef CONFIG_FRAME_POINTER
	reg_load(s0, 0xC0FF0000FF000808);
#endif /* CONFIG_FRAME_POINTER */

	reg_load(s1, 0xC1FF0000FF000909);

	reg_load(a0, 0xA0FF0000FF000A10);
	reg_load(a1, 0xA1FF0000FF000B11);
	reg_load(a2, 0xA2FF0000FF000C12);
	reg_load(a3, 0xA3FF0000FF000D13);
	reg_load(a4, 0xA4FF0000FF000E14);
	reg_load(a5, 0xA5FF0000FF000F15);

#ifndef CONFIG_RISCV_ISA_RV32E
	reg_load(a6, 0xA6FF0000FF001016);
	reg_load(a7, 0xA7FF0000FF001117);

	reg_load(s2, 0xC2FF0000FF001218);
	reg_load(s3, 0xC3FF0000FF001319);
	reg_load(s4, 0xC4FF0000FF001420);
	reg_load(s5, 0xC5FF0000FF001521);
	reg_load(s6, 0xC6FF0000FF001622);
	reg_load(s7, 0xC7FF0000FF001723);
	reg_load(s8, 0xC8FF0000FF001824);
	reg_load(s9, 0xC9FF0000FF001925);
	reg_load(s10, 0xC10FF000FF001A26);
	reg_load(s11, 0xC11FF000FF001B27);

	reg_load(t3, 0xD3FF0000FF001C28);
	reg_load(t4, 0xD4FF0000FF001D29);
	reg_load(t5, 0xD5FF0000FF001E30);
	reg_load(t6, 0xD6FF0000FF001F31);
#endif /* CONFIG_RISCV_ISA_RV32E */

#ifdef CONFIG_TEST_RISCV_FATAL_PANIC
	reg_load(a0, 4); /* K_ERR_KERNEL_PANIC */
	reg_load(t0, 0); /* RV_ECALL_RUNTIME_EXCEPT */
	__asm__("ecall");
#else /* CONFIG_TEST_RISCV_FATAL_ILLEGAL_INSTRUCTION */
	__asm__(
		/**
		 * 0 is an illegal instruction,
		 * put 2 copies to make it 4 bytes wide just in case.
		 */
		".insn 2, 0\n\t"
		".insn 2, 0\n\t"
		"");
#endif /* CONFIG_TEST_RISCV_FATAL_PANIC */

	return 0;
}
