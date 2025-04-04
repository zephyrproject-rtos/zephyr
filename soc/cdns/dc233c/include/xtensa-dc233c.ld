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
#define ROMABLE_REGION RAM :sram0_phdr

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
  vectors : org = 0x00002000, len = 0x2400
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
  vectors_phdr PT_LOAD;
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

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif

/* Auto-generated vector linkage, to "vectors" memory region */
#include <xtensa_vectors.ld>
 >vectors :vectors_phdr

#define LIB_OBJ_FUNC_IN_SECT(library, obj_file, func)		\
	*##library##:##obj_file##(.literal.##func .text.##func)	\

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

    *libarch__xtensa__core.a:xtensa_asm2_util.S.obj(.literal .text)
    *libarch__xtensa__core.a:xtensa_asm2_util.S.obj(.iram.text .iram0.text)

    *libarch__xtensa__core.a:window_vectors.S.obj(.iram.text)

    *libarch__xtensa__core.a:crt1.S.obj(.literal .text)

    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,xtensa_asm2.c.obj,*)
    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,fatal.c.obj,*)
    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,cpu_idle.c.obj,*)

    *(.text.arch_is_in_isr)

    /* To support backtracing */
    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,xtensa_backtrace.c.obj,*)

    *libarch__xtensa__core.a:debug_helpers_asm.S.obj(.iram1.literal .iram1)

    /* Userspace related stuff */
    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,userspace.S.obj,xtensa_do_syscall)

    /* Below are to speed up execution by avoiding TLB misses
     * on frequently used functions.
     *
     * There is almost 1MB space (due to TLB pinning) so we can
     * be generous.
     */
    LIB_OBJ_FUNC_IN_SECT(libkernel.a,,*)

    LIB_OBJ_FUNC_IN_SECT(libdrivers__console.a,,*)
    LIB_OBJ_FUNC_IN_SECT(libdrivers__timer.a,,*)

    *(.literal.z_vrfy_* .text.z_vrfy_*)
    *(.literal.z_mrsh_* .text.z_mrsh_*)
    *(.literal.z_impl_* .text.z_impl_*)
    *(.literal.z_obj_*  .text.z_obj_*)

    *(.literal.k_sys_fatal_error_handler .text.k_sys_fatal_error_handler)
  } >vec_helpers :vec_helpers_phdr
#endif /* CONFIG_XTENSA_MMU */

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_relocate.ld>
#endif

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
    *(.iram1.literal .iram1)
    KEEP(*(.init))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)

    #include <zephyr/linker/kobject-text.ld>

    MMU_PAGE_ALIGN

    _text_end = ABSOLUTE(.);
    _etext = .;
  } >RAMABLE_REGION
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
  } >RAMABLE_REGION

#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>

#include <zephyr/linker/thread-local-storage.ld>

#include <zephyr/linker/cplusplus-rom.ld>

  .rodata_end : ALIGN(4)
  {
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);

    MMU_PAGE_ALIGN

    __rodata_region_end = ABSOLUTE(.);
  } >RAMABLE_REGION

#ifdef CONFIG_USERSPACE
#define SMEM_PARTITION_ALIGN(size) MMU_PAGE_ALIGN
#define APP_SHARED_ALIGN  MMU_PAGE_ALIGN

#include <app_smem.ld>

  _image_ram_start = _app_smem_start;
  _app_smem_size = _app_smem_end - _app_smem_start;
  _app_smem_num_words = _app_smem_size >> 2;
  _app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
  _app_smem_num_words = _app_smem_size >> 2;
#endif /* CONFIG_USERSPACE */

  .data : HDR_MMU_PAGE_ALIGN
  {
#ifndef CONFIG_USERSPACE
    _image_ram_start = ABSOLUTE(.);
#endif
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
  } >RAMABLE_REGION

#include <snippets-sections.ld>

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
