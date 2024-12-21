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

#ifdef CONFIG_MPU
#define MPU_SEGMENT_SIZE_ALIGN . = ALIGN(XCHAL_MPU_ALIGN);
#define HDR_MPU_SEGMENT_SIZE_ALIGN ALIGN(XCHAL_MPU_ALIGN)
#define HDR_4K_OR_MPU_SEGMENT_SIZE_ALIGN ALIGN(XCHAL_MPU_ALIGN)
#define LAST_RAM_ALIGN MPU_SEGMENT_SIZE_ALIGN
#else
#define MPU_SEGMENT_SIZE_ALIGN . = ALIGN(4);
#define HDR_MPU_SEGMENT_SIZE_ALIGN ALIGN(4)
#define HDR_4K_OR_MPU_SEGMENT_SIZE_ALIGN ALIGN(4096)
#endif

#define PHYS_SRAM0_ADDR	(DT_REG_ADDR(DT_NODELABEL(sram0)))
#define PHYS_SRAM0_SIZE	(DT_REG_SIZE(DT_NODELABEL(sram0)))

#define PHYS_ROM0_ADDR	(DT_REG_ADDR(DT_NODELABEL(srom0)))
#define PHYS_ROM0_SIZE	(DT_REG_SIZE(DT_NODELABEL(srom0)))

/* Usable RAM is after the exception vectors and page-aligned. */
#define PHYS_RAM_ADDR	(PHYS_SRAM0_ADDR + CONFIG_SRAM_OFFSET)
#define PHYS_RAM_SIZE	(PHYS_SRAM0_SIZE - CONFIG_SRAM_OFFSET)

MEMORY
{
  dram1_0_seg  : org = 0x3FFC0000, len = 0x20000
  dram0_0_seg  : org = 0x3FFE0000, len = 0x20000
  iram0_0_seg  : org = 0x40000000, len = 0x178
  iram0_1_seg  : org = 0x40000178, len = 0x8
  iram0_2_seg  : org = 0x40000180, len = 0x38
  iram0_3_seg  : org = 0x400001B8, len = 0x8
  iram0_4_seg  : org = 0x400001C0, len = 0x38
  iram0_5_seg  : org = 0x400001F8, len = 0x8
  iram0_6_seg  : org = 0x40000200, len = 0x38
  iram0_7_seg  : org = 0x40000238, len = 0x8
  iram0_8_seg  : org = 0x40000240, len = 0x38
  iram0_9_seg  : org = 0x40000278, len = 0x8
  iram0_10_seg : org = 0x40000280, len = 0x38
  iram0_11_seg : org = 0x400002B8, len = 0x8
  iram0_12_seg : org = 0x400002C0, len = 0x38
  iram0_13_seg : org = 0x400002F8, len = 0x8
  iram0_14_seg : org = 0x40000300, len = 0x38
  iram0_15_seg : org = 0x40000338, len = 0x8
  iram0_16_seg : org = 0x40000340, len = 0x38
  iram0_17_seg : org = 0x40000378, len = 0x48
  iram0_18_seg : org = 0x400003C0, len = 0x40
  iram0_19_seg : org = 0x40000400, len = 0x1FC00
#ifdef CONFIG_MPU
  vec_helpers  : org = 0x40002400, len = (PHYS_RAM_ADDR - 0x00002400)
#endif
  srom0_seg    : org = PHYS_ROM0_ADDR, len = PHYS_ROM0_SIZE
  RAM          : org = PHYS_RAM_ADDR, len = PHYS_RAM_SIZE

#ifdef CONFIG_GEN_ISR_TABLES
  /* The space before exception vectors is not being used.
   * So we stuff the temporary IDT_LIST there to avoid
   * some linker issues which would balloon the size of
   * the intermediate files (like zephyr_pre0.elf, to
   * couple hundred MBs or even GBs).
   */
  IDT_LIST     : org = 0x3FFBE000, len = 0x2000
#endif
}

