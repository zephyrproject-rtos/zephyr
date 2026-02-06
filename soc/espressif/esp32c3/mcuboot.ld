/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#include "memory.h"

/* Disable all romable LMA */
#undef GROUP_DATA_LINK_IN
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion

#define RAMABLE_REGION dram_seg
#define RODATA_REGION  dram_seg
#define ROMABLE_REGION dram_seg

_bootloader_iram_loader_seg_start = BOOTLOADER_IRAM_LOADER_SEG_START;
_bootloader_dram_loader_seg_start = BOOTLOADER_DRAM_LOADER_SEG_START;

/* Global symbols required for espressif hal build */
MEMORY
{
  iram_loader_seg (RX) : org = BOOTLOADER_IRAM_LOADER_SEG_START,
                         len = BOOTLOADER_IRAM_LOADER_SEG_LEN
  dram_loader_seg (RW) : org = BOOTLOADER_DRAM_LOADER_SEG_START,
                         len = BOOTLOADER_DRAM_LOADER_SEG_LEN
  iram_seg        (RX) : org = BOOTLOADER_IRAM_SEG_START,
                         len = BOOTLOADER_IRAM_SEG_LEN
  dram_seg        (RW) : org = BOOTLOADER_DRAM_SEG_START,
                         len = BOOTLOADER_DRAM_SEG_LEN

#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif
}

