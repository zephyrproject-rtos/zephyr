/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the esp32c3 platform.
 */

#include <devicetree.h>
#include <autoconf.h>
#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define ROMABLE_REGION drom0_0_seg :drom0_0_phdr
#define RAMABLE_REGION dram0_0_seg :dram0_0_phdr
#define IRAM_REGION  iram0_0_seg :iram0_0_phdr
#define FLASH_CODE_REGION irom0_0_seg :irom0_0_phdr

#define SRAM_IRAM_START     0x4037C000
#define SRAM_DRAM_START     0x3FC7C000
#define ICACHE_SIZE         0x4000 /* ICache size is fixed to 16KB on ESP32-C3 */
#define I_D_SRAM_OFFSET     (SRAM_IRAM_START - SRAM_DRAM_START)
#define SRAM_DRAM_END       0x403D0000 - I_D_SRAM_OFFSET  /* 2nd stage bootloader iram_loader_seg start address */
#define SRAM_IRAM_ORG       (SRAM_IRAM_START + ICACHE_SIZE)
#define SRAM_DRAM_ORG       (SRAM_DRAM_START + ICACHE_SIZE)
#define I_D_SRAM_SIZE       SRAM_DRAM_END - SRAM_DRAM_ORG

/* Global symbols required for espressif hal build */
MEMORY
{
  iram0_0_seg(RX): org = SRAM_IRAM_ORG, len = I_D_SRAM_SIZE
  irom0_0_seg(RX): org = 0x42000020, len = 0x8000000-0x20

  drom0_0_seg (R) : org = 0x3C000020, len = 0x8000000-0x20
  dram0_0_seg(RW): org = SRAM_DRAM_ORG, len = I_D_SRAM_SIZE

  rtc_iram_seg(RWX): org = 0x50000000, len = 0x2000

#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif
}

PHDRS
{
  drom0_0_phdr PT_LOAD;
  dram0_0_phdr PT_LOAD;
  iram0_0_phdr PT_LOAD;
  irom0_0_phdr PT_LOAD;
}

/*  Default entry point:  */
ENTRY(__start)

_rom_store_table = 0;

