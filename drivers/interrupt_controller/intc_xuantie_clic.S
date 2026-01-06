/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>

#define IRQ_NESTED_MAX          6

#if __riscv_xlen == 32
#define REGBYTES                4
#else
#define REGBYTES                8
#endif

#if __riscv_xlen == 32
.section .bss
.align  2
irq_nested_level:
.long 0
irq_nested_mcause:
.long 0, 0, 0, 0, 0, 0
#else
.section .bss
.align  3
.global irq_nested_level
irq_nested_level:
.space 8 * CONFIG_MP_MAX_NUM_CPUS
irq_nested_level_end:
irq_nested_mcause:
.space 8 * CONFIG_MP_MAX_NUM_CPUS * IRQ_NESTED_MAX
irq_nested_mcause_end:
#endif

/* Exports */
GTEXT(__soc_handle_irq)

/*
 * No need to clear anything, pending bit is cleared by HW.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
    addi    sp, sp, -REGBYTES
    sw      ra, 0(sp)
    csrr    a0, mcause
    li      t0,  CONFIG_RISCV_MCAUSE_EXCEPTION_MASK
    and     a0, a0, t0
    la      t0, riscv_clic_irq_clear_pending
    jalr    t0
    lw      ra, 0(sp)
    addi    sp, sp, REGBYTES
    ret

#if !defined(CONFIG_RISCV_VECTORED_MODE)

GTEXT(__soc_handle_all_irqs)

#if __riscv_xlen == 32

SECTION_FUNC(exception.other, __soc_handle_all_irqs)
    addi    sp, sp, -REGBYTES
    sw      ra, 0(sp)

    jal     ra, __soc_handle_irq

#if CONFIG_XUANTIE_CLIC_IRQ_NEST
    la      t0, irq_nested_level
    lw      t1, (t0)
    addi    t1, t1, 1
    sw      t1, (t0)
    li      t0, IRQ_NESTED_MAX
    bgt     t1, t0, is_trap_xx
#endif

    addi    sp, sp, -REGBYTES
    csrr    a0, mcause
    sw      a0, 0(sp)

#if CONFIG_XUANTIE_CLIC_IRQ_NEST
    csrs    mstatus, (1 << 3)
#endif

    li t0, CONFIG_RISCV_MCAUSE_EXCEPTION_MASK
    and a0, a0, t0
    /*
    * Call corresponding registered function in _sw_isr_table.
    * (table is 2-word wide, we should shift index accordingly)
    */
    la t0, _sw_isr_table
    slli a0, a0, (RV_REGSHIFT + 1)
    add t0, t0, a0

    /* Load argument in a0 register */
    lw a0, 0(t0)

    /* Load ISR function address in register t1 */
    lw t1, RV_REGSIZE(t0)

    /* Call ISR function */
    jalr ra, t1, 0

#if CONFIG_XUANTIE_CLIC_IRQ_NEST
    csrc    mstatus, (1 << 3)
    la      t0, irq_nested_level
    lw      t1, (t0)
    addi    t1, t1, -1
    sw      t1, (t0)
    bgt     t1, zero, mcause_handle
#endif

    addi    sp, sp, REGBYTES

    lw      ra, 0(sp)
    addi    sp, sp, REGBYTES
    ret
mcause_handle:
    /* keep mpil in current mcause & load exception code before */
    addi    t1, t1, -1
    la      t0, irq_nested_mcause
    slli    t1, t1, 2
    add     t1, t0, t1
    lw      t0, (t1)
    andi    t0, t0, 0x3FF

    lw      a1, 0(sp)
    addi    sp, sp, REGBYTES

    andi    a0, a1, 0xFFFFFC00
    or      t0, a0, t0
    csrw    mcause, t0

    lw      ra, 0(sp)
    addi    sp, sp, REGBYTES
    ret

#else

SECTION_FUNC(exception.other, __soc_handle_all_irqs)
    addi    sp, sp, -REGBYTES
    sw      ra, 0(sp)
    jal     ra, __soc_handle_irq

#if CONFIG_XUANTIE_CLIC_IRQ_NEST
    csrr    t3, mhartid

    la      t0, irq_nested_level
    slli    t2, t3, 3    /* mhartid * 8 */
    add     t0, t0, t2
    ld      t1, (t0)
    addi    t1, t1, 1
    sd      t1, (t0)

    li      t0, IRQ_NESTED_MAX
    /* nested too deeply, may be error happens */
    bgt     t1, t0, is_trap_xx
#endif

    addi    sp, sp, -REGBYTES
    csrr    a0, mcause
    sw      a0,0(sp)

#if CONFIG_XUANTIE_CLIC_IRQ_NEST
    csrs    mstatus, (1 << 3)
#endif

    li t0, CONFIG_RISCV_MCAUSE_EXCEPTION_MASK
    and a0, a0, t0
    /*
    * Call corresponding registered function in _sw_isr_table.
    * (table is 2-word wide, we should shift index accordingly)
    */
    la t0, _sw_isr_table
    slli a0, a0, (RV_REGSHIFT + 1)
    add t0, t0, a0

    /* Load argument in a0 register */
    ld a0, 0(t0)

    /* Load ISR function address in register t1 */
    ld t1, RV_REGSIZE(t0)

    /* Call ISR function */
    jalr ra, t1, 0

#if CONFIG_XUANTIE_CLIC_IRQ_NEST
    csrc    mstatus, (1 << 3)

    csrr    t3, mhartid

    la      t0, irq_nested_level
    slli    t2, t3, 3    /* mhartid * 8 */
    add     t0, t0, t2
    ld      t1, (t0)
    addi    t1, t1, -1
    sd      t1, (t0)
    bgt     t1, zero, mcause_handle
#endif

    addi    sp, sp, REGBYTES

    lw      ra, 0(sp)
    addi    sp, sp, REGBYTES
ret

mcause_handle:
/* keep mpil in current mcause & load exception code before */
    addi    t1, t1, -1
    la      t0, irq_nested_mcause
    li      t2, 8 * IRQ_NESTED_MAX
    mul     t2, t2, t3
    slli    t1, t1, 3
    add     t2, t2, t1
    add     t0, t0, t2
    mv      t1, t0

    ld      t0, (t1)
    andi    t0, t0, 0x3FF

    lw      a1,0(sp)
    addi    sp, sp, REGBYTES

    srli    a0, a1, 11
    slli    a0, a0, 11
    or      t0, a0, t0
    csrw    mcause, t0
    lw      ra, 0(sp)
    addi    sp, sp, REGBYTES
    ret
#endif
is_trap_xx:
    j .
#endif
