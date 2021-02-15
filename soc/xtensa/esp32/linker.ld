/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Xtensa platform.
 */

#include <devicetree.h>
#include <autoconf.h>
#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define RAMABLE_REGION dram0_0_seg :dram0_0_phdr
#define RAMABLE_REGION_1 dram0_1_seg :dram0_1_phdr
#define ROMABLE_REGION drom0_0_seg :drom0_0_phdr
#define IRAM_REGION iram0_0_seg :iram0_0_phdr
#define FLASH_CODE_REGION irom0_0_seg :irom0_0_phdr

PROVIDE ( __stack = 0x3ffe3f20 );

/* Global symbols required for espressif hal build */
PROVIDE ( ets_printf = 0x40007d54 );
PROVIDE ( intr_matrix_set = 0x4000681c );
PROVIDE ( g_ticks_per_us_app = 0x3ffe40f0 );
PROVIDE ( g_ticks_per_us_pro = 0x3ffe01e0 );
PROVIDE ( ets_delay_us = 0x40008534 );
PROVIDE ( gpio_output_set = 0x40009b24 );
PROVIDE ( gpio_output_set_high = 0x40009b5c );
PROVIDE ( roundup2 = 0x4000ab7c );
PROVIDE ( crc32_le = 0x4005cfec );
PROVIDE ( Cache_Read_Disable_rom = 0x40009ab8 );
PROVIDE ( Cache_Read_Enable_rom = 0x40009a84 );
PROVIDE ( Cache_Read_Init_rom = 0x40009950 );
PROVIDE ( phy_get_romfuncs = 0x40004100 );
PROVIDE ( SPI1 = 0x3ff42fff );
PROVIDE ( SPI2 = 0x3ff64fff );
PROVIDE ( SPI3 = 0x3ff65fff );

PROVIDE ( esp32_rom_uart_tx_one_char = 0x40009200 );
PROVIDE ( esp32_rom_uart_rx_one_char = 0x400092d0 );
PROVIDE ( esp32_rom_uart_attach = 0x40008fd0 );
PROVIDE ( esp32_rom_uart_tx_wait_idle = 0x40009278 );
PROVIDE ( esp32_rom_intr_matrix_set = intr_matrix_set );
PROVIDE ( esp32_rom_gpio_matrix_in = 0x40009edc );
PROVIDE ( esp32_rom_gpio_matrix_out = 0x40009f0c );
PROVIDE ( esp32_rom_Cache_Flush = 0x40009a14 );
PROVIDE ( esp32_rom_Cache_Read_Enable = 0x40009a84 );
PROVIDE ( esp32_rom_ets_set_appcpu_boot_addr = 0x4000689c );
PROVIDE ( esp32_rom_i2c_readReg = 0x40004148 );
PROVIDE ( esp32_rom_i2c_writeReg = 0x400041a4 );
PROVIDE ( esp32_rom_ets_printf = ets_printf );
PROVIDE ( esp32_rom_g_ticks_per_us_app = g_ticks_per_us_app );
PROVIDE ( esp32_rom_g_ticks_per_us_pro = g_ticks_per_us_app );
PROVIDE ( esp32_rom_ets_delay_us = ets_delay_us );
PROVIDE ( TIMERG0 = 0x3ff5F000 );
PROVIDE ( TIMERG1 = 0x3ff60000 );

/* __udivdi3 is exported using assignment, which declares strong symbols */
__udivdi3 = 0x4000cff8;
__umoddi3 = 0x4000d280;

MEMORY
{
  iram0_0_seg(RX): org = 0x40080000, len = 0x20000
  irom0_0_seg(RX): org = 0x400D0020, len = 0x330000-0x20
  /*
   * Following is DRAM memory split with reserved address ranges in ESP32:
   *
   * 0x3FFA_E000 - 0x3FFB_0000 (Reserved: data memory for ROM functions)
   * 0x3FFB_0000 - 0x3FFE_0000 (RAM bank 1 for application usage)
   * 0x3FFE_0000 - 0x3FFE_0440 (Reserved: data memory for ROM PRO CPU)
   * 0x3FFE_3F20 - 0x3FFE_4350 (Reserved: data memory for ROM APP CPU)
   * 0x3FFE_4350 - 0x3F10_0000 (RAM bank 2 for application usage)
   *
   * FIXME:
   *  - Utilize available memory regions to full capacity
   *  - Reserve memory region for BT controller library from ROM
   */
  dram0_0_seg(RW): org = 0x3FFB0000, len = 0x30000
  dram0_1_seg(RW): org = 0x3FFE4350, len = 0x1BCB0
  drom0_0_seg(R): org = 0x3F400020, len = 0x400000-0x20
  rtc_iram_seg(RWX): org = 0x400C0000, len = 0x2000
  rtc_slow_seg(RW): org = 0x50000000, len = 0x1000
#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif
}

