/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/semihost.h>
#include <zephyr/sys/util.h>
#include <cmsis_core.h>

long semihost_exec(enum semihost_instr instr, void *args)
{
	register unsigned int r0 __asm__("r0") = instr;
	register void *r1 __asm__("r1") = args;
	register int ret __asm__("r0");

	__asm__ volatile("bkpt 0xab" : "=r"(ret) : "r"(r0), "r"(r1) : "memory");
	return ret;
}

bool semihost_debugger_attached(void)
{
	/* QEMU services semihosting itself and does not model DHCSR.C_DEBUGEN */
	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		return true;
	}

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) || defined(CONFIG_ARMV8_M_BASELINE)
	return (DCB->DHCSR & DCB_DHCSR_C_DEBUGEN_Msk) != 0U;
#else
	/* Debug registers are not accessible to software on ARMv6-M */
	return false;
#endif
}
