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

/* Aliases for zephyr scripts */
#define RAMABLE_REGION    dram_seg
#define RODATA_REGION     dram_seg
#define ROMABLE_REGION    dram_seg

_bootloader_iram_loader_seg_start = BOOTLOADER_IRAM_LOADER_SEG_START;
_bootloader_dram_loader_seg_start = BOOTLOADER_DRAM_LOADER_SEG_START;
_bootloader_dram_seg_start = BOOTLOADER_DRAM_SEG_START;

MEMORY
{
  iram_loader_seg (RWX) : org = BOOTLOADER_IRAM_LOADER_SEG_START,
                          len = BOOTLOADER_IRAM_LOADER_SEG_LEN
  dram_loader_seg (RW) :  org = BOOTLOADER_DRAM_LOADER_SEG_START,
                          len = BOOTLOADER_DRAM_LOADER_SEG_LEN
  iram_seg (RWX) :        org = BOOTLOADER_IRAM_SEG_START,
                          len = BOOTLOADER_IRAM_SEG_LEN
  dram_seg (RW) :         org = BOOTLOADER_DRAM_SEG_START,
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

    *libarch__xtensa__core.a:xtensa_asm2_util.*(.literal .text .literal.* .text.*)
    *libarch__common.a:dynamic_isr.*(.literal .text .literal.* .text.*)
    *libarch__common.a:sw_isr_common.*(.literal .text .literal.* .text.*)

    *libapp.a:main.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libapp.a:flash_map_extended.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:flash_map_default.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:cbprintf_nano.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:cpu.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:mmu_hal.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:flash_map.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:esp_rom_sys.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:esp_rom_spiflash.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:esp_image_loader.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:bootloader_flash.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:cache_hal.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libkernel.a:device.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
    *libzephyr.a:printk.*(.literal .text .literal.* .text.* .iram0 .iram0.* .iram1 .iram1.*)
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

    /* CPU will try to prefetch up to 16 bytes of
     * of instructions. This means that any configuration (e.g. MMU, PMS) must allow
     * safe access to up to 16 bytes after the last real instruction, add
     * dummy bytes to ensure this
     */
    . += 16;

    _text_end = ABSOLUTE(.);
    _etext = .;
    . = ALIGN(4);
    _loader_text_end = ABSOLUTE(.);
    _iram_text_end = ABSOLUTE(.);
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
    *liblib__libc__common.a:abort.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *liblib__libc__picolibc.a:assert.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)
    *libc.a:libc_tinystdio_vfmprintf.*(.sbss .sbss.* .sbss2 .sbss2.* .bss .bss.*)

    . = ALIGN (8);
    _loader_bss_end = ABSOLUTE(.);
  } > dram_loader_seg

  .dram_loader.data :
  {
    . = ALIGN(16);

    *libapp.a:main.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libapp.a:flash_map_extended.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:flash_map.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:flash_map_default.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:cache_hal.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:mmu_hal.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_rom_sys.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_rom_spiflash.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_image_loader.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_flash.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:log.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libkernel.a:device.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:printk.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *liblib__libc__common.a:abort.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *liblib__libc__picolibc.a:assert.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libc.a:libc_tinystdio_vfmprintf.*(.data .data.* .rodata .rodata.* .sdata .sdata.* .sdata2 .sdata2.* .srodata .srodata.*)

    . = ALIGN(16);
    _dram_loader_end = ABSOLUTE(.);
  } > dram_loader_seg

  /* Ensure the dram_loader_seg is large enough */
  ASSERT((_dram_loader_end - ORIGIN(dram_loader_seg)) <= BOOTLOADER_DRAM_LOADER_SEG_LEN,
    "dram_loader_seg overflow: increase BOOTLOADER_DRAM_LOADER_SEG_LEN")

  .iram0.vectors : ALIGN(4)
  {
    /* Vectors go to IRAM */
    _init_start = ABSOLUTE(.);
    /* Vectors according to builds/RF-2015.2-win32/esp108_v1_2_s5_512int_2/config.html */
    . = 0x0;
    KEEP(*(.WindowVectors.text));
    . = 0x180;
    KEEP(*(.Level2InterruptVector.text));
    . = 0x1c0;
    KEEP(*(.Level3InterruptVector.text));
    . = 0x200;
    KEEP(*(.Level4InterruptVector.text));
    . = 0x240;
    KEEP(*(.Level5InterruptVector.text));
    . = 0x280;
    KEEP(*(.DebugExceptionVector.text));
    . = 0x2c0;
    KEEP(*(.NMIExceptionVector.text));
    . = 0x300;
    KEEP(*(.KernelExceptionVector.text));
    . = 0x340;
    KEEP(*(.UserExceptionVector.text));
    . = 0x3C0;
    KEEP(*(.DoubleExceptionVector.text));
    . = 0x400;
    _invalid_pc_placeholder = ABSOLUTE(.);
    *(.*Vector.literal)

    *(.UserEnter.literal);
    *(.UserEnter.text);
    . = ALIGN (16);
    *(.entry.text)
    *(.init.literal)
    *(.init)
    . = ALIGN (4);
    _init_end = ABSOLUTE(.);

    /* This goes here, not at top of linker script, so addr2line finds it last,
     * and uses it in preference to the first symbol in IRAM
     */
    _iram_start = ABSOLUTE(.);
  } > iram_seg

  .iram0.text :
  {
    . = ALIGN(4);

    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)

    *(.literal .text .literal.* .text.*)
    . = ALIGN(4);

    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
    . = ALIGN(4);
  } > iram_seg

  .dram0.data : ALIGN(16)
  {
    . = ALIGN(4);
    __data_start = ABSOLUTE(.);

    #include <snippets-rodata.ld>

    . = ALIGN(4);
    #include <snippets-rwdata.ld>
    . = ALIGN(4);

    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    *libzephyr.a:rtc_clk.*(.rodata .rodata.*)

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
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

    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS_ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);
    __rodata_region_end = ABSOLUTE(.);
    /* Literals are also RO data. */
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
    . = ALIGN(4);
    *(.rodata_wlog)
    *(.rodata_wlog*)
    _thread_local_end = ABSOLUTE(.);
    . = ALIGN(4);
  } > dram_seg

  #include <zephyr/linker/common-rom/common-rom-kernel-devices.ld>
  #include <zephyr/linker/common-rom/common-rom-debug.ld>
  #include <zephyr/linker/common-rom/common-rom-misc.ld>
  #include <snippets-sections.ld>

  #include <zephyr/linker/cplusplus-rom.ld>
  #include <snippets-data-sections.ld>
  #include <zephyr/linker/common-ram.ld>
  #include <snippets-ram-sections.ld>
  #include <zephyr/linker/cplusplus-ram.ld>
  #include <zephyr/linker/common-rom/common-rom-logging.ld>

  .noinit (NOLOAD):
  {
    . = ALIGN(8);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(8) ;
  } > dram_seg

  /* Shared RAM */
  .bss (NOLOAD):
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.); /* required by bluetooth library */
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
    _end = ABSOLUTE(.);
  } > dram_seg

  ASSERT(((__bss_end - ORIGIN(dram_seg)) <= LENGTH(dram_seg)), "DRAM segment data does not fit.")

#include <zephyr/linker/debug-sections.ld>

  .xtensa.info  0 :  { *(.xtensa.info) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .xt.profile_range 0 :
  {
    KEEP (*(.xt.profile_range))
    KEEP (*(.gnu.linkonce.profile_range.*))
  }
  .xt.profile_ranges 0 :
  {
    KEEP (*(.xt.profile_ranges))
    KEEP (*(.gnu.linkonce.xt.profile_ranges.*))
  }
  .xt.profile_files 0 :
  {
    KEEP (*(.xt.profile_files))
    KEEP (*(.gnu.linkonce.xt.profile_files.*))
  }

#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif
}
