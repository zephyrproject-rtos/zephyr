/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/riscv/csr.h>

#include <counters.h>

#define MISA_S BIT('S' - 'A')

void z_riscv_counteren_init(void)
{
	unsigned long bits = COUNTEREN_CY | COUNTEREN_TM | COUNTEREN_IR;

#ifdef CONFIG_RISCV_S_MODE
	/*
	 * mcounteren is the SBI's. The built-in one sets it in reset.S.
	 * scounteren exists because we are running in S-mode.
	 */
	csr_write(scounteren, bits);
#else
	csr_write(mcounteren, bits);

	/*
	 * A U-mode read is gated by scounteren too whenever the hart
	 * implements S-mode, even though Zephyr does not use it.
	 */
	if ((csr_read(misa) & MISA_S) != 0) {
		csr_write(scounteren, bits);
	}
#endif
}