SECTIONS
{

#include <linker/rel-sections.ld>

  .rtc.text :
  {
    . = ALIGN(4);
    *(.rtc.literal .rtc.text)
    *rtc_wake_stub*.o(.literal .text .literal.* .text.*)
  } > rtc_iram_seg

  /* This section is required to skip rtc.text area because the text and
   * data segments reflect the same address space on different buses.
   */
  .rtc.dummy (NOLOAD):
  {
    . = SIZEOF(.rtc.text);
  } > rtc_iram_seg

  .rtc.data :
  {
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data)
    *(.rtc.rodata)
    *rtc_wake_stub*.o(.data .rodata .data.* .rodata.* .bss .bss.*)
    _rtc_data_end = ABSOLUTE(.);
  } > rtc_iram_seg

  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *rtc_wake_stub*.o(.bss .bss.*)
    *rtc_wake_stub*.o(COMMON)
    _rtc_bss_end = ABSOLUTE(.);
  } > rtc_iram_seg

  .iram0.text : ALIGN(4)
  {
    /* Vectors go to IRAM */
    _iram_start = ABSOLUTE(.);
    _iram_text_start = ABSOLUTE(.);
    _init_start = ABSOLUTE(.);
    KEEP(*(.exception_vectors.text));
    . = ALIGN(256);

    KEEP(*(.exception.entry*)); /* contains __irq_wrapper */
    *(.exception.other*)
    . = ALIGN(4);

    . = ALIGN (4);
    *(.entry.text)
    *(.init.literal)
    *(.init)
    . = ALIGN(4);
    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    *libkernel.a:(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_noos.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp32c3_sys_timer.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_core.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cbprintf_complete.*(.literal .text .literal.* .text.*)
    *libzephyr.a:printk.*(.literal.printk .literal.vprintk .literal.char_out .text.printk .text.vprintk .text.char_out)
    *libzephyr.a:log_msg.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_list.*(.literal .text .literal.* .text.*)
    *libzephyr.a:uart_console.*(.literal.console_out .text.console_out)
    *libzephyr.a:log_output.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_backend_uart.*(.literal .text .literal.* .text.*)
    *liblib__libc__minimal.a:string.*(.literal .text .literal.* .text.*)
    *libgcov.a:(.literal .text .literal.* .text.*)
    . = ALIGN(4);
    _init_end = ABSOLUTE(.);
    _iram_text_end = ABSOLUTE(.);   *(.srodata)
    _iram_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)

  /**
   * This section is required to skip .iram0.text area because iram0_0_seg and
   * dram0_0_seg reflect the same address space on different buses.
   */
  .dram0.dummy (NOLOAD):
  {
    . = ORIGIN(dram0_0_seg) + _iram_end - _iram_start;
  } > dram0_0_seg

#include <linker/common-ram.ld>

  .dram0.data :
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    *(.data1)
    __global_pointer$ = . + 0x800;
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    *libkernel.a:fatal.*(.rodata .rodata.*)
    *libkernel.a:init.*(.rodata .rodata.*)
    *libzephyr.a:cbprintf_complete*(.rodata .rodata.*)
    *libzephyr.a:log_core.*(.rodata .rodata.*)
    *libzephyr.a:log_backend_uart.*(.rodata .rodata.*)
    *libzephyr.a:log_output.*(.rodata .rodata.*)
   . = ALIGN(4);
    __esp_log_const_start = .;
    KEEP(*(SORT(.log_const_*)));
    __esp_log_const_end = .;

    . = ALIGN(4);
    __esp_log_backends_start = .;
    KEEP(*("._log_backend.*"));
    __esp_log_backends_end = .;

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    _data_end = ABSOLUTE(.);
    . = ALIGN(4);
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Shared RAM */
  dram0.bss (NOLOAD) :
  {
    . = ALIGN (8);
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
  } GROUP_LINK_IN(RAMABLE_REGION)

  SECTION_DATA_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD),)
  {
    . = ALIGN (8);
    *(.noinit)
    *(".noinit.*")
    . = ALIGN (8);
    _heap_start = ABSOLUTE(.);
  } GROUP_LINK_IN(RAMABLE_REGION)

  .flash.text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);

    *(.literal .text .literal.* .text.*)
    _text_end = ABSOLUTE(.);
    _etext = .;
    _flash_cache_start = ABSOLUTE(0);
  } GROUP_LINK_IN(FLASH_CODE_REGION)

  .flash_rodata_dummy (NOLOAD):
  {
    . = SIZEOF(.flash.text);
    . = ALIGN(0x10000) + 0x20;
    _rodata_reserved_start = .;
  } GROUP_LINK_IN(ROMABLE_REGION)

  .flash.rodata : ALIGN(0x10)
  {
    _rodata_start = ABSOLUTE(.);

    *(.rodata_desc .rodata_desc.*)               /* Should be the first.  App version info.        DO NOT PUT ANYTHING BEFORE IT! */
    *(.rodata_custom_desc .rodata_custom_desc.*) /* Should be the second. Custom app version info. DO NOT PUT ANYTHING BEFORE IT! */
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
    /*
     * C++ constructor and destructor tables
     * Don't include anything from crtbegin.o or crtend.o, as IDF doesn't use toolchain crt.
     *
     * RISC-V gcc is configured with --enable-initfini-array so it emits an .init_array section instead.
     * But the init_priority sections will be sorted for iteration in ascending order during startup.
     * The rest of the init_array sections is sorted for iteration in descending order during startup, however.
     * Hence a different section is generated for the init_priority functions which is iterated in
     * ascending order during startup. The corresponding code can be found in startup.c.
     */
    __init_priority_array_start = ABSOLUTE(.);
    KEEP (*(EXCLUDE_FILE (*crtend.* *crtbegin.*) .init_array.*))
    __init_priority_array_end = ABSOLUTE(.);
    __init_array_start = ABSOLUTE(.);
    KEEP (*(EXCLUDE_FILE (*crtend.* *crtbegin.*) .init_array))
    __init_array_end = ABSOLUTE(.);
    KEEP (*crtbegin.*(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.*) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /* C++ exception handlers table: */
    __XT_EXCEPTION_DESCS_ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    /* Addresses of memory regions reserved via SOC_RESERVE_MEMORY_REGION() */
    soc_reserved_memory_region_start = ABSOLUTE(.);
    KEEP (*(.reserved_memory_address))
    soc_reserved_memory_region_end = ABSOLUTE(.);
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
    *(".srodata.*")
    *(.rodata)
    *(.rodata.*)
    _thread_local_end = ABSOLUTE(.);
    _rodata_reserved_end = ABSOLUTE(.);
    . = ALIGN(4);
  } GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/common-rom.ld>

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif

#include <linker/debug-sections.ld>
  /DISCARD/ : { *(.note.GNU-stack) }

  SECTION_PROLOGUE(.riscv.attributes, 0,)
	{
    KEEP(*(.riscv.attributes))
    KEEP(*(.gnu.attributes))
	}
}
