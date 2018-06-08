/*
 * Copyright (c) 2018 ispace, inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <autoconf.h>
#include <arch/sparc/arch.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(z_sparc_prep_c)
GDATA(_interrupt_stack)

#define RESET_ENTRY(handler)   	 \
	ba handler;                 \
	nop;                        \
	nop;                        \
	nop;

/* task terminating exceptions */
#define FAULT_TRAP_ENTRY()      \
	ta 0;                       \
	nop;                        \
	nop;                        \
	nop;

/* not implemented */
#define SOFT_TRAP_ENTRY()       \
	ta 0;                       \
	nop;                        \
	nop;                        \
	nop;

/* trap not defined. dummy entry */
#define EMPTY()                 \
	ta 0;                       \
	nop;                        \
	nop;                        \
	nop;


SECTION_FUNC(vector_table, __start)

/* trap table */
/* 0x00 - 0x0f */
RESET_ENTRY(reset_handler)       /* 00: reset */
FAULT_TRAP_ENTRY()               /* 01: inst access exception */
FAULT_TRAP_ENTRY()               /* 02: illegal inst */
FAULT_TRAP_ENTRY()               /* 03: priveleged inst */

FAULT_TRAP_ENTRY()               /* 04: fp disabled */
FAULT_TRAP_ENTRY()               /* 05: window overflow */
FAULT_TRAP_ENTRY()               /* 06: window underflow */
FAULT_TRAP_ENTRY()               /* 07: memory access not aligned */

FAULT_TRAP_ENTRY()               /* 08: floating point exveption */
FAULT_TRAP_ENTRY()               /* 09: data miss exception */
FAULT_TRAP_ENTRY()               /* 0a: tagged inst overflow */
FAULT_TRAP_ENTRY()               /* 0b: watchpoint detected */

EMPTY()                          /* 0c: */
EMPTY()                          /* 0d: */
EMPTY()                          /* 0e: */
EMPTY()                          /* 0f: */

/* 0x10 - 0x1f */
EMPTY()                          /* 10: */
EMPTY()                          /* 11: irq level 1 */
EMPTY()                          /* 12: irq level 2 */
EMPTY()                          /* 13: irq level 3 */

EMPTY()                          /* 14: irq level 4 */
EMPTY()                          /* 15: irq level 5 */
EMPTY()                          /* 16: irq level 6 */
EMPTY()                          /* 17: irq level 7 */

EMPTY()                          /* 18: irq level 8 */
EMPTY()                          /* 19: irq level 9 */
EMPTY()                          /* 1a: irq level 10 */
EMPTY()                          /* 1b: irq level 11 */

EMPTY()                          /* 1c: irq level 12 */
EMPTY()                          /* 1d: irq level 13 */
EMPTY()                          /* 1e: irq level 14 */
EMPTY()                          /* 1f: irq level 15 */

/* 0x20 - 0x2f */
FAULT_TRAP_ENTRY()               /* 20: r reg access err */
FAULT_TRAP_ENTRY()               /* 21: inst access err */
EMPTY()                          /* 22: */
EMPTY()                          /* 23: */

FAULT_TRAP_ENTRY()               /* 24: co-processor disabled */
FAULT_TRAP_ENTRY()               /* 25: unimplemented flush */
EMPTY()                          /* 26: */
EMPTY()                          /* 27: */

FAULT_TRAP_ENTRY()               /* 28: co-processor exception */
FAULT_TRAP_ENTRY()               /* 29: data access err */
FAULT_TRAP_ENTRY()               /* 2a: division by zero */
FAULT_TRAP_ENTRY()               /* 2b: data store err */

FAULT_TRAP_ENTRY()               /* 2c: data access mmu miss */
FAULT_TRAP_ENTRY()               /* 2d: */
EMPTY()                          /* 2e: */
EMPTY()                          /* 2f: */