/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
  .iram_loader.text :
  {
    . = ALIGN (16);
    _loader_text_start = ABSOLUTE(.);
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)

    *libapp.a:main.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libapp.a:flash_map_extended.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:flash_map.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:flash_map_default.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:cbprintf_nano.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:cpu.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:mmu_hal.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:flash_map.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:esp_rom_spiflash.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libkernel.a:device.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:esp_image_loader.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:bootloader_flash.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:cache_hal.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libkernel.a:device.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:printk.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:systimer_hal.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libdrivers__timer.a:esp32_sys_timer.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *liblib__libc__common.a:abort.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *liblib__libc__picolibc.a:assert.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libc.a:libc_tinystdio_vfmprintf.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)

    *(.literal.esp_intr_disable .literal.esp_intr_disable.* .text.esp_intr_disable .text.esp_intr_disable.*)
    *(.literal.default_intr_handler .text.default_intr_handler .iram1.*.default_intr_handler)
    *(.literal.esp_log_timestamp .text.esp_log_timestamp)
    *(.literal.esp_log_early_timestamp .text.esp_log_early_timestamp)
    *(.literal.esp_system_abort .text.esp_system_abort)

    *(.fini.literal)
    *(.fini)
    *(.gnu.version)
    _loader_text_end = ABSOLUTE(.);
    _iram_end = ABSOLUTE(.);
  } > iram_loader_seg

  /* Ensure the iram_loader_seg is large enough */
  ASSERT((_loader_text_end - ORIGIN(iram_loader_seg)) <= BOOTLOADER_IRAM_LOADER_SEG_LEN,
    "iram_loader_seg overflow: increase BOOTLOADER_IRAM_LOADER_SEG_LEN")

  .dram_loader.bss (NOLOAD) :
  {
    . = ALIGN (8);
    _dram_loader_start = ABSOLUTE(.);
    _loader_bss_start = ABSOLUTE(.);

    *libapp.a:main.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libapp.a:flash_map_extended.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:flash_map.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:flash_map_default.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:cache_hal.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:mmu_hal.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:esp_rom_sys.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:esp_rom_spiflash.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:esp_image_loader.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:bootloader_flash.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:log.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libkernel.a:device.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:printk.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libzephyr.a:systimer_hal.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libdrivers__timer.a:esp32_sys_timer.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *liblib__libc__common.a:abort.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *liblib__libc__picolibc.a:assert.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libc.a:libc_tinystdio_vfmprintf.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)

    . = ALIGN (8);
    _loader_bss_end = ABSOLUTE(.);
  } > dram_loader_seg

  .dram_loader.data :
  {
    . = ALIGN(16);

    *libapp.a:main.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libapp.a:flash_map_extended.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:flash_map.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:flash_map_default.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:cache_hal.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:mmu_hal.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:esp_rom_sys.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:esp_rom_spiflash.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:esp_image_loader.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:bootloader_flash.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:log.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libkernel.a:device.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:printk.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libzephyr.a:systimer_hal.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *liblib__libc__common.a:abort.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *liblib__libc__picolibc.a:assert.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libc.a:libc_tinystdio_vfmprintf.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)
    *libdrivers__timer.a:esp32_sys_timer.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.* .dram*)

    . = ALIGN(16);
    _dram_loader_end = ABSOLUTE(.);
  } > dram_loader_seg

  /* Ensure the dram_loader_seg is large enough */
  ASSERT((_dram_loader_end - ORIGIN(dram_loader_seg)) <= BOOTLOADER_DRAM_LOADER_SEG_LEN,
    "dram_loader_seg overflow: increase BOOTLOADER_DRAM_LOADER_SEG_LEN")

  .iram0.text :
  {
    /* Vectors go to IRAM */
    _iram_start = ABSOLUTE(.);
    _init_start = ABSOLUTE(.);
    __text_region_start = ABSOLUTE(.);

    KEEP(*(.exception_vectors.text));
    . = ALIGN(256);

    _invalid_pc_placeholder = ABSOLUTE(.);

    _iram_text_start = ABSOLUTE(.);

    KEEP(*(.exception.entry*)); /* contains _isr_wrapper */
    *(.exception.other*)
    . = ALIGN(4);

    *(.entry.text)
    *(.init.literal)
    *(.init)
    . = ALIGN(4);
    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)

    /* C3 memprot requires 512 B alignment for split lines */
    . = ALIGN (16);
    _init_end = ABSOLUTE(.);
    . = ALIGN(16);
    *(.iram.data)
    *(.iram.data*)
    . = ALIGN(16);
    *(.iram.bss)
    *(.iram.bss*)

    . = ALIGN(16);

    *(.literal .text .literal.* .text.*)
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.fini.literal)
    *(.fini)
    *(.gnu.version)

    /* CPU will try to prefetch up to 16 bytes of
     * of instructions. This means that any configuration (e.g. MMU, PMS) must allow
     * safe access to up to 16 bytes after the last real instruction, add
     * dummy bytes to ensure this
     */
    . += 16;

    _text_end = ABSOLUTE(.);
    __text_region_end = ABSOLUTE(.);
    _etext = .;

    /* Similar to _iram_start, this symbol goes here so it is
     * resolved by addr2line in preference to the first symbol in
     * the flash.text segment.
     */
    _flash_cache_start = ABSOLUTE(0);
  } > iram_seg

  .dram0.data :
  {
    . = ALIGN(4);
    __data_start = ABSOLUTE(.);
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    *(.data1)
#ifdef CONFIG_RISCV_GP
    __global_pointer$ = . + 0x800;
#endif /* CONFIG_RISCV_GP */
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    *libzephyr.a:mmu_hal.*(.rodata .rodata.*)
    *libzephyr.a:rtc_clk.*(.rodata .rodata.*)
    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    . = ALIGN(4);

    #include <snippets-rwdata.ld>
    . = ALIGN(4);

    *(.rodata_desc .rodata_desc.*)
    *(.rodata_custom_desc .rodata_custom_desc.*)

    . = ALIGN(4);
    #include <snippets-rodata.ld>
    . = ALIGN(4);

    *(.rodata)
    *(.rodata.*)
    *(.irom1.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE_ = ABSOLUTE(.);
    *(.xt_except_table)
    *(.gcc_except_table .gcc_except_table.*)
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    . = (. + 3) & ~ 3;
    __eh_frame = ABSOLUTE(.);
    KEEP(*(.eh_frame))
    . = (. + 7) & ~ 3;

    /* C++ exception handlers table: */
    __XT_EXCEPTION_DESCS_ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    __rodata_region_end = .;
    _rodata_end = ABSOLUTE(.);
    /* Literals are also RO data. */
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
    . = ALIGN(4);
    _thread_local_start = ABSOLUTE(.);
    *(.tdata)
    *(.tdata.*)
    *(.tbss)
    *(.tbss.*)
    *(.srodata)
    *(.srodata.*)
    *(.rodata)
    *(.rodata.*)
    *(.rodata_wlog)
    *(.rodata_wlog*)
    _thread_local_end = ABSOLUTE(.);
    /* _rodata_reserved_end = ABSOLUTE(.); */
    . = ALIGN(4);
  } > dram_seg

  #include <zephyr/linker/common-rom/common-rom-init.ld>
  #include <zephyr/linker/common-rom/common-rom-kernel-devices.ld>
  #include <zephyr/linker/common-rom/common-rom-debug.ld>
  #include <zephyr/linker/common-rom/common-rom-misc.ld>
  #include <snippets-sections.ld>

  #include <zephyr/linker/cplusplus-rom.ld>
  #include <zephyr/linker/thread-local-storage.ld>
  #include <snippets-data-sections.ld>
  #include <zephyr/linker/common-ram.ld>
  #include <snippets-ram-sections.ld>
  #include <zephyr/linker/cplusplus-ram.ld>

  #include <zephyr/linker/common-rom/common-rom-logging.ld>

  .noinit (NOLOAD):
  {
    . = ALIGN(4);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(4);
  } > dram_seg

  /* Shared RAM */
  .bss (NOLOAD):
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    __bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(.bss.*)
    *(.share.mem)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN (8);
    __bss_end = ABSOLUTE(.);
    _bss_end = ABSOLUTE(.);
  } > dram_seg

  /* linker rel sections*/
  #include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_GEN_ISR_TABLES
  #include <zephyr/linker/intlist.ld>
#endif

#include <zephyr/linker/debug-sections.ld>
  /DISCARD/ : { *(.note.GNU-stack) }

  SECTION_PROLOGUE(.riscv.attributes, 0,)
    {
    KEEP(*(.riscv.attributes))
    KEEP(*(.gnu.attributes))
    }
}
