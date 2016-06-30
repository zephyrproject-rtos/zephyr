/*
 * Copyright (c) 2016 Intel Corporation
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
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Nios II platform
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <sections.h>

#include <linker-defs.h>
#include <linker-tool.h>

/* These sections are specific to this CPU */
#define _EXCEPTION_SECTION_NAME exceptions
#define _RESET_SECTION_NAME reset

/* This linker script requires the following macros to be defined in the
 * SOC-specfic linker script. All of these values can be found defined
 * in system.h for CPU configurations that can generate a HAL.
 *
 * _RESET_VECTOR     CPU entry point at boot
 * _EXC_VECTOR       General exception vector
 * _ROM_ADDR         Beginning of flash memory
 * _ROM_SIZE         Size in bytes of flash memory
 * _RAM_ADDR         Beginning of RAM
 * _RAM_SIZE         Size of RAM in bytes
 *
 * For now we support two scenarios:
 *
 * 1. Non-XIP systems where the reset vector is at the beginning of RAM
 * with the exception vector 0x20 bytes after it.
 * 2. XIP systems where the reset vector is at the beginning of ROM and
 * the exception vector is in RAM
 */


#ifdef CONFIG_XIP
	#define ROMABLE_REGION FLASH
	#define RAMABLE_REGION SRAM
#else
	#define ROMABLE_REGION SRAM
	#define RAMABLE_REGION SRAM
#endif

#ifdef CONFIG_XIP

ASSERT(_RESET_VECTOR == _ROM_ADDR, "Reset vector not at beginning of ROM!")

MEMORY
    {
    RESET (rx) : ORIGIN = _RESET_VECTOR, LENGTH = 0x20
    FLASH (rx) : ORIGIN = _RESET_VECTOR + 0x20 , LENGTH = (_ROM_SIZE - 0x20)
    SRAM  (wx) : ORIGIN = _EXC_VECTOR, LENGTH = _RAM_SIZE - (_EXC_VECTOR - _RAM_ADDR)
    }

#else

MEMORY
    {
    RESET (wx) : ORIGIN = _RESET_VECTOR, LENGTH = 0x20
    SRAM  (wx) : ORIGIN = _EXC_VECTOR, LENGTH = _RAM_SIZE - (_EXC_VECTOR - _RAM_ADDR)
}
#endif