PHDRS
{
  dram1_0_phdr PT_LOAD;
  dram1_0_bss_phdr PT_LOAD;
  dram0_0_phdr PT_LOAD;
  dram0_0_bss_phdr PT_LOAD;
  iram0_0_phdr PT_LOAD;
  iram0_1_phdr PT_LOAD;
  iram0_2_phdr PT_LOAD;
  iram0_3_phdr PT_LOAD;
  iram0_4_phdr PT_LOAD;
  iram0_5_phdr PT_LOAD;
  iram0_6_phdr PT_LOAD;
  iram0_7_phdr PT_LOAD;
  iram0_8_phdr PT_LOAD;
  iram0_9_phdr PT_LOAD;
  iram0_10_phdr PT_LOAD;
  iram0_11_phdr PT_LOAD;
  iram0_12_phdr PT_LOAD;
  iram0_13_phdr PT_LOAD;
  iram0_14_phdr PT_LOAD;
  iram0_15_phdr PT_LOAD;
  iram0_16_phdr PT_LOAD;
  iram0_17_phdr PT_LOAD;
  iram0_18_phdr PT_LOAD;

#ifdef CONFIG_XTENSA_MPU
  vec_helpers_phdr PT_LOAD;
#endif

  srom0_phdr PT_LOAD;
  sram0_phdr PT_LOAD;
  sram0_bss_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

_rom_store_table = 0;

PROVIDE(_memmap_vecbase_reset = 0x40000000);
PROVIDE(_memmap_reset_vector = 0x50000000);

/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x00001110;
_memmap_cacheattr_wt_base = 0x00001110;
_memmap_cacheattr_bp_base = 0x00002220;
_memmap_cacheattr_unused_mask = 0xFFFF000F;
_memmap_cacheattr_wb_trapnull = 0x2222111F;
_memmap_cacheattr_wba_trapnull = 0x2222111F;
_memmap_cacheattr_wbna_trapnull = 0x2222111F;
_memmap_cacheattr_wt_trapnull = 0x2222111F;
_memmap_cacheattr_bp_trapnull = 0x2222222F;
_memmap_cacheattr_wb_strict = 0xFFFF111F;
_memmap_cacheattr_wt_strict = 0xFFFF111F;
_memmap_cacheattr_bp_strict = 0xFFFF222F;
_memmap_cacheattr_wb_allvalid = 0x22221112;
_memmap_cacheattr_wt_allvalid = 0x22221112;
_memmap_cacheattr_bp_allvalid = 0x22222222;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_bp_trapnull);

SECTIONS
{

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif

  .dram1.rodata : ALIGN(4)
  {
    _dram1_rodata_start = ABSOLUTE(.);
    *(.dram1.rodata)
    _dram1_rodata_end = ABSOLUTE(.);
  } >dram1_0_seg :dram1_0_phdr

  .dram1.literal : ALIGN(4)
  {
    _dram1_literal_start = ABSOLUTE(.);
    *(.dram1.literal)
    _dram1_literal_end = ABSOLUTE(.);
  } >dram1_0_seg :dram1_0_phdr

  .dram1.data : ALIGN(4)
  {
    _dram1_data_start = ABSOLUTE(.);
    *(.dram1.data)
    _dram1_data_end = ABSOLUTE(.);
  } >dram1_0_seg :dram1_0_phdr

  .dram1.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _dram1_bss_start = ABSOLUTE(.);
    *(.dram1.bss)
    . = ALIGN (8);
    _dram1_bss_end = ABSOLUTE(.);
    _memmap_seg_dram1_0_end = ALIGN(0x8);
  } >dram1_0_seg :dram1_0_bss_phdr

  .dram0.rodata : ALIGN(4)
  {
    _dram0_rodata_start = ABSOLUTE(.);
    *(.dram0.rodata)
    _dram0_rodata_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

  .dram0.literal : ALIGN(4)
  {
    _dram0_literal_start = ABSOLUTE(.);
    *(.dram0.literal)
    _dram0_literal_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

  .dram0.data : ALIGN(4)
  {
    _dram0_data_start = ABSOLUTE(.);
    *(.dram0.data)
    _dram0_data_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

  .dram0.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _dram0_bss_start = ABSOLUTE(.);
    *(.dram0.bss)
    . = ALIGN (8);
    _dram0_bss_end = ABSOLUTE(.);
    _memmap_seg_dram0_0_end = ALIGN(0x8);
  } >dram0_0_seg :dram0_0_bss_phdr

  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
  } >iram0_0_seg :iram0_0_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_1_seg :iram0_1_phdr

  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_2_seg :iram0_2_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_3_seg :iram0_3_phdr

  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_4_seg :iram0_4_phdr

  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_5_seg :iram0_5_phdr

  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_6_seg :iram0_6_phdr

  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
  } >iram0_7_seg :iram0_7_phdr

  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
  } >iram0_8_seg :iram0_8_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_9_seg :iram0_9_phdr

  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_10_seg :iram0_10_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_11_seg :iram0_11_phdr

  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_12_seg :iram0_12_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_13_seg :iram0_13_phdr

  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_14_seg :iram0_14_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_15_seg :iram0_15_phdr

  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_16_seg :iram0_16_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
  } >iram0_17_seg :iram0_17_phdr

  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
  } >iram0_18_seg :iram0_18_phdr

