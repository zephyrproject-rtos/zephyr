/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HVM guest-mode event vectors.
 *
 * Zephyr runs as a guest OS under the Hexagon Virtual Machine (HVM).
 * The hypervisor dispatches events to the guest via the Guest Event
 * Vector Base (GEVB) table set by the vmsetvec trap1 call.
 *
 * GEVB layout (offsets from base):
 *   0  Reset
 *   1  Machine check
 *   2  General exception
 *   3  Debug
 *   5  Trap0 (syscall)
 *   7  Interrupt
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <hexagon_vm.h>

GTEXT(__start)
GTEXT(__reset_handler)
GTEXT(_machine_check_handler)
GTEXT(_general_exception_handler)
GTEXT(_trap0_handler)
GTEXT(_interrupt_handler)

/*
 * Guest Event Vector Base (GEVB) table.
 *
 * The hypervisor dispatches to PC = GEVB + offset.  Each entry is a
 * single 4-byte jump instruction (packet).
 *
 *   Offset  Event
 *   ------  -----
 *   0       Reset
 *   1       Machine check
 *   2       General exception
 *   3       Debug
 *   4       (reserved)
 *   5       Trap0 (syscall)
 *   6       (reserved)
 *   7       Interrupt
 */
SECTION_FUNC(reset, __start)
	.p2align 2
	/* 0: Reset */
	jump #__reset_handler
	/* 1: Machine check */
	jump #_machine_check_handler
	/* 2: General exception */
	jump #_general_exception_handler
	/* 3: Debug */
	jump #_unhandled_event
	/* 4: Reserved */
	jump #_unhandled_event
	/* 5: Trap0 (syscall) */
	jump #_trap0_handler
	/* 6: Reserved */
	jump #_unhandled_event
	/* 7: Interrupt */
	jump #_interrupt_handler


