/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Common parts of the linker scripts for the ARCv2/EM targets.
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <sections.h>

#if defined(CONFIG_NSIM)
	EXTERN(_VectorTable)
#endif

#include <linker-defs.h>
#include <linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_HARVARD
	#define ROMABLE_REGION ICCM
	#define RAMABLE_REGION DCCM
#else
	#if defined(CONFIG_XIP) && (FLASH_SIZE != 0)
		#define ROMABLE_REGION FLASH
		#define RAMABLE_REGION SRAM
	#else
		#define ROMABLE_REGION SRAM
		#define RAMABLE_REGION SRAM
	#endif
#endif

#if defined(CONFIG_XIP)
	#define _DATA_IN_ROM __data_rom_start
#else
	#define _DATA_IN_ROM
#endif

OUTPUT_ARCH(arc)
ENTRY(__start)

MEMORY {
#ifdef FLASH_START
	FLASH (rx)  : ORIGIN = FLASH_START, LENGTH = FLASH_SIZE*1k
#endif
#ifdef ICCM_START
	ICCM  (rx)  : ORIGIN = ICCM_START,  LENGTH = ICCM_SIZE*1k
#endif
#ifdef SRAM_START
	SRAM  (rwx) : ORIGIN = SRAM_START,  LENGTH = SRAM_SIZE*1k
#endif
#ifdef DCCM_START
	DCCM  (rw)  : ORIGIN = DCCM_START,  LENGTH = DCCM_SIZE*1k
#endif
}

