/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets.h>
#include <toolchain.h>

#include <soc.h>

/* Exports */
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif

/*
 * Whether we're in an IRQ is bog-standard RISC-V on this SoC:
 * yes if the top mcause bit is set, otherwise no.
 */
SECTION_FUNC(exception.other, __soc_is_irq)
	csrr a0, mcause
	srli a0, a0, 31
	ret

/*
 * With a0 == irq_num, this is equivalent to:
 *
 * EVENT_UNIT->INTPTPENDCLEAR = (1U << irq_num);
 *
 * We could write this routine in C, but the assembly
 * that's calling us requires that a0 still contain irq_num
 * on return, and assuming nobody would ever change a
 * C implementation in a way that silently clobbers it
 * is playing with fire. Instead, we play tricks in
 * soc_context.h so that offsets.h contains a pointer to
 * INTPTPENDCLEAR.
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
	csrr t0, RI5CY_LPSTART0
	csrr t1, RI5CY_LPEND0
	csrr t2, RI5CY_LPCOUNT0
	sw t0, __soc_esf_t_lpstart0_OFFSET(a0)
	sw t1, __soc_esf_t_lpend0_OFFSET(a0)
	sw t2, __soc_esf_t_lpcount0_OFFSET(a0)
	csrr t0, RI5CY_LPSTART1
	csrr t1, RI5CY_LPEND1
	csrr t2, RI5CY_LPCOUNT1
	sw t0, __soc_esf_t_lpstart1_OFFSET(a0)
	sw t1, __soc_esf_t_lpend1_OFFSET(a0)
	sw t2, __soc_esf_t_lpcount1_OFFSET(a0)
#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

	ret

SECTION_FUNC(exception.other, __soc_restore_context)
#ifdef CONFIG_SOC_OPENISA_RV32M1_RI5CY
	lw t0, __soc_esf_t_lpstart0_OFFSET(a0)
	lw t1, __soc_esf_t_lpend0_OFFSET(a0)
	lw t2, __soc_esf_t_lpcount0_OFFSET(a0)
	csrw RI5CY_LPSTART0, t0
	csrw RI5CY_LPEND0, t1
	csrw RI5CY_LPCOUNT0, t2
	lw t0, __soc_esf_t_lpstart1_OFFSET(a0)
	lw t1, __soc_esf_t_lpend1_OFFSET(a0)
	lw t2, __soc_esf_t_lpcount1_OFFSET(a0)
	csrw RI5CY_LPSTART1, t0
	csrw RI5CY_LPEND1, t1
	csrw RI5CY_LPCOUNT1, t2
#endif /* CONFIG_SOC_OPENISA_RV32M1_RI5CY */

	ret
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
