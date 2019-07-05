/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DEVICE_INIT_LEVEL(level) \
	__device_##level##_start = .; \
	KEEP(*(SORT(.init_##level[0-9]))); \
	KEEP(*(SORT(.init_##level[1-9][0-9])));

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
    /*
     * The "locore" must be in the 64K of RAM, so that 16-bit code (with
     * segment registers == 0x0000) and 32/64-bit code agree on addresses.
     * ... there is no 16-bit code yet, but there will be when we add SMP.
     */

    .locore 0x8000 : ALIGN(16)
	{
	*(.locore)
	*(.locore.*)
	}

    /*
     * The parts of the system that don't benefit from being in the locore
     * start at the 1MB mark, otherwise the system size would be artificially
     * clamped by the ISA memory hole/ROM space at 0x90000-0xFFFFF.
     */

    .text 0x100000 : ALIGN(16)
	{
	*(.text)
	*(.text.*)
	*(.rodata.*)

	. = ALIGN(8);
	__devconfig_start = .;
	*(.devconfig.*)
	KEEP(*(SORT_BY_NAME(.devconfig*)))
	__devconfig_end = .;
	}

    .data : ALIGN(16)
	{
	*(.data*)

	. = ALIGN(8);
	__device_init_start = .;
	DEVICE_INIT_LEVEL(PRE_KERNEL_1)
	DEVICE_INIT_LEVEL(PRE_KERNEL_2)
	DEVICE_INIT_LEVEL(POST_KERNEL)
	DEVICE_INIT_LEVEL(APPLICATION)
	__device_init_end = .;

	. = ALIGN(8);
	__static_thread_data_list_start = .;
	KEEP(*(SORT_BY_NAME(.__static_thread_data.static.*)))
	__static_thread_data_list_end = .;

	. = ALIGN(8);
	_k_mem_pool_list_start = .;
	KEEP(*(._k_mem_pool_static.*))
	_k_mem_pool_list_end = .;
	}

    .bss : ALIGN(16)
	{
	__bss_start = .;
	*(.bss)
	*(.bss.*)
	*(COMMON)
	. = ALIGN(8);	/* so __bss_num_qwords is exact */
	__bss_end = .;
	}

	__bss_num_qwords = (__bss_end - __bss_start) >> 3;

    .noinit (NOLOAD) : ALIGN(16)
	{
	*(.noinit.*)
	}

    /DISCARD/ :
	{
	*(.comment*)
	*(.debug*)
	*(.got*)
	*(.igot.*)
	*(.iplt)
	*(.note.GNU-stack)
	*(.rela.*)
	}
}
