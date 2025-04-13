/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#include "memory.h"

#define ALIGN_DOWN(SIZE, AL)   (SIZE & ~(AL - 1))
/* Ensure the end where the shared memory starts is aligned to 8 bytes
   if updating this also update the same in ulp_lp_core_memory_shared.c
 */
#define ALIGNED_COPROC_MEM ALIGN_DOWN(ULP_COPROC_RESERVE_MEM, 0x8)

#define RODATA_REGION      ram
#define RAMABLE_REGION     ram
#define ROMABLE_REGION     ram

/* User available memory segments  */
_aligned_coproc_mem = ALIGNED_COPROC_MEM;
_vector_table_org = LPSRAM_IRAM_START;
_vector_table_len = 0x80;
_ram_org = _vector_table_org + _vector_table_len;
_ram_len = _aligned_coproc_mem - _vector_table_len - ULP_SHARED_MEM;
_shared_mem_org = _ram_org + _ram_len;
_shared_mem_len = ULP_SHARED_MEM;

ENTRY(reset_vector)

MEMORY
{
    /*first 128byte for exception/interrupt vectors*/
    vector_table(RX) :   ORIGIN = _vector_table_org , LENGTH = _vector_table_len
    ram(RWX) :           ORIGIN = _ram_org, LENGTH = _ram_len
    shared_mem_ram(RW) : ORIGIN = _shared_mem_org, LENGTH = _shared_mem_len
}

SECTIONS
{
    .vector.text :
    {
        __mtvec_base = .;
        KEEP (*(.init.vector .init.vector.*))
    } > vector_table

    . = ORIGIN(ram);

    .text ALIGN(4):
    {
        *(.text.vectors) /* Default reset vector must link to offset 0x80 */
        __text_region_start = ABSOLUTE(.);
        *(.text)
        *(.text*)
        *(.iram1)
        *(.iram1.*)
        __text_region_end = ABSOLUTE(.);
    } >ram

  #include <zephyr/linker/rel-sections.ld>

    .rodata ALIGN(4):
    {
        __rodata_region_start = ABSOLUTE(.);
        *(.rodata .srodata)
        *(.rodata* .srodata*)
        __rodata_region_end = .;

    } > ram

#include <zephyr/linker/common-rom/common-rom-kernel-devices.ld>

    .rodata.end ALIGN(4):
    {
        _rodata_reserved_end = ABSOLUTE(.);
    } > ram

    .data ALIGN(4):
    {
        _image_ram_start = ABSOLUTE(.);
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)

    } > ram

  #include <snippets-data-sections.ld>
  #include <zephyr/linker/common-ram.ld>
  #include <snippets-ram-sections.ld>

    .data.end ALIGN(4):
    {
        _image_ram_end = ABSOLUTE(.);
    } > ram

    .data.noinit (NOLOAD):
    {
        . = ALIGN(4);
        *(.noinit)
        *(.noinit.*)
        . = ALIGN(4);
    } > ram

    .bss ALIGN(4) :
    {
        __bss_start = .;
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        PROVIDE(end = .);
        __bss_end = .;
        _heap_sentry = .;
    } >ram

#include <zephyr/linker/ram-end.ld>

    __stack_top = ORIGIN(ram) + LENGTH(ram);

#include <zephyr/linker/debug-sections.ld>
    SECTION_PROLOGUE(.riscv.attributes, 0,)
    {
    KEEP(*(.riscv.attributes))
    KEEP(*(.gnu.attributes))
    }

    . = ORIGIN(shared_mem_ram);
    .shared_mem (ALIGN(4)) :
    {
        KEEP(*(.shared_mem))
    } > shared_mem_ram
}
