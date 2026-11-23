/*
 * Copyright (c) 2024 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/irq.h>

/* Exports */
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

SECTION_FUNC(exception.other, __soc_save_context)

	ret

SECTION_FUNC(exception.other, __soc_restore_context)

	/*
	 * For Nuclei ECLIC, the interrupt level (mintstatus.MIL) is restored
	 * from the previous interrupt level (mcause.MPIL) only if
	 * mcause.interrupt is set when executing MRET.
	 * Always set the next context's mcause.interrupt to ensure the
	 * interrupt level is restored correctly after MRET.
	 */
	addi a0, a0, -__struct_arch_esf_soc_context_OFFSET
	lw t0, __struct_arch_esf_mcause_OFFSET(a0)
	li t1, 1 << RISCV_MCAUSE_IRQ_POS
	or t0, t0, t1
	sw t0, __struct_arch_esf_mcause_OFFSET(a0)

	ret

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
