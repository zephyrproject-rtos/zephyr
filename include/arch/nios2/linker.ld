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

#define ROMABLE_REGION FLASH
#define RAMABLE_REGION SRAM

#define ROM_ADDR CONFIG_FLASH_BASE_ADDRESS
#define ROM_SIZE CONFIG_FLASH_SIZE * 1K

#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#define RAM_SIZE CONFIG_SRAM_SIZE * 1K

#define _DATA_IN_ROM __data_rom_start

MEMORY
    {
    FLASH (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
    SRAM  (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
    }

SECTIONS
    {
    GROUP_START(ROMABLE_REGION)
    _image_rom_start = CONFIG_FLASH_BASE_ADDRESS;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
        {

        _image_text_start = .;
        *(.text)
        *(".text.*")
        *(.gnu.linkonce.t.*)
        } GROUP_LINK_IN(ROMABLE_REGION)

    _image_text_end = .;

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

    _gp = ALIGN(16) + 0x7ff0;
    PROVIDE(gp = _gp);

    SECTION_AT_PROLOGUE(_DATA_SECTION_NAME,,,_DATA_IN_ROM)
        {
        _image_ram_start = .;
        __data_ram_start = .;
        *(.data)
        *(".data.*")
        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(initlevel, (OPTIONAL),)
        {
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

    __data_ram_end = .;

    SECTION_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
        {
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
    __bss_num_words = (__bss_end - __bss_start) >> 2;

    GROUP_END(RAMABLE_REGION)

    }

/*
 * Round up number of words for DATA section to ensure that XIP copies the
 * entire data section. XIP copy is done in words only, so there may be up
 * to 3 extra bytes copied in next section (BSS). At run time, the XIP copy
 * is done first followed by clearing the BSS section.
 */
__data_size = (__data_ram_end - __data_ram_start);
__data_num_words = (__data_size + 3) >> 2;