SECTIONS {
	GROUP_START(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,ALIGN(1024)) {
		_image_rom_start = .;
		_image_text_start = .;

/* when !XIP, .text is in RAM, and vector table must be at its very start */

		KEEP(*(.exc_vector_table))
		KEEP(*(".exc_vector_table.*"))


		KEEP(*(.irq_vector_table))
		KEEP(*(".irq_vector_table.*"))

#ifdef CONFIG_VERSION_HEADER
		*(.version_header)
		KEEP(*(".version_header*"))
#endif
#ifndef CONFIG_SW_ISR_TABLE_DYNAMIC
		KEEP(*(.isr_irq*))

		/*
		 * The following sections maps the location of the different
		 * rows for the _sw_isr_table. Each row maps to an IRQ entry
		 * (handler, argument).
		 *
		 * In ARC architecture, IRQ 0-15 are reserved for the system
		 * and are not * assignable by the user, for that reason the
		 * linker sections start on IRQ 16
		 */

		/* sections for IRQ16-19 */
		KEEP(*(SORT(.gnu.linkonce.isr_irq[1][6-9])))
		/* sections for IRQ20-99 */
		KEEP(*(SORT(.gnu.linkonce.isr_irq[2-9][0-9])))
		/* sections for IRQ100-999 */
		KEEP(*(SORT(.gnu.linkonce.isr_irq[1-9][0-9][0-9])))
#endif

		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)

		_image_text_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE (devconfig, (OPTIONAL),)
	{
		__devconfig_start = .;
		*(".devconfig.*")
		KEEP(*(SORT_BY_NAME(".devconfig*")))
		__devconfig_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(gpio_compat, (OPTIONAL),)
	{
		__gpio_compat_start = .;
		*(".gpio_compat.*")
		KEEP(*(SORT_BY_NAME(".gpio_compat*")))
		__gpio_compat_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(net_l2, (OPTIONAL),)
	{
		__net_l2_start = .;
		*(".net_l2.*")
		KEEP(*(SORT_BY_NAME(".net_l2*")))
		__net_l2_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CPLUSPLUS
	SECTION_PROLOGUE(_CTOR_SECTION_NAME,,) {
		/*
		 * The compiler fills the constructor pointers table below, hence
		 * symbol __CTOR_LIST__ must be aligned on 4 byte boundary.
		 * To align with the C++ standard, the first element of the array
		 * contains the number of actual constructors. The last element is
		 * NULL.
		 */
		. = ALIGN(4);
		__CTOR_LIST__ = .;
		LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
		KEEP(*(SORT_BY_NAME(".ctors*")))
		LONG(0)
		__CTOR_END__ = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(init_array, (OPTIONAL),)
	{
		. = ALIGN(4);
		__init_array_start = .;
		KEEP(*(SORT_BY_NAME(".init_array*")))
		__init_array_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,) {
		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rom_end = .;
	__data_rom_start = ALIGN(4);	/* XIP imaged DATA ROM start addr */

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

#if defined(CONFIG_XIP)
	SECTION_AT_PROLOGUE(_DATA_SECTION_NAME,,,_DATA_IN_ROM) {
#else
	SECTION_PROLOGUE(_DATA_SECTION_NAME,,) {
#endif

/* when XIP, .text is in ROM, but vector table must be at start of .data */

		_image_ram_start = .;
		__data_ram_start = .;
		*(.data)
		*(".data.*")

#if CONFIG_SW_ISR_TABLE_DYNAMIC
		KEEP(*(.isr_irq*))

		/*The following sections maps the location of the different rows for
		the _sw_isr_table. Each row maps to an IRQ entry (handler, argument).*/
		/*In ARC architecture, IRQ 0-15 are reserved for the system and are not
		assignable by the user, for that reason the linker sections start
		on IRQ 16*/
		/* sections for IRQ16-19 */
		KEEP(*(SORT(.gnu.linkonce.isr_irq[1][6-9])))
		/* sections for IRQ20-99 */
		KEEP(*(SORT(.gnu.linkonce.isr_irq[2-9][0-9])))
		/* sections for IRQ100-999 */
		KEEP(*(SORT(.gnu.linkonce.isr_irq[1-9][0-9][0-9])))
#endif
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(initlevel, (OPTIONAL),)
	{
		DEVICE_INIT_SECTIONS()
	} GROUP_LINK_IN(RAMABLE_REGION)

SECTION_PROLOGUE (_k_task_list, (OPTIONAL),)
	{
		_k_task_list_start = .;
			*(._k_task_list.public.*)
			*(._k_task_list.private.*)
		_k_task_list_idle_start = .;
			*(._k_task_list.idle.*)
		KEEP(*(SORT_BY_NAME("._k_task_list*")))
		_k_task_list_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE (_k_task_ptr, (OPTIONAL),)
	{
		_k_task_ptr_start = .;
			*(._k_task_ptr.public.*)
			*(._k_task_ptr.private.*)
			*(._k_task_ptr.idle.*)
		KEEP(*(SORT_BY_NAME("._k_task_ptr*")))
		_k_task_ptr_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE (_k_pipe_ptr, (OPTIONAL),)
	{
		_k_pipe_ptr_start = .;
			*(._k_pipe_ptr.public.*)
			*(._k_pipe_ptr.private.*)
		KEEP(*(SORT_BY_NAME("._k_pipe_ptr*")))
		_k_pipe_ptr_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE (_k_mem_map_ptr, (OPTIONAL),)
	{
		_k_mem_map_ptr_start = .;
			*(._k_mem_map_ptr.public.*)
			*(._k_mem_map_ptr.private.*)
		KEEP(*(SORT_BY_NAME("._k_mem_map_ptr*")))
		_k_mem_map_ptr_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(_k_event_list, (OPTIONAL),)
	{
		_k_event_list_start = .;
			*(._k_event_list.event.*)
		KEEP(*(SORT_BY_NAME("._k_event_list*")))
		_k_event_list_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)

	__data_ram_end = .;

	SECTION_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),) {
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		__bss_start = .;
		*(.bss)
		*(".bss.*")
		COMMON_SYMBOLS
		/*
		 * BSP clears this memory in words only and doesn't clear any
		 * potential left over bytes.
		 */
		__bss_end = ALIGN(4);
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),) {
		/*
		 * This section is used for non-initialized objects that
		 * will not be cleared during the boot process.
		 */
		*(.noinit)
		*(".noinit.*")

	} GROUP_LINK_IN(RAMABLE_REGION)

	/* Define linker symbols */
	_image_ram_end = .;
	_end = .; /* end of image */

	GROUP_END(RAMABLE_REGION)

	/* Data Closely Coupled Memory (DCCM) */
	GROUP_START(DCCM)
	GROUP_END(DCCM)

	SECTION_PROLOGUE(initlevel_error, (OPTIONAL),)
	{
		DEVICE_INIT_UNDEFINED_SECTION()
	}
	ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")

	}