/* 0x30 - 0x3f */
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
/* 3c: instruction_access_mmu_miss */
FAULT_TRAP_ENTRY()   EMPTY() EMPTY() EMPTY()
/* 0x40 - 0x4f */
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
/* 0x50 - 0x5f */
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
/* 0x60 - 0x6f */
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
/* 0x70 - 0x7f */
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
EMPTY() EMPTY() EMPTY() EMPTY()
/* 0x80 - 0x8f */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
/* 0x90 - 0x9f */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
/* 0xa0 - 0xaf */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
/* 0xb0 - 0xbf */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
/* 0xc0 - 0xcf */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
/* 0xd0 - 0xdf */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
/* 0xe0 - 0xef */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
/* 0xf0 - 0xff */
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()
SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY() SOFT_TRAP_ENTRY()


	.align  32

	/* We assume that both I and D cache are disabled before reaching */
	/* here. The initialization is in the following order:
	 *  1) Initialize the special registers (TBR, WIM, PSR)
	 *  2) Initialize stack and frame pointer
	 *  3) Enable trap
	 *  4) Jump to the C World
	 *
	 * Once we are done preparing, jump to the C world */
SECTION_FUNC(text, reset_handler)

	/* Initialize the Trap Base Register */
	sethi %hi(__start), %g1 /* We have trap vector at __start */
	mov %g1, %tbr           /* Write to %tbr */
	nop                     /* %tbr is delayed-write */
	nop                     /* We assume three delay instructions */
	nop

	/* Initialize the Window Invalid Mask */
	mov INIT_WIM, %g1      /* Move INIT_WIM to %g1 */
	mov %g1, %wim           /* Write to %wim */
	nop                     /* %wim is delayed-write */
	nop                     /* We assume three delay instructions */
	nop

	/* Initialize the Processor State Register */
	set INIT_PSR, %g1       /* Move INIT_PSR to %g1 */
	mov %g1, %psr           /* Write to %psr */
	nop                     /* %psr is delayed-write */
	nop                     /* We assume three delay instructions */
	nop

	/* Initialize the stack pointer.  We use _interrupt_stack for
	 * kernel initialization.  The stack starts at
	 * _interrupt_stack + CONFIG_ISR_STACK_SIZE.  Because "add"
	 * instruction can only take simm13, we load both the address
	 * of _interrupt_stack and the size of it on registers, then
	 * add them together.  We first initialize %fp here and then
	 * initialize %sp later. */
	set _interrupt_stack, %g1       /* Load _interrut_stack to %g1 */
	set CONFIG_ISR_STACK_SIZE, %g2  /* Load stack size */
	add %g1, %g2, %fp               /* Add them to calculate the end of */
					/* the stack as %fp */

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all byets in _initerrupt_stack with 0xAA. */
	/* We use double word store instruction to speed things up. */
	set 0xaaaaaaaa, %g4	/* Set %g4 first */
	mov %g4, %g5		/* then, copy it to %g5 */
fill_aa:
	std %g4, [%g1]		/* start from low address */
	inc 8, %g1		/* increment by double word 8 bytes */
	cmp %g1, %fp		/* did we pass the top of stack? */
	ble fill_aa		/* go back if not */
	nop
#endif /* CONFIG_INIT_STACKS */

	/* %sp must be aligned to double word.
	 * __STD_STACK_FRAME_SIZEOF is aligned to 8 bytes.  Thus, need
	 * to make sure %fp is aligned. */
	andn %fp, STACK_ALIGN_SIZE, %fp	/* Drop the LSB bits to round down to */
					/* 8 bytes */

	/* We must have at least 96 bytes of stack frame on the stack
	 * _before_ calling a C function.  It is the storage area to
	 * spill registers on window overflow. Even though we use -mflat
	 * compiler option for now, it is required. Also note that even
	 * if we have -fomit-frame-pointer, we are fine having %fp,
	 * because it'll just be overwritten in the C world. */
	sub %fp, __STD_STACK_FRAME_SIZEOF, %sp

	/* Stack is ready.  Jump to the C world */
	ba z_sparc_prep_c
	nop