PHDRS
{
  drom0_0_phdr PT_LOAD;
  dram0_0_phdr PT_LOAD;
  dram0_1_phdr PT_LOAD;
  iram0_0_phdr PT_LOAD;
  irom0_0_phdr PT_LOAD;
}

/*  Default entry point:  */
PROVIDE ( _ResetVector = 0x40000400 );
ENTRY(CONFIG_KERNEL_ENTRY)

_rom_store_table = 0;

PROVIDE(_memmap_vecbase_reset = 0x40000450);
PROVIDE(_memmap_reset_vector = 0x40000400);

SECTIONS
{

#include <linker/rel-sections.ld>

  /* RTC fast memory holds RTC wake stub code,
     including from any source file named rtc_wake_stub*.c
  */
  .rtc.text :
  {
    . = ALIGN(4);
    *(.rtc.literal .rtc.text)
    *rtc_wake_stub*.o(.literal .text .literal.* .text.*)
  } >rtc_iram_seg

  /* RTC slow memory holds RTC wake stub
     data/rodata, including from any source file
     named rtc_wake_stub*.c
  */
  .rtc.data :
  {
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data)
    *(.rtc.rodata)
    *rtc_wake_stub*.o(.data .rodata .data.* .rodata.* .bss .bss.*)
    _rtc_data_end = ABSOLUTE(.);
  } > rtc_slow_seg

  /* RTC bss, from any source file named rtc_wake_stub*.c */
  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *rtc_wake_stub*.o(.bss .bss.*)
    *rtc_wake_stub*.o(COMMON)
    _rtc_bss_end = ABSOLUTE(.);
  } > rtc_slow_seg

  /* Send .iram0 code to iram */
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
    *(.*Vector.literal)

    *(.UserEnter.literal);
    *(.UserEnter.text);
    . = ALIGN (16);
    *(.entry.text)
    *(.init.literal)
    *(.init)
    _init_end = ABSOLUTE(.);

    /* This goes here, not at top of linker script, so addr2line finds it last,
       and uses it in preference to the first symbol in IRAM */
    _iram_start = ABSOLUTE(0);
  } GROUP_LINK_IN(IRAM_REGION)

  SECTION_DATA_PROLOGUE(k_objects,, ALIGN(4))
  {
    Z_LINK_ITERABLE_GC_ALLOWED(k_timer);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_mem_slab);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_mem_pool);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_heap);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_mutex);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_stack);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_msgq);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_mbox);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_pipe);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_sem);
    . = ALIGN(4);
    Z_LINK_ITERABLE_GC_ALLOWED(k_queue);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  SECTION_DATA_PROLOGUE(net,, ALIGN(4))
  {
    _esp_net_buf_pool_list = .;
    KEEP(*(SORT_BY_NAME("._net_buf_pool.static.*")))

#if defined(CONFIG_NETWORKING)
    . = ALIGN(4);
    Z_LINK_ITERABLE(net_if);
    . = ALIGN(4);
    Z_LINK_ITERABLE(net_if_dev);
    . = ALIGN(4);
    Z_LINK_ITERABLE(net_l2);
#endif

  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  Z_ITERABLE_SECTION_RAM(_static_thread_data, 4)

#pragma push_macro("Z_ITERABLE_SECTION_RAM")
#pragma push_macro("Z_ITERABLE_SECTION_RAM_GC_ALLOWED")
#undef Z_ITERABLE_SECTION_RAM_GC_ALLOWED
#define Z_ITERABLE_SECTION_RAM_GC_ALLOWED(x, y)
#undef Z_ITERABLE_SECTION_RAM
#define Z_ITERABLE_SECTION_RAM(x, y)
#include <linker/common-ram.ld>
/* Restore original value for symbols referenced by `common-ram.ld` */
_net_buf_pool_list = _esp_net_buf_pool_list;
#pragma pop_macro("Z_ITERABLE_SECTION_RAM_GC_ALLOWED")
#pragma pop_macro("Z_ITERABLE_SECTION_RAM")

  .dram0.data :
  {
    _data_start = ABSOLUTE(.);
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
    /* rodata for panic handler(libarch__xtensa__core.a) and all
     * dependent functions should be placed in DRAM to avoid issue
     * when flash cache is disabled */
    *libarch__xtensa__core.a:(.rodata .rodata.*)
    *libkernel.a:fatal.*(.rodata .rodata.*)
    *libkernel.a:init.*(.rodata .rodata.*)
    *libzephyr.a:cbprintf_complete*(.rodata .rodata.*)
    *libzephyr.a:log_core.*(.rodata .rodata.*)
    *libzephyr.a:log_backend_uart.*(.rodata .rodata.*)
    *libzephyr.a:log_output.*(.rodata .rodata.*)
    *libdrivers__serial.a:uart_esp32.*(.rodata .rodata.*)

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

 SECTION_PROLOGUE(_RODATA_SECTION_NAME,,ALIGN(20))
  {
    _rodata_start = ABSOLUTE(.);
#if defined(CONFIG_NET_SOCKETS)
    . = ALIGN(4);
    Z_LINK_ITERABLE(net_socket_register);
    . = ALIGN(4);
#endif

#if defined(CONFIG_SETTINGS)
    . = ALIGN(4);
    Z_LINK_ITERABLE(settings_handler_static);
    . = ALIGN(4);
#endif

    . = ALIGN(4);
    Z_LINK_ITERABLE(shell);
    . = ALIGN(4);

    __esp_shell_root_cmds_start = .;
    KEEP(*(SORT(.shell_root_cmd_*)));
    __esp_shell_root_cmds_end = .;

    . = ALIGN(4);
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table .gcc_except_table.*))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);               /* this table MUST be 4-byte aligned */
    _rodata_end = ABSOLUTE(.);
  } GROUP_LINK_IN(ROMABLE_REGION)

