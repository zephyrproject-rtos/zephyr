/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Xtensa platform.
 */

#include <xtensa/config/core-isa.h>

#include <zephyr/linker/sections.h>

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define RAMABLE_REGION RAM :sram0_phdr
#define ROMABLE_REGION rom0_seg :rom0_phdr

#ifdef CONFIG_MMU
#define MMU_PAGE_ALIGN . = ALIGN(CONFIG_MMU_PAGE_SIZE);
#define HDR_MMU_PAGE_ALIGN ALIGN(CONFIG_MMU_PAGE_SIZE)
#define HDR_4K_OR_MMU_PAGE_ALIGN ALIGN(CONFIG_MMU_PAGE_SIZE)
#define LAST_RAM_ALIGN MMU_PAGE_ALIGN
#else
#define MMU_PAGE_ALIGN . = ALIGN(4);
#define HDR_MMU_PAGE_ALIGN ALIGN(4)
#define HDR_4K_OR_MMU_PAGE_ALIGN ALIGN(4096)
#endif

#define PHYS_SRAM0_ADDR	(DT_REG_ADDR(DT_NODELABEL(sram0)))
#define PHYS_SRAM0_SIZE	(DT_REG_SIZE(DT_NODELABEL(sram0)))

#define PHYS_ROM0_ADDR	(DT_REG_ADDR(DT_NODELABEL(rom0)))
#define PHYS_ROM0_SIZE	(DT_REG_SIZE(DT_NODELABEL(rom0)))

/* Usable RAM is after the exception vectors and page-aligned. */
#define PHYS_RAM_ADDR	(PHYS_SRAM0_ADDR + CONFIG_SRAM_OFFSET)
#define PHYS_RAM_SIZE	(PHYS_SRAM0_SIZE - CONFIG_SRAM_OFFSET)

MEMORY
{
  sram0_0_seg  : org = 0x00002000, len = 0x178
  sram0_1_seg  : org = 0x00002178, len = 0x8
  sram0_2_seg  : org = 0x00002180, len = 0x38
  sram0_3_seg  : org = 0x000021B8, len = 0x8
  sram0_4_seg  : org = 0x000021C0, len = 0x38
  sram0_5_seg  : org = 0x000021F8, len = 0x8
  sram0_6_seg  : org = 0x00002200, len = 0x38
  sram0_7_seg  : org = 0x00002238, len = 0x8
  sram0_8_seg  : org = 0x00002240, len = 0x38
  sram0_9_seg  : org = 0x00002278, len = 0x8
  sram0_10_seg : org = 0x00002280, len = 0x38
  sram0_11_seg : org = 0x000022B8, len = 0x8
  sram0_12_seg : org = 0x000022C0, len = 0x38
  sram0_13_seg : org = 0x000022F8, len = 0x8
  sram0_14_seg : org = 0x00002300, len = 0x38
  sram0_15_seg : org = 0x00002338, len = 0x8
  sram0_16_seg : org = 0x00002340, len = 0x38
  sram0_17_seg : org = 0x00002378, len = 0x48
  sram0_18_seg : org = 0x000023C0, len = 0x40
#ifdef CONFIG_XTENSA_MMU
  vec_helpers  : org = 0x00002400, len = (PHYS_RAM_ADDR - 0x00002400)
#endif
  RAM          : org = PHYS_RAM_ADDR, len = PHYS_RAM_SIZE

  /* Although ROM is of size 0x02000000, we limit it to 8KB so
   * fewer L2 page table entries are needed.
   */
  rom0_seg    : org = PHYS_ROM0_ADDR, len = PHYS_ROM0_SIZE

#ifdef CONFIG_GEN_ISR_TABLES
  /* The space before exception vectors is not being used.
   * So we stuff the temporary IDT_LIST there to avoid
   * some linker issues which would balloon the size of
   * the intermediate files (like zephyr_pre0.elf, to
   * couple hundred MBs or even GBs).
   */
  IDT_LIST     : org = 0x00000000, len = 0x2000
#endif
}

