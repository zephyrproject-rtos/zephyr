/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_X86_KPTI
	MMU_PAGE_ALIGN_PERM
	z_shared_kernel_page_start = .;
	/* Special page containing supervisor data that is still mapped in
	 * user mode page tables. IDT, GDT, TSSes, trampoline stack, and
	 * any LDT must go here as they always must live in a page that is
	 * marked 'present'. Still not directly user accessible, but
	 * no sensitive data should be here as Meltdown exploits may read it.
	 */
#endif /* CONFIG_X86_KPTI */

#ifdef CONFIG_DYNAMIC_INTERRUPTS
	. = ALIGN(8);
	_idt_base_address = .;
#ifdef LINKER_ZEPHYR_FINAL
	KEEP(*(staticIdt))
#else
	. += CONFIG_IDT_NUM_VECTORS * 8;
#endif /* LINKER_ZEPHYR_FINAL */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#ifdef CONFIG_GDT_DYNAMIC
	KEEP(*(.tss))
	. = ALIGN(8);
	_gdt = .;
#ifdef LINKER_ZEPHYR_FINAL
	KEEP(*(gdt))
#else /* LINKER_ZEPHYR_FINAL */

#if defined(CONFIG_THREAD_LOCAL_STORAGE) && !defined(CONFIG_X86_64)
#define GDT_NUM_TLS_ENTRIES 1
#else
#define GDT_NUM_TLS_ENTRIES 0
#endif

#ifdef CONFIG_USERSPACE
    #define GDT_NUM_ENTRIES 7
#elif defined(CONFIG_HW_STACK_PROTECTION)
    #define GDT_NUM_ENTRIES 5
#else
    #define GDT_NUM_ENTRIES 3
#endif /* CONFIG_X86_USERSPACE */
	. += (GDT_NUM_ENTRIES + GDT_NUM_TLS_ENTRIES) * 8;
	. += CONFIG_GDT_RESERVED_NUM_ENTRIES * 8;
#endif /* LINKER_ZEPHYR_FINAL */
#endif /* CONFIG_GDT_DYNAMIC */

#ifdef CONFIG_X86_KPTI
	z_trampoline_stack_start = .;
	MMU_PAGE_ALIGN_PERM
	z_trampoline_stack_end = .;
	z_shared_kernel_page_end = .;

	ASSERT(z_trampoline_stack_end - z_trampoline_stack_start >= 40,
		"trampoline stack too small");
	ASSERT(z_shared_kernel_page_end - z_shared_kernel_page_start == 4096,
	       "shared kernel area is not one memory page");
#endif /* CONFIG_X86_KPTI */
