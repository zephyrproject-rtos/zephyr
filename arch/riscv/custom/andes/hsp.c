/*
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/irq.h>
#include <andes_csr.h>

/* HSP feature configuration in MMSC_CFG */
#define MMSC_CFG_HSP                 (1UL << 5)

/* Machine mode MHSP_CTL */
#define MHSP_CTL_OVF_EN              (1UL << 0)
#define MHSP_CTL_UDF_EN              (1UL << 1)
#define MHSP_CTL_SCHM_MASK           (3UL << 2)
#define MHSP_CTL_SCHM_TOS            (1UL << 2)
#define MHSP_CTL_SCHM_DETECT         (0   << 2)
#define MHSP_CTL_U_EN                (1UL << 3)
#define MHSP_CTL_S_EN                (1UL << 4)
#define MHSP_CTL_M_EN                (1UL << 5)

/* Machine Trap Cause exception code  */
#define TRAP_M_STACKOVF              32
#define TRAP_M_STACKUDF              33

/*
 * Initial built-in hardware stack guild
 *
 */
void z_riscv_builtin_stack_guard_init(void)
{
	if (csr_read(NDS_MMSC_CFG) & MMSC_CFG_HSP) {
		/*
		 * Set operating scheme to stack overflow/underflow detection
		 * Enable the stack protection mechanism in machine mode.
		 */
		unsigned long hsp_ctl = (MHSP_CTL_M_EN | MHSP_CTL_SCHM_DETECT);

		csr_write(NDS_MHSP_CTL, hsp_ctl);
	}
}

/*
 * Enable built-in hardware stack guard
 *
 */
void z_riscv_builtin_stack_guard_enable(void)
{
	csr_set(NDS_MHSP_CTL, (MHSP_CTL_UDF_EN | MHSP_CTL_OVF_EN));
}

/*
 * Disable built-in hardware stack guard
 *
 */
void z_riscv_builtin_stack_guard_disable(void)
{
	csr_clear(NDS_MHSP_CTL, (MHSP_CTL_UDF_EN | MHSP_CTL_OVF_EN));
}

/*
 * Configure built-in hardware stack guard
 * start: The start address of the stack buffer
 * size: The size of the stack buffer
 */
void z_riscv_builtin_stack_guard_config(unsigned long start, unsigned long size)
{
	unsigned long bottom = start + size;

	csr_write(NDS_MSP_BOUND, start);
	csr_write(NDS_MSP_BASE, bottom);
}

/*
 * Check if stack is overflow or underflow
 * return (true: stack overflow or underflow, false: normal)
 */
bool z_riscv_builtin_stack_guard_is_fault(struct arch_esf *esf)
{
	unsigned long mcause = csr_read(mcause);

	if (!(mcause & RISCV_MCAUSE_IRQ_BIT)) {
		mcause &= CONFIG_RISCV_MCAUSE_EXCEPTION_MASK;
		if ((mcause == TRAP_M_STACKOVF) || (mcause == TRAP_M_STACKUDF)) {
			return true;
		}
	}

	return false;
}