PHDRS
{
  sram0_0_phdr PT_LOAD;
  sram0_1_phdr PT_LOAD;
  sram0_2_phdr PT_LOAD;
  sram0_3_phdr PT_LOAD;
  sram0_4_phdr PT_LOAD;
  sram0_5_phdr PT_LOAD;
  sram0_6_phdr PT_LOAD;
  sram0_7_phdr PT_LOAD;
  sram0_8_phdr PT_LOAD;
  sram0_9_phdr PT_LOAD;
  sram0_10_phdr PT_LOAD;
  sram0_11_phdr PT_LOAD;
  sram0_12_phdr PT_LOAD;
  sram0_13_phdr PT_LOAD;
  sram0_14_phdr PT_LOAD;
  sram0_15_phdr PT_LOAD;
  sram0_16_phdr PT_LOAD;
  sram0_17_phdr PT_LOAD;
  sram0_18_phdr PT_LOAD;

#ifdef CONFIG_XTENSA_MMU
  vec_helpers_phdr PT_LOAD;
#endif

  rom0_phdr PT_LOAD;
  sram0_phdr PT_LOAD;
  sram0_bss_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0x00002000);
PROVIDE(_memmap_reset_vector = 0xFE000000);
/* Various memory-map dependent cache attribute settings:
 *
 * Note that there is no cacheattr register which means thah
 * cacheattr is emulated through TLB way 6 (8x 512MB regions).
 * So the attributes here are the MMU memory attributes:
 *   0x3 - rwx, bypass cache
 *   0x7 - rwx, cache write back
 *   0xB - wrx, cache write through
 * Refer to the ISA manual for other attributes.
 */
_memmap_cacheattr_wb_base = 0x70000007;
_memmap_cacheattr_wt_base = 0xB000000B;
_memmap_cacheattr_bp_base = 0x30000003;
_memmap_cacheattr_unused_mask = 0x0FFFFFF0;
_memmap_cacheattr_wb_strict = 0x7FFFFFF7;
_memmap_cacheattr_wt_strict = 0xBFFFFFFB;
_memmap_cacheattr_bp_strict = 0x3FFFFFF3;
_memmap_cacheattr_wb_allvalid = 0x73333337;
_memmap_cacheattr_wt_allvalid = 0xB333333B;
_memmap_cacheattr_bp_allvalid = 0x33333333;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_allvalid);

SECTIONS
{

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif

  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
  } >sram0_0_seg :sram0_0_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
  } >sram0_1_seg :sram0_1_phdr

  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
  } >sram0_2_seg :sram0_2_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
  } >sram0_3_seg :sram0_3_phdr

  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
  } >sram0_4_seg :sram0_4_phdr

  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
  } >sram0_5_seg :sram0_5_phdr

  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
  } >sram0_6_seg :sram0_6_phdr

  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
  } >sram0_7_seg :sram0_7_phdr

  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
  } >sram0_8_seg :sram0_8_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
  } >sram0_9_seg :sram0_9_phdr

  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
  } >sram0_10_seg :sram0_10_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
  } >sram0_11_seg :sram0_11_phdr

  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
  } >sram0_12_seg :sram0_12_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
  } >sram0_13_seg :sram0_13_phdr

  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
  } >sram0_14_seg :sram0_14_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
  } >sram0_15_seg :sram0_15_phdr

  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
  } >sram0_16_seg :sram0_16_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
  } >sram0_17_seg :sram0_17_phdr

  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
  } >sram0_18_seg :sram0_18_phdr

