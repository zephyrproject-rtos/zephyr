/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
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

ENTRY(reset_vector)

MEMORY
{
    /*first 128byte for exception/interrupt vectors*/
    vector_table(RX) :   ORIGIN = LPSRAM_IRAM_START , LENGTH = 0x80
    ram(RWX) :           ORIGIN = LPSRAM_IRAM_START + 0x80, LENGTH = ALIGNED_COPROC_MEM - 0x80 - ULP_SHARED_MEM
    shared_mem_ram(RW) : ORIGIN = LPSRAM_IRAM_START + ALIGNED_COPROC_MEM - ULP_SHARED_MEM, LENGTH = ULP_SHARED_MEM
}

SECTIONS
{
    .vector.text :
    {
        /*exception/interrupt vectors*/
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
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        PROVIDE(end = .);
        _heap_sentry = .;
    } >ram

#include <zephyr/linker/ram-end.ld>

    __stack_top = ORIGIN(ram) + LENGTH(ram);

    . = ORIGIN(shared_mem_ram);
    .shared_mem (ALIGN(4)) :
    {
        KEEP(*(.shared_mem))
    } > shared_mem_ram

  #include <zephyr/linker/debug-sections.ld>
  SECTION_PROLOGUE(.riscv.attributes, 0,)
    {
    KEEP(*(.riscv.attributes))
    KEEP(*(.gnu.attributes))
    }

}