/* Main reset handler */
SECTION_FUNC(TEXT, __reset_handler)
	/*
	 * Set the guest event vector base via the HVM vmsetvec call.
	 * Under H2, trap1 is handled by the hypervisor which sets GEVB.
	 */
	r0 = ##__start
	trap1(#HEXAGON_VM_vmsetvec)

	/*
	 * Set up initial stack pointer -- must be done before
	 * _setup_page_table because allocframe needs a valid
	 * (guest-accessible) stack.  Under H2, r29 initially points
	 * to the boot-VM's stack which is outside the guest's fence
	 * range.
	 */
	r29 = ##_interrupt_stack_end

	/* Clear frame pointer */
	r30 = #0

	/*
	 * Initialize BSS section BEFORE page table setup.
	 * The page table lives in BSS (.bss.page_table), so we must
	 * zero it first, then write the PDE entries and call vmnewmap.
	 * This runs under the initial linear mapping provided by H2.
	 */
	r0 = ##__bss_start
	r1 = ##__bss_end
	r2 = #0

	/* BSS clear using word stores (BSS end is ALIGN(4)) */
	r3 = sub(r1, r0)
	r3 = lsr(r3, #2)        /* number of 4-byte words */
	p0 = cmp.gt(r3, #0)
	if (!p0) jump #_bss_done

	loop0(_bss_loop, r3)
_bss_loop:
	{
		memw(r0++#4) = r2
	} :endloop0

_bss_done:

#ifdef CONFIG_MMU
	/* MMU mappings setup -- must follow BSS init (page table is in BSS) */
	call ##_setup_page_table
#endif

	/* Jump to C initialization */
	call ##z_prep_c

	/* Should never reach here */
	jump #_hang

_mmu_error:
    r0 = #3
    jump #_fatal_error


/* Event vector table marker */
SECTION_FUNC(event_vectors, _event_vector_table)

SECTION_FUNC(TEXT, _unhandled_event)
	r0 = #0xFF
	jump ##z_hexagon_fatal_error

SECTION_FUNC(TEXT, _machine_check_handler)
	r0 = #1
	jump ##z_hexagon_fatal_error

/*
 * General exception handler.
 * Route through the enhanced handler which saves full volatile context,
 * calls z_hexagon_exception_handler(), and handles preemption on exit.
 */
SECTION_FUNC(TEXT, _general_exception_handler)
	jump ##z_hexagon_enhanced_exception

/*
 * Trap0 (syscall) handler.
 * Route through the enhanced handler so that trap0 events get full
 * context save/restore.  The C handler z_hexagon_trap0_handler()
 * dispatches based on r6 (syscall number).
 */
SECTION_FUNC(TEXT, _trap0_handler)
	jump ##z_hexagon_enhanced_trap0

SECTION_FUNC(TEXT, _interrupt_handler)
	jump ##z_hexagon_enhanced_interrupt

_fatal_error:
	r0 = #2  /* VM_STOP_HALT */
	trap1(#HEXAGON_VM_vmstop)
	jump #_hang

_hang:
	jump #_hang

#ifdef CONFIG_MMU
SECTION_FUNC(TEXT, _setup_page_table)
	allocframe(#8)

	/*
	 * Use only volatile registers (r0-r15) -- callee-saved r16-r27
	 * must not be clobbered since this is called from __reset_handler
	 * before the C runtime is fully up.
	 *
	 * r5 = page table base address (kept across the function)
	 * r6 = number of 4MB PDE entries to fill
	 */
	r5 = ##_hexagon_page_table

	{
		r1 = ##_image_ram_end
		r2 = ##__start
	}
	r1 = sub(r1, r2)
	r3.h = #0x0040            /* 4MB - 1 for rounding */
	r3.l = #0x0000
	r3 = add(r3, #-1)
	r1 = add(r1, r3)
	r6 = lsr(r1, #22)         /* number of 4MB pages */

	r1 = ##__start
	r2.h = #0xffc0             /* 4MB alignment mask */
	r2.l = #0x0000
	r1 = and(r1, r2)
	r2 = lsr(r1, #22)
	r2 = asl(r2, #2)          /* PDE byte offset */
	r0 = add(r5, r2)

	r2 = #(__HVM_PTE_R | __HVM_PTE_W | __HVM_PTE_X)
	r3 = #(__HEXAGON_C_WB_L2 << 6)
	r2 = or(r2, r3)
	r2 = or(r2, #__HVM_PDE_S_4MB)

	r1 = add(r1, r2)
	r2.h = #0x0040
	r2.l = #0x0000

	p0 = cmp.gt(r6, #0)
	if (!p0) jump #_page_table_done

	loop0(_pte_loop, r6)
_pte_loop:
	{
		memw(r0++#4) = r1
		r1 = add(r1, r2)
	} :endloop0

_page_table_done:
	/* Map PL011 UART device at 0x10000000 (PDE byte offset 0x100).
	 * The shared bit bypasses H2's guestmap translation so the
	 * guest physical address is used directly as the host address.
	 */
	r0 = add(r5, #0x100)
	r1.h = #0x1000
	r1.l = #0x0000
	r2 = #(__HVM_PTE_R | __HVM_PTE_W | __HVM_PTE_SHARED)
	r3 = #(__HEXAGON_C_DEV << 6)
	r2 = or(r2, r3)
	r2 = or(r2, #__HVM_PDE_S_4MB)
	r1 = add(r1, r2)
	memw(r0+#0) = r1

	/* Map H2 kernel at 0x9b800000 (PDE byte offset 0x9B8) */
	r0 = add(r5, #0x9B8)
	r1.h = #0x9b80
	r1.l = #0x0000
	r2 = #(__HVM_PTE_R | __HVM_PTE_W | __HVM_PTE_X)
	r3 = #(__HEXAGON_C_WB_L2 << 6)
	r2 = or(r2, r3)
	r2 = or(r2, #__HVM_PDE_S_4MB)
	r1 = add(r1, r2)
	memw(r0+#0) = r1

	r1.h = #0x9bc0
	r1.l = #0x0000
	r1 = add(r1, r2)
	memw(r0+#4) = r1

	/* Use trap1 for vmnewmap -- handled by the hypervisor */
	r0 = r5
	r1 = #VM_TRANS_TYPE_TABLE
	r2 = #VM_TLB_INVALIDATE_TRUE
	trap1(#HEXAGON_VM_vmnewmap)

	p0 = cmp.eq(r0, #0)
	if (!p0) jump #_mmu_error

	deallocframe
	jumpr r31

	.section .bss.page_table, "aw", @nobits
	.align 4096
_hexagon_page_table:
	.skip 4096
#endif /* CONFIG_MMU */