#ifdef CONFIG_XTENSA_MMU
  .vec_helpers :
  {
    /* There is quite some space between .DoubleExceptionVector
     * and the beginning of .text. We can put exception handling
     * code here to avoid TLB misses, thus speed up exception
     * handling a little bit.
     *
     * Note: DO NOT PUT MMU init code here as this will be
     *       mapped in TLB manually. This manual entry will
     *       conflict with auto-refill TLB resulting in
     *       TLB multi-hit exception.
     */

    *libarch__xtensa__core.a:xtensa-asm2-util.S.obj(.literal)
    *libarch__xtensa__core.a:xtensa-asm2-util.S.obj(.text)
    *libarch__xtensa__core.a:xtensa-asm2-util.S.obj(.iram.text)
    *libarch__xtensa__core.a:xtensa-asm2-util.S.obj(.iram0.text)

    *libarch__xtensa__core.a:window_vectors.S.obj(.iram.text)

    *libarch__xtensa__core.a:xtensa-asm2.c.obj(.literal.*)
    *libarch__xtensa__core.a:xtensa-asm2.c.obj(.text.*)

    *libarch__xtensa__core.a:fatal.c.obj(.literal.*)
    *libarch__xtensa__core.a:fatal.c.obj(.text.*)

    *libarch__xtensa__core.a:crt1.S.obj(.literal)
    *libarch__xtensa__core.a:crt1.S.obj(.text)

    *libarch__xtensa__core.a:cpu_idle.c.obj(.literal.*)
    *libarch__xtensa__core.a:cpu_idle.c.obj(.text.*)

    *(.text.arch_is_in_isr)

    /* To support backtracing */
    *libarch__xtensa__core.a:xtensa_backtrace.c.obj(.literal.*)
    *libarch__xtensa__core.a:xtensa_backtrace.c.obj(.text.*)
    *libarch__xtensa__core.a:debug_helpers_asm.S.obj(.iram1.literal)
    *libarch__xtensa__core.a:debug_helpers_asm.S.obj(.iram1)

    *libkernel.a:fatal.c.obj(.literal.*)
    *libkernel.a:fatal.c.obj(.text.*)

    /* Below are to speed up execution by avoiding TLB misses
     * on frequently used functions.
     */
    *libkernel.a:sched.c.obj(.literal.*)
    *libkernel.a:sched.c.obj(.text.*)
    *libkernel.a:timeout.c.obj(.literal.*)
    *libkernel.a:timeout.c.obj(.text.*)

    *libdrivers__console.a:(.literal.*)
    *libdrivers__console.a:(.text.*)
    *libdrivers__timer.a:(.literal.*)
    *libdrivers__timer.a:(.text.*)
  } >vec_helpers :vec_helpers_phdr
#endif /* CONFIG_XTENSA_MMU */

  .ResetVector.text : ALIGN(4)
  {
    __rom_region_start = ABSOLUTE(.);
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
  } >rom0_seg :rom0_phdr

  .text : HDR_MMU_PAGE_ALIGN
  {
    _stext = .;
    __text_region_start = .;
    z_mapped_start = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    KEEP(*(.init))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)

    #include <zephyr/linker/kobject-text.ld>

    MMU_PAGE_ALIGN

    _text_end = ABSOLUTE(.);
    _etext = .;
  } >RAM :sram0_phdr
  __text_region_end = .;

  .rodata : HDR_MMU_PAGE_ALIGN
  {
    __rodata_region_start = ABSOLUTE(.);
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)

    . = ALIGN(4);
    #include <snippets-rodata.ld>
    #include <zephyr/linker/kobject-rom.ld>

    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);

    MMU_PAGE_ALIGN

    __rodata_region_end = ABSOLUTE(.);
  } >RAM :sram0_phdr

#include <zephyr/linker/common-rom.ld>

#include <zephyr/linker/thread-local-storage.ld>

#include <zephyr/linker/cplusplus-rom.ld>

#include <snippets-sections.ld>

  .data : HDR_MMU_PAGE_ALIGN
  {
    _image_ram_start = ABSOLUTE(.);
    __data_start = ABSOLUTE(.);
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))

    . = ALIGN(4);
    #include <snippets-rwdata.ld>
    . = ALIGN(4);

    MMU_PAGE_ALIGN

    __data_end = ABSOLUTE(.);
  } >RAM :sram0_phdr

#include <snippets-data-sections.ld>

#include <zephyr/linker/common-ram.ld>

#include <zephyr/linker/cplusplus-ram.ld>

#include <snippets-ram-sections.ld>

  .bss (NOLOAD) : HDR_MMU_PAGE_ALIGN
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
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
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *(.sram.bss)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);

    MMU_PAGE_ALIGN

  } >RAM :sram0_bss_phdr

#include <zephyr/linker/common-noinit.ld>

/* Must be last in RAM */
#include <zephyr/linker/kobject-data.ld>

#include <zephyr/linker/ram-end.ld>

  _heap_start = .;

  PROVIDE(_heap_sentry = ORIGIN(RAM) + LENGTH(RAM));
  PROVIDE(_heap_end = ORIGIN(RAM) + LENGTH(RAM));

  PROVIDE(__stack = z_interrupt_stacks + CONFIG_ISR_STACK_SIZE);

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
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
}
