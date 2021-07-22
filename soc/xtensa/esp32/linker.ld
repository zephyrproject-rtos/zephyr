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
   */
  dram0_0_seg(RW): org = 0x3FFB0000 + CONFIG_ESP32_BT_RESERVE_DRAM, len = 0x2c200 - CONFIG_ESP32_BT_RESERVE_DRAM
  dram0_1_seg(RW): org = 0x3FFE4350, len = 0x1BCB0
  drom0_0_seg(R): org = 0x3F400020, len = 0x400000-0x20
  rtc_iram_seg(RWX): org = 0x400C0000, len = 0x2000
  rtc_slow_seg(RW): org = 0x50000000, len = 0x1000
#if defined(CONFIG_ESP_SPIRAM)
  ext_ram_seg(RW): org = 0x3F800000, len = CONFIG_ESP_SPIRAM_SIZE
#endif
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
    Z_LINK_ITERABLE_ALIGNED(net_if, 4);
    Z_LINK_ITERABLE_ALIGNED(net_if_dev, 4);
    Z_LINK_ITERABLE_ALIGNED(net_l2, 4);
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

    _btdm_data_start = ABSOLUTE(.);
    *libbtdm_app.a:(.data .data.*)
    . = ALIGN (4);
    _btdm_data_end = ABSOLUTE(.);

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
    *libdrivers__flash.a:flash_esp32.*(.rodata  .rodata.*)
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

#if defined(CONFIG_ESP_SPIRAM)
  .ext_ram.bss (NOLOAD):
  {
    _ext_ram_data_start = ABSOLUTE(.);
    *(.ext_ram.bss*)
    _ext_ram_data_end = ABSOLUTE(.) + CONFIG_ESP_SPIRAM_SIZE;
  } > ext_ram_seg
#endif

 SECTION_PROLOGUE(_RODATA_SECTION_NAME,,ALIGN(20))
  {
    _rodata_start = ABSOLUTE(.);

#ifdef CONFIG_USERSPACE
    Z_LINK_ITERABLE_ALIGNED(z_object_assignment, 4);
#endif

#if defined(CONFIG_NET_SOCKETS)
    Z_LINK_ITERABLE_ALIGNED(net_socket_register, 4);
#endif

#if defined(CONFIG_NET_L2_PPP)
    Z_LINK_ITERABLE_ALIGNED(ppp_protocol_handler, 4);
#endif

    Z_LINK_ITERABLE_ALIGNED(bt_l2cap_fixed_chan, 4);

#if defined(CONFIG_BT_BREDR)
    Z_LINK_ITERABLE_ALIGNED(bt_l2cap_br_fixed_chan, 4);
#endif

    Z_LINK_ITERABLE_ALIGNED(bt_gatt_service_static, 4);

#if defined(CONFIG_BT_MESH)
    Z_LINK_ITERABLE_ALIGNED(bt_mesh_subnet_cb, 4);
    Z_LINK_ITERABLE_ALIGNED(bt_mesh_app_key_cb, 4);
    Z_LINK_ITERABLE_ALIGNED(bt_mesh_hb_cb, 4);
#endif

#if defined(CONFIG_BT_MESH_FRIEND)
    Z_LINK_ITERABLE_ALIGNED(bt_mesh_friend_cb, 4);
#endif

#if defined(CONFIG_BT_MESH_LOW_POWER)
    Z_LINK_ITERABLE_ALIGNED(bt_mesh_lpn_cb, 4);
#endif

#if defined(CONFIG_BT_MESH_PROXY)
    Z_LINK_ITERABLE_ALIGNED(bt_mesh_proxy_cb, 4);
#endif

#if defined(CONFIG_EC_HOST_CMD)
    Z_LINK_ITERABLE_ALIGNED(ec_host_cmd_handler, 4);
#endif

#if defined(CONFIG_SETTINGS)
    Z_LINK_ITERABLE_ALIGNED(settings_handler_static, 4);
#endif

    Z_LINK_ITERABLE_ALIGNED(k_p4wq_initparam, 4);

    Z_LINK_ITERABLE_ALIGNED(shell, 4);

    Z_LINK_ITERABLE_ALIGNED(tracing_backend, 4)

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
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libdrivers__flash.a:flash_esp32.*(.literal .text .literal.* .text.*)
    *libzephyr.a:windowspill_asm.*(.literal .text .literal.* .text.*)
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
    *libphy.a:( .phyiram .phyiram.*)
    *libgcov.a:(.literal .text .literal.* .text.*)

#if defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
#endif

#if defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif

    _iram_text_end = ABSOLUTE(.);
    . = ALIGN(4);
    _iram_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)

  .flash.text :
  {
    _stext = .;
    _text_start = ABSOLUTE(.);

#if !defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
#endif

#if !defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif

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

    _btdm_bss_start = ABSOLUTE(.);
    *libbtdm_app.a:(.bss .bss.* COMMON)
    . = ALIGN (4);
    _btdm_bss_end = ABSOLUTE(.);

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

  ASSERT(((_bss_end - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)),
          "DRAM segment data does not fit.")

  SECTION_DATA_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD),)
  {
    . = ALIGN (8);
    *(.noinit)
    *(".noinit.*")
    . = ALIGN (8);
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

ASSERT(((_iram_end - ORIGIN(iram0_0_seg)) <= LENGTH(iram0_0_seg)),
          "IRAM0 segment data does not fit.")
