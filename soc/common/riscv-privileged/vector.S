/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
#if defined(CONFIG_USE_ISR_WRAPPER)
GTEXT(_isr_wrapper)
#endif

SECTION_FUNC(vectors, __start)
#if defined(CONFIG_RISCV_GP)
	/* Initialize global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
#endif

	.option norvc;

#if defined(CONFIG_SOC_RESET_HOOK)
	call soc_reset_hook
#endif

#ifdef CONFIG_POSITION_INDEPENDENT
    /*
	 * A static PIE has no dynamic linker, so only load-bias fixups
	 * (R_RISCV_RELATIVE) can appear in .rela.dyn; symbol-resolution
	 * relocations like R_RISCV_JUMP_SLOT/GLOB_DAT don't apply here.
	 */
#define REL_RISCV_RELATIVE  3

    /*
	 * .rela.dyn entry layout: Elf32_Rela is { r_offset, r_info, r_addend }
	 * with 4-byte fields, Elf64_Rela is the same, but with 8-byte fields.
	 * The relocation type is the low 32 bits of r_info on both.
	 */
#ifdef CONFIG_64BIT
#define REL_OFFSET_OFF  0
#define REL_INFO_OFF    8
#define REL_ADDEND_OFF  16
#define REL_ENTRY_SIZE  24
#define REL_LOAD        ld
#define REL_STORE       sd
#define REL_ALIGN_MASK  0x7
#define REL_WORD_BYTES  8
#else
#define REL_OFFSET_OFF  0
#define REL_INFO_OFF    4
#define REL_ADDEND_OFF  8
#define REL_ENTRY_SIZE  12
#define REL_LOAD        lw
#define REL_STORE       sw
#define REL_ALIGN_MASK  0x3
#define REL_WORD_BYTES  4
#endif

    /*
	 * Calculate the delta between linker time address and run time address
	 */
reloc_pc:
    auipc t0, 0       /* Get the runtime address of this instruction */
    la t1, reloc_pc   /* Get the linker time address of this instruction */
    sub t2, t0, t1

    /*
	 * A misaligned load delta would corrupt every relocated jump target
	 * (mtvec, __reset, ISR handlers, ...), causing a confusing fault far
	 * from the real cause. Halt here instead, at a known location.
	 */
    andi t5, t2, 0x3
    beqz t5, delta_aligned
delta_misaligned:
    j delta_misaligned
delta_aligned:

    /* adjust gp pointer */
    add x3, x3, t2

    /* Save offset to global variable */
    la t0, z_load_offset
    add t0, t0, t2
    REL_STORE t2, 0(t0)

#ifndef CONFIG_RISCV_VECTORED_MODE
    /* since the entries in _irq_vector_table has not been
     * relocated, we only support non-vector mode here before
     * relocation. For vector mode, the handler will be set
     * after relocation.
     */
    la t0, _isr_wrapper
    add t0, t0, t2
#ifdef CONFIG_RISCV_S_MODE
	csrw stvec, t0
#else
	csrw mtvec, t0
#endif
#endif

    /* Do relocation work according to rela.dyn section */
    la  t0, __rel_dyn_start
    la  t1, __rel_dyn_end
    beq t0, t1, rel_skip
    add t0, t0, t2
    add t1, t1, t2
    li  t6, 0

rel_loop:
    lw t3, REL_INFO_OFF(t0)     /* load relocation type (low 32 bits of r_info) */
    li t4, REL_RISCV_RELATIVE
    bne t3, t4, rel_handle_next
    /* case 1: handle RISCV_RELATIVE type */
    REL_LOAD t3, REL_OFFSET_OFF(t0)
    REL_LOAD t4, REL_ADDEND_OFF(t0)
    add t3, t3, t2      /* Adjusted GOT entry address */
    add t4, t4, t2      /* Adjusted target address */
    andi t5, t3, REL_ALIGN_MASK /* Check whether the address is naturally aligned */
    bnez t5, rel_not_word_aligned
    REL_STORE t4, 0(t3) /* Fix GOT entry */
    j rel_handle_next
rel_not_word_aligned:
#ifdef CONFIG_64BIT
    /* Misaligned 64-bit target: store it one byte at a time */
    li t5, REL_WORD_BYTES