#define LIB_OBJ_FUNC_IN_SECT(library, obj_file, func)		\
	*##library##:##obj_file##(.literal.##func .text.##func)	\

#ifdef CONFIG_XTENSA_MPU
  .vec_helpers :
  {
    /* There is quite some space between .DoubleExceptionVector
     * and the beginning of .text. We can put exception handling
     * code here.
     */

    *libarch__xtensa__core.a:xtensa_asm2_util.S.obj(.literal .text)
    *libarch__xtensa__core.a:xtensa_asm2_util.S.obj(.iram.text .iram0.text)

    *libarch__xtensa__core.a:window_vectors.S.obj(.iram.text)

    *libarch__xtensa__core.a:crt1.S.obj(.literal .text)

    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,xtensa_asm2.c.obj,*)
    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,fatal.c.obj,*)
    LIB_OBJ_FUNC_IN_SECT(libarch__xtensa__core.a,cpu_idle.c.obj,*)

    *(.literal.arch_is_in_isr .text.arch_is_in_isr)

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
#endif /* CONFIG_XTENSA_MPU */

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_relocate.ld>
#endif

  .ResetVector.text : ALIGN(4)
  {
    __rom_region_start = ABSOLUTE(.);
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
  } >srom0_seg :srom0_phdr

  .text : HDR_MPU_SEGMENT_SIZE_ALIGN
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

    MPU_SEGMENT_SIZE_ALIGN

    _text_end = ABSOLUTE(.);
    _etext = .;
  } >RAMABLE_REGION
  __text_region_end = .;

  .rodata : HDR_MPU_SEGMENT_SIZE_ALIGN
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

#include <zephyr/linker/thread-local-storage.ld>

#include <zephyr/linker/cplusplus-rom.ld>

  .rodata_end : ALIGN(4)
  {
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);

    MPU_SEGMENT_SIZE_ALIGN

    __rodata_region_end = ABSOLUTE(.);
  } >RAMABLE_REGION

#ifdef CONFIG_USERSPACE
#define SMEM_PARTITION_ALIGN(size) MPU_SEGMENT_SIZE_ALIGN
#define APP_SHARED_ALIGN  MPU_SEGMENT_SIZE_ALIGN

#include <app_smem.ld>

  _image_ram_start = _app_smem_start;
  _app_smem_size = _app_smem_end - _app_smem_start;
  _app_smem_num_words = _app_smem_size >> 2;
  _app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
  _app_smem_num_words = _app_smem_size >> 2;
#endif /* CONFIG_USERSPACE */

  .data : HDR_MPU_SEGMENT_SIZE_ALIGN
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

    MPU_SEGMENT_SIZE_ALIGN

    __data_end = ABSOLUTE(.);
  } >RAMABLE_REGION

#include <snippets-sections.ld>

#include <snippets-data-sections.ld>

#include <zephyr/linker/common-ram.ld>

#include <zephyr/linker/cplusplus-ram.ld>

#include <snippets-ram-sections.ld>

  .bss (NOLOAD) : HDR_MPU_SEGMENT_SIZE_ALIGN
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

    MPU_SEGMENT_SIZE_ALIGN

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
