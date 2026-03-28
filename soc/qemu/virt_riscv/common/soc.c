/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief QEMU RISC-V virt machine hardware depended interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/poweroff.h>

#define SIFIVE_SYSCON_TEST 0x00100000

/*
 * Exit QEMU and tell error number.
 *   Higher 16bits: indicates error number.
 *   Lower 16bits : set FINISHER_FAIL
 */
#define FINISHER_FAIL		0x3333

/* Exit QEMU successfully */
#define FINISHER_EXIT		0x5555

/* Reboot machine */
#define FINISHER_REBOOT		0x7777

void sys_arch_reboot(int type)
{
	volatile uint32_t *reg = (uint32_t *)SIFIVE_SYSCON_TEST;

	*reg = FINISHER_REBOOT;

	ARG_UNUSED(type);
}

#ifdef CONFIG_RISCV_S_MODE
#include <zephyr/arch/riscv/sbi.h>

/*
 * In S-mode the sifive_test device is still memory-mapped and accessible,
 * but using the standard SBI System Reset extension is the portable path.
 * Our in-tree M-mode SBI runtime (sbi.S) handles SBI_EXT_SRST by writing
 * FINISHER_EXIT to the QEMU test finisher and halting.
 */
void z_sys_poweroff(void)
{
	register unsigned long a0 __asm__("a0") = SBI_SRST_RESET_TYPE_SHUTDOWN;
	register unsigned long a1 __asm__("a1") = SBI_SRST_RESET_REASON_NONE;
	register unsigned long a6 __asm__("a6") = SBI_FUNC_SYSTEM_RESET;
	register unsigned long a7 __asm__("a7") = SBI_EXT_SRST;

	__asm__ volatile("ecall"
			 : "+r"(a0), "+r"(a1)
			 : "r"(a6), "r"(a7)
			 : "memory");
	CODE_UNREACHABLE;
}
#else
void z_sys_poweroff(void)
{
	volatile uint32_t *reg = (uint32_t *)SIFIVE_SYSCON_TEST;

	*reg = FINISHER_EXIT;
	CODE_UNREACHABLE;
}
#endif /* CONFIG_RISCV_S_MODE */