#pragma push_macro("Z_ITERABLE_SECTION_ROM")
#pragma push_macro("ROMABLE_REGION")
#undef Z_ITERABLE_SECTION_ROM
#define Z_ITERABLE_SECTION_ROM(x,y)
#undef ROMABLE_REGION
/* This is to workaround limitation of `esptool` which needs single `FLASH` data segment
 * which is already defined above. In case, `common-rom.ld` creates additional segments
 * they will be placed in DRAM instead. */
#define ROMABLE_REGION RAMABLE_REGION
#include <linker/common-rom.ld>
/* Restore original value for symbols referenced by `common-rom.ld` */
__log_const_start = __esp_log_const_start;
__log_const_end = __esp_log_const_end;
__log_backends_start = __esp_log_backends_start;
__log_backends_end = __esp_log_backends_end;
__shell_root_cmds_start = __esp_shell_root_cmds_start;
__shell_root_cmds_end = __esp_shell_root_cmds_end;
#pragma pop_macro("ROMABLE_REGION")
#pragma pop_macro("Z_ITERABLE_SECTION_ROM")

  SECTION_PROLOGUE(_TEXT_SECTION_NAME, , ALIGN(4))
  {
    /* Code marked as running out of IRAM */
    _iram_text_start = ABSOLUTE(.);
    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    *libesp32.a:panic.*(.literal .text .literal.* .text.*)
    *librtc.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__l2__ethernet.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__lib__config.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__ip.a:(.literal .text .literal.* .text.*)
    *libsubsys__net.a:(.literal .text .literal.* .text.*)
    *libarch__xtensa__core.a:(.literal .text .literal.* .text.*)
    *libkernel.a:(.literal .text .literal.* .text.*)
    *libsoc.a:rtc_*.*(.literal .text .literal.* .text.*)
    *libsoc.a:cpu_util.*(.literal .text .literal.* .text.*)
    *libhal.a:(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_rom_patch.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_noos.*(.literal .text .literal.* .text.*)
    *libzephyr.a:xtensa_sys_timer.*(.literal .text .literal.* .text.*)
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
    *libnet80211.a:( .wifi0iram  .wifi0iram.*)
    *libpp.a:( .wifi0iram  .wifi0iram.*)
    *libnet80211.a:( .wifirxiram  .wifirxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.*)
    _iram_text_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)

  .flash.text :
  {
    _stext = .;
    _text_start = ABSOLUTE(.);

    *(.literal .text .literal.* .text.*)
    _text_end = ABSOLUTE(.);
    _etext = .;

    /* Similar to _iram_start, this symbol goes here so it is
       resolved by addr2line in preference to the first symbol in
       the flash.text segment.
    */
    _flash_cache_start = ABSOLUTE(0);
  } GROUP_LINK_IN(FLASH_CODE_REGION)

  /* Shared RAM */
  SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
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
    *(.share.mem)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(RAMABLE_REGION)

  SECTION_DATA_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD),)
  {
    . = ALIGN (8);
    *(.noinit)
    *(".noinit.*")
    . = ALIGN (8);
    _heap_start = ABSOLUTE(.);
  } GROUP_LINK_IN(RAMABLE_REGION_1)

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif

#include <linker/debug-sections.ld>

  SECTION_PROLOGUE(.xtensa.info, 0,)
  {
    *(.xtensa.info)
  }

}
