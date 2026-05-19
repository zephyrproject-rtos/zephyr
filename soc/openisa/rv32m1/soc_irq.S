/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>

#include <soc.h>

#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY
#include <csr_irq.inc>
#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

/* Exports */
GTEXT(__soc_handle_irq)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif

/*
 * With a0 == irq_num, this is equivalent to:
 *
 * EVENT_UNIT->INTPTPENDCLEAR = (1U << irq_num);
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	la t0, __EVENT_INTPTPENDCLEAR
	li t1, 1
	sll t1, t1, a0
	sw t1, 0x00(t0)
	ret

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
/*
 * The RI5CY core has ISA extensions for faster loop performance
 * that use extra registers.
 *
 * If the toolchain generates instructions that use them, they must be saved
 * prior to handling an interrupt/exception. This case is handled using
 * Zephyr's generic RISC-V mechanism for soc-specific context.
 *
 * For details, see the Kconfig help for CONFIG_RISCV_SOC_CONTEXT_SAVE.
 */
SECTION_FUNC(exception.other, __soc_save_context)
#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY
	__custom_csr_save_context a0, t0
#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

	ret

SECTION_FUNC(exception.other, __soc_restore_context)
#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY
	__custom_csr_restore_context a0, t0
#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

	ret
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
