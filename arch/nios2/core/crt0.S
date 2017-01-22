/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/nios2/asm.h>
#include <kernel_structs.h>

/* exports */
GTEXT(__start)
GTEXT(__reset)

/* imports */
GTEXT(_PrepC)
GTEXT(_interrupt_stack)

	/* Allow use of r1/at (the assembler temporary register) in this
	 * code, normally reserved for internal assembler use
	 */
	.set noat


#if CONFIG_INCLUDE_RESET_VECTOR
/*
 * Reset vector entry point into the system. Placed into special 'reset'
 * section so that the linker puts this at ALT_CPU_RESET_ADDR defined in
 * system.h
 *
 * This code can be at most 0x20 bytes, since the exception vector for Nios II
 * is usually configured to be 0x20 past the reset vector.
 */
SECTION_FUNC(reset, __reset)

#if ALT_CPU_ICACHE_SIZE > 0
	/* Aside from the instruction cache line associated with the reset
	 * vector, the contents of the cache memories are indeterminate after
	 * reset. To ensure cache coherency after reset, the reset handler
	 * located at the reset vector must immediately initialize the
	 * instruction cache. Next, either the reset handler or a subsequent
	 * routine should proceed to initialize the data cache.
	 *
	 * The cache memory sizes are *always* a power of 2.
	 */
#if ALT_CPU_ICACHE_SIZE > 0x8000
	movhi r2, %hi(ALT_CPU_ICACHE_SIZE)
#else
	movui r2, ALT_CPU_ICACHE_SIZE
#endif
0:
	/* If ECC present, need to execute initd for each word address
	 * to ensure ECC parity bits in data RAM get initialized
	 */
#ifdef ALT_CPU_ECC_PRESENT
	subi r2, r2, 4
#else
	subi r2, r2, ALT_CPU_ICACHE_LINE_SIZE
#endif
	initi r2
	bgt r2, zero, 0b
#endif /* ALT_CPU_ICACHE_SIZE > 0 */

	/* Done all we need to do here, jump to __text_start */
	movhi r1, %hi(__start)
	ori r1, r1, %lo(__start)
	jmp r1
#endif /* CONFIG_INCLUDE_RESET_VECTOR */

/* Remainder of asm-land initialization code before we can jump into
 * the C domain
 */
SECTION_FUNC(TEXT, __start)

	/* TODO if shadow register sets enabled, ensure we are in set 0
	 * ZEP-258
	 */

	/* Initialize the data cache if booting from bare metal. If
	 * we're not booting from our reset vector, either by a bootloader
	 * or JTAG, assume caches already initialized.
	 */
#if ALT_CPU_DCACHE_SIZE > 0 && defined(CONFIG_INCLUDE_RESET_VECTOR)
	/* Per documentation data cache size is always a power of two. */
#if ALT_CPU_DCACHE_SIZE > 0x8000
	movhi r2, %hi(ALT_CPU_DCACHE_SIZE)
#else
	movui r2, ALT_CPU_DCACHE_SIZE
#endif
0:
	/* If ECC present, need to execute initd for each word address
	 * to ensure ECC parity bits in data RAM get initialized
	 */
#ifdef ALT_CPU_ECC_PRESENT
	subi r2, r2, 4
#else
	subi r2, r2, ALT_CPU_DCACHE_LINE_SIZE
#endif
	initd 0(r2)
	bgt r2, zero, 0b
#endif /* ALT_CPU_DCACHE_SIZE && defined(CONFIG_INCLUDE_RESET_VECTOR) */

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in _interrupt_stack with 0xAA
	 * init.c enforces that the _interrupt_stack pointer
	 * and CONFIG_ISR_STACK_SIZE are a multiple of STACK_ALIGN (4) */
	movhi r1, %hi(_interrupt_stack)
	ori r1, r1, %lo(_interrupt_stack)
	movhi r2, %hi(CONFIG_ISR_STACK_SIZE)
	ori r2, r2, %lo(CONFIG_ISR_STACK_SIZE)
	/* Put constant 0xaaaaaaaa in r3 */
	movhi r3, 0xaaaa
	ori r3, r3, 0xaaaa
1:
	/* Loop through the _interrupt_stack treating it as an array of
	 * uint32_t, setting each element to r3 */
	stw r3, (r1)
	subi r2, r2, 4
	addi r1, r1, 4
	blt r0, r2, 1b
#endif

	/* Set up the initial stack pointer to the interrupt stack, safe
	 * to use this as the CPU boots up with interrupts disabled and we
	 * don't turn them on until much later, when the kernel is on
	 * the main stack */
	movhi sp, %hi(_interrupt_stack)
	ori sp, sp, %lo(_interrupt_stack)
	addi sp, sp, CONFIG_ISR_STACK_SIZE

#if defined(CONFIG_GP_LOCAL) || defined(CONFIG_GP_GLOBAL) || \
		defined(CONFIG_GP_ALL_DATA)
	/* Initialize global pointer with the linker variable we set */
	movhi gp, %hi(_gp)
	ori gp, gp, %lo(_gp)
#endif

	/* TODO if shadow register sets enabled, interate through them to set
	 * up. Need to clear r0, write gp, set the execption stack pointer
	 * ZEP-258
	 */

	/* Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters _Cstart */
	call _PrepC