rel_store_byte:
    sb t4, 0(t3)
    srli t4, t4, 8
    addi t3, t3, 1
    addi t5, t5, -1
    bnez t5, rel_store_byte
#else
    andi t5, t5, 0x1    /* check whether the address is 2 bytes aligned */
    bnez t5, rel_not_half_word_aligned
    srli t5, t4, 16
    sh t4, 0(t3)        /* store lower half word */
    sh t5, 2(t3)        /* store higher half word */
    j rel_handle_next
rel_not_half_word_aligned:
    sb t4, 0(t3)
    srli t4, t4, 8
    sb t4, 1(t3)
    srli t4, t4, 8
    sb t4, 2(t3)
    srli t4, t4, 8
    sb t4, 3(t3)
#endif /* CONFIG_64BIT */
rel_handle_next:
    addi t0, t0, REL_ENTRY_SIZE
    blt t0, t1, rel_loop
rel_skip:
#endif

#if defined(CONFIG_RISCV_VECTORED_MODE)
#if defined(CONFIG_RISCV_HAS_CLIC)

	/*
	 * CLIC vectored mode
	 *
	 * CLIC vectored mode uses mtvec exclusively for exception handling and
	 * mtvec.base must be aligned to 64 bytes (this is done using
	 * CONFIG_RISCV_TRAP_HANDLER_ALIGNMENT)
	 */
#if defined(CONFIG_USE_ISR_WRAPPER)
	la t0, _isr_wrapper
#else
	add t0, zero, zero
#endif
	addi t0, t0, 0x03 /* Enable CLIC vectored mode by setting LSB */
	csrw mtvec, t0

	/*
	 * CLIC vectored mode has a similar concept to CLINT vectored mode,
	 * where an interrupt vector table is used for specific interrupts.
	 * However, in CLIC vectored mode, the handler table contains the
	 * address of the interrupt handler instead of an opcode containing a
	 * jump instruction, this is done by leveraging
	 * CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS.
	 * When an interrupt occurs in CLIC vectored mode, the address of the
	 * handler entry from the vector table is loaded and then jumped to in
	 * hardware. This time mtvt is used as the base address for the
	 * interrupt table.
	 */
	la t0, _irq_vector_table
	csrw 0x307, t0 /* mtvt */

#else /* !CONFIG_RISCV_HAS_CLIC */

	/*
	 * CLINT vectored mode
	 *
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _irq_vector_table (interrupt vector table). Add 1 to base
	 * address of _irq_vector_table to indicate that vectored mode
	 * is used (LSB = 0x1). CPU will mask the LSB out of
	 * the address so that base address of _irq_vector_table is used.
	 *
	 * NOTE: _irq_vector_table is 256-byte aligned. Incorrect alignment
	 *        of _irq_vector_table breaks this code.
	 */
	la t0, _irq_vector_table /* Load address of interrupt vector table */
	addi t0, t0, 0x01 /* Enable vectored mode by setting LSB */
#ifdef CONFIG_RISCV_S_MODE
	csrw stvec, t0
#else
	csrw mtvec, t0
#endif

#endif /* CONFIG_RISCV_HAS_CLIC */

#else /* !CONFIG_RISCV_VECTORED_MODE */

#if defined(CONFIG_RISCV_HAS_CLIC) && !defined(CONFIG_LEGACY_CLIC)

	la t0, _isr_wrapper
	addi t0, t0, 0x03 /* Set mode bits to 3, signifying CLIC. Everything else is reserved. */
	csrw mtvec, t0

#else /* !CONFIG_RISCV_HAS_CLIC || CONFIG_LEGACY_CLIC */

	/*
	 * CLINT direct mode
	 *
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _isr_wrapper.
	 */
	la t0, _isr_wrapper
#ifdef CONFIG_RISCV_S_MODE
	csrw stvec, t0
#else
	csrw mtvec, t0
#endif

#endif /* CONFIG_RISCV_HAS_CLIC&& !CONFIG_LEGACY_CLIC */

#endif /* CONFIG_RISCV_VECTORED_MODE */

	/* Jump to __reset */
	tail __reset