SECTIONS
    {
    GROUP_START(ROMABLE_REGION)
    _image_rom_start = _ROM_ADDR;

    SECTION_PROLOGUE(_RESET_SECTION_NAME,,)
    {
        KEEP(*(.reset.*))
    } GROUP_LINK_IN(RESET)

#ifndef CONFIG_XIP
    SECTION_PROLOGUE(_EXCEPTION_SECTION_NAME,,)
        {
            KEEP(*(".exception.entry.*"))
            *(".exception.other.*")

        } GROUP_LINK_IN(ROMABLE_REGION)
#endif

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
        {
        /* XXX If ALT_CPU_RESET_ADDR is not the same as _ROM_ADDR
         * we are going to waste flash space? */
        . = ALT_CPU_RESET_ADDR;

        _image_text_start = .;
        *(.text)
        *(".text.*")
        *(.gnu.linkonce.t.*)
        } GROUP_LINK_IN(ROMABLE_REGION)

    _image_text_end = .;

#if defined(CONFIG_GP_ALL_DATA)
        _gp = ABSOLUTE(. + 0x8000);
        PROVIDE(gp = _gp);
#endif

#ifdef CONFIG_CPLUSPLUS
    SECTION_PROLOGUE(_CTOR_SECTION_NAME, ,)
        {
        /*
         * The compiler fills the constructor pointers table below, hence symbol
         * __CTOR_LIST__ must be aligned on 4 byte boundary.
         * To align with the C++ standard, the first elment of the array
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

    SECTION_PROLOGUE(devconfig, (OPTIONAL),)
        {
                __devconfig_start = .;
                *(".devconfig.*")
                KEEP(*(SORT_BY_NAME(".devconfig*")))
                __devconfig_end = .;
        } GROUP_LINK_IN(ROMABLE_REGION)

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
        {
        *(.rodata)
        *(".rodata.*")
        *(.gnu.linkonce.r.*)
        } GROUP_LINK_IN(ROMABLE_REGION)

    _image_rom_end = .;
    __data_rom_start = ALIGN(4);    /* XIP imaged DATA ROM start addr */

    GROUP_END(ROMABLE_REGION)

    GROUP_START(RAMABLE_REGION)

#ifdef CONFIG_XIP
    /* Altera strongly recommends keeping exception entry code in RAM
     * even on XIP systems
     *
     * This is code not data, but we need this copied just like XIP data
     */

    SECTION_AT_PROLOGUE(_EXCEPTION_SECTION_NAME,,, __data_rom_start)
        {
            _image_ram_start = .;
            __data_ram_start = .;

            KEEP(*(".exception.entry.*"))
            *(".exception.other.*")

        } GROUP_LINK_IN(RAMABLE_REGION)
#endif

    SECTION_PROLOGUE(initlevel, (OPTIONAL),)
        {
#ifndef CONFIG_XIP
        _image_ram_start = .;
#endif
                DEVICE_INIT_SECTIONS()
        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(_k_task_list, (OPTIONAL),)
        {
                _k_task_list_start = .;
                        *(._k_task_list.public.*)
                        *(._k_task_list.private.*)
                _k_task_list_idle_start = .;
                        *(._k_task_list.idle.*)
                KEEP(*(SORT_BY_NAME("._k_task_list*")))
                _k_task_list_end = .;
        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(_k_task_ptr, (OPTIONAL),)
        {
                _k_task_ptr_start = .;
                        *(._k_task_ptr.public.*)
                        *(._k_task_ptr.private.*)
                        *(._k_task_ptr.idle.*)
                KEEP(*(SORT_BY_NAME("._k_task_ptr*")))
                _k_task_ptr_end = .;
        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(_k_pipe_ptr, (OPTIONAL),)
        {
                _k_pipe_ptr_start = .;
                        *(._k_pipe_ptr.public.*)
                        *(._k_pipe_ptr.private.*)
                KEEP(*(SORT_BY_NAME("._k_pipe_ptr*")))
                _k_pipe_ptr_end = .;
        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(_k_mem_map_ptr, (OPTIONAL),)
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

    SECTION_PROLOGUE(_DATA_SECTION_NAME,,)
        {

        KEEP(*(.isr_irq*))

        /* sections for IRQ0-9 */
        KEEP(*(SORT(.gnu.linkonce.isr_irq[0-9])))

        /* sections for IRQ10-99 */
        KEEP(*(SORT(.gnu.linkonce.isr_irq[0-9][0-9])))

        /* sections for IRQ100-999 */
        KEEP(*(SORT(.gnu.linkonce.isr_irq[0-9][0-9][0-9])))

        *(.data)
        *(".data.*")

        /* the Nios2 architecture only has 16-bit signed immediate offsets in
         * the instructions, so accessing a general address requires typically
         * three instructions – basically, two for the two halves of the 32-bit
         * address, and one to merge them – but if we can put the most commonly
         * accessed globals in a special 64K span of memory addressed by the GP
         * register, then we can access those values in a single instruction,
         * saving both codespace and runtime.
         *
         * Since these immediate offsets are signed, place gp 0x8000 past the
         * beginning of .sdata so that we can use both positive and negative
         * offsets.
         */
#if defined(CONFIG_GP_LOCAL) || defined(CONFIG_GP_GLOBAL)
        _gp = ABSOLUTE(. + 0x8000);
        PROVIDE(gp = _gp);
#endif

        *(.sdata .sdata.* .gnu.linkonce.s.*)
        *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)

        } GROUP_LINK_IN(RAMABLE_REGION)

    __data_ram_end = .;

    SECTION_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
        {
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
        __bss_start = .;
        *(.sbss)
        *(".sbss.*")
        *(.bss)
        *(".bss.*")
        COMMON_SYMBOLS
        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
                 */
        __bss_end = ALIGN(4);
        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
        {
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

    }

