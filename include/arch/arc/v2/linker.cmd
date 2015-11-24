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
#ifdef CONFIG_XIP
	#define ROMABLE_REGION FLASH
	#define RAMABLE_REGION SRAM
#else
	#define ROMABLE_REGION SRAM
	#define RAMABLE_REGION SRAM
#endif

#if defined(CONFIG_XIP)
	#define _DATA_IN_ROM __data_rom_start
#else
	#define _DATA_IN_ROM
#endif

MEMORY {
	FLASH (rx) : ORIGIN = FLASH_START, LENGTH = FLASH_SIZE
	SRAM  (wx) : ORIGIN = SRAM_START,  LENGTH = SRAM_SIZE*1K
	DCCM  (wx) : ORIGIN = DCCM_START,  LENGTH = DCCM_SIZE
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


		*(.text)
		*(".text.*")

		_image_text_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE (devconfig, (OPTIONAL),)
	{
		__devconfig_start = .;
		*(".devconfig.*")
		KEEP(*(SORT_BY_NAME(".devconfig*")))
		__devconfig_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

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

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,) {
		*(.rodata)
		*(".rodata.*")
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
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(initlevel, (OPTIONAL),)
	{
		DEVICE_INIT_SECTIONS()
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
	__bss_num_words = (__bss_end - __bss_start) >> 2;

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

#ifdef CONFIG_XIP
/*
 * Round up number of words for DATA section to ensure that XIP copies the
 * entire data section. XIP copy is done in words only, so there may be up
 * to 3 extra bytes copied in next section (BSS). At run time, the XIP copy
 * is done first followed by clearing the BSS section.
 */
__data_size = (__data_ram_end - __data_ram_start);
__data_num_words = (__data_size + 3) >> 2;

#endif
