/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the ESP32S3 platform.
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define SRAM_IRAM_START     0x40370000
#define SRAM_DIRAM_I_START  0x40378000
/* SRAM_IRAM_END is equivalent 2nd stage bootloader iram_loader_seg
   start address (that should not be overlapped) */
#define SRAM_IRAM_END       0x403BA000
#define I_D_SRAM_OFFSET     (SRAM_DIRAM_I_START - SRAM_DRAM_START)

#define SRAM_DRAM_START     0x3FC88000
#define SRAM_DRAM_END       (SRAM_IRAM_END - I_D_SRAM_OFFSET)
#define I_D_SRAM_SIZE       (SRAM_DRAM_END - SRAM_DRAM_START)

#define ICACHE_SIZE         0x8000
#define SRAM_IRAM_ORG       (SRAM_IRAM_START + CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE)
#define SRAM_IRAM_SIZE      (I_D_SRAM_SIZE + ICACHE_SIZE - CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE)

#define DCACHE_SIZE         0x10000
#define SRAM_DRAM_ORG       (SRAM_DRAM_START)

#define DRAM0_0_SEG_LEN I_D_SRAM_SIZE

#define FLASH_CODE_REGION irom0_0_seg
#define RODATA_REGION drom0_0_seg
#define IRAM_REGION iram0_0_seg
#define RAMABLE_REGION dram0_0_seg
#define ROMABLE_REGION ROM

#define EXT_RAM_ORG (0x3E000000 - CONFIG_ESP_SPIRAM_SIZE)

#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE 0x800000
#endif

#ifdef CONFIG_BOOTLOADER_ESP_IDF
#define IROM_SEG_ORG 0x42000020
#define IROM_SEG_LEN FLASH_SIZE-0x20
#else
#define IROM_SEG_ORG 0x42000000
#define IROM_SEG_LEN FLASH_SIZE
#endif

#ifdef CONFIG_SOC_ESP32S3_PROCPU
#define APPCPU_IRAM_SIZE CONFIG_ESP32S3_APPCPU_IRAM
#define APPCPU_DRAM_SIZE CONFIG_ESP32S3_APPCPU_DRAM
#else
#define APPCPU_IRAM_SIZE 0x0
#define APPCPU_DRAM_SIZE 0x0
#endif

#define IROM_SEG_ALIGN 0x10000

/* Flash segments (rodata and text) should be mapped in virtual address space by providing VMA.
 * Executing directly from LMA is not possible. */
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > RODATA_REGION AT > lregion

MEMORY
{
  mcuboot_hdr (RX): org = 0x0, len = 0x20
  metadata (RX): org = 0x20, len = 0x20
  ROM (RX): org = 0x40, len = FLASH_SIZE - 0x40
  iram0_0_seg(RX): org = SRAM_IRAM_ORG, len = SRAM_IRAM_SIZE - APPCPU_IRAM_SIZE
  dram0_0_seg(RW): org = SRAM_DRAM_ORG, len = DRAM0_0_SEG_LEN - APPCPU_DRAM_SIZE

  irom0_0_seg(RX): org = IROM_SEG_ORG, len = IROM_SEG_LEN

  /* DROM is the first segment placed in generated binary.
   * MCUboot binary for ESP32 has image header of 0x20 bytes.
   * Additional load header of 0x20 bytes are appended to the image.
   * Hence, an offset of 0x40 is added to DROM segment origin.
   */
  drom0_0_seg(R): org = 0x3C000040, len = FLASH_SIZE - 0x40
  /**
   * `extern_ram_seg` and `drom0_0_seg` share the same bus and the address region.
   * so we allocate `extern_ram_seg` at the end of the address region.
   */
#if defined(CONFIG_ESP_SPIRAM)
  ext_ram_seg(RWX): org = EXT_RAM_ORG, len = CONFIG_ESP_SPIRAM_SIZE
#endif

  /* RTC fast memory (executable). Persists over deep sleep.
   */
  rtc_iram_seg(RWX): org = 0x600fe000, len = 0x2000

  /* RTC fast memory (same block as above), viewed from data bus
   */
  rtc_data_seg(RW): org = 0x600fe000, len = 0x2000

  /* RTC slow memory (data accessible). Persists over deep sleep.
   */
  rtc_slow_seg(RW): org = 0x50000000, len = 0x2000

#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif
}

/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
  /* Reserve space for MCUboot header in the binary */
  .mcuboot_header :
  {
    QUAD(0x0)
    QUAD(0x0)
    QUAD(0x0)
    QUAD(0x0)
  } > mcuboot_hdr
  .metadata :
  {
    /* Magic byte for load header */
    LONG(0xace637d3)

    /* Application entry point address */
    KEEP(*(.entry_addr))

    /* IRAM metadata:
     * - Destination address (VMA) for IRAM region
     * - Flash offset (LMA) for start of IRAM region
     * - Size of IRAM region
     */
    LONG(ADDR(.iram0.vectors))
    LONG(LOADADDR(.iram0.vectors))
    LONG(LOADADDR(.iram0.text) + SIZEOF(.iram0.text) - LOADADDR(.iram0.vectors))

    /* DRAM metadata:
     * - Destination address (VMA) for DRAM region
     * - Flash offset (LMA) for start of DRAM region
     * - Size of DRAM region
     */
    LONG(ADDR(.dram0.data))
    LONG(LOADADDR(.dram0.data))
    LONG(LOADADDR(.dram0.end) + SIZEOF(.dram0.end) - LOADADDR(.dram0.data))
  } > metadata

  #include <zephyr/linker/rel-sections.ld>

  _image_drom_start = LOADADDR(_RODATA_SECTION_NAME);
  _image_drom_size = LOADADDR(_RODATA_SECTION_END) + SIZEOF(_RODATA_SECTION_END)  - _image_drom_start;
  _image_drom_vaddr = ADDR(_RODATA_SECTION_NAME);

  /* NOTE: .rodata section should be the first section in the linker script and no
   * other section should appear before .rodata section. This is the requirement
   * to align ROM section to 64K page offset.
   * Adding .rodata as first section helps to reduce size of generated binary by
   * few kBs.
   */
  SECTION_PROLOGUE(_RODATA_SECTION_NAME,,ALIGN(0x10))
  {
    _rodata_start = ABSOLUTE(.);

    *(.rodata_desc .rodata_desc.*)               /* Should be the first.  App version info.        DO NOT PUT ANYTHING BEFORE IT! */
    *(.rodata_custom_desc .rodata_custom_desc.*) /* Should be the second. Custom app version info. DO NOT PUT ANYTHING BEFORE IT! */

    __rodata_region_start = ABSOLUTE(.);
    . = ALIGN(4);
    #include <snippets-rodata.ld>

    . = ALIGN(4);
    *(EXCLUDE_FILE (*libarch__xtensa__core.a:* *libkernel.a:fatal.* *libkernel.a:init.* *libzephyr.a:cbprintf_complete* *libzephyr.a:log_core.* *libzephyr.a:log_backend_uart.* *libzephyr.a:log_output.* *libzephyr.a:loader.* *libdrivers__serial.a:uart_esp32.*) .rodata)
    *(EXCLUDE_FILE (*libarch__xtensa__core.a:* *libkernel.a:fatal.* *libkernel.a:init.* *libzephyr.a:cbprintf_complete* *libzephyr.a:log_core.* *libzephyr.a:log_backend_uart.* *libzephyr.a:log_output.* *libzephyr.a:loader.* *libdrivers__serial.a:uart_esp32.*) .rodata.*)

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
    _thread_local_start = ABSOLUTE(.);
    *(.tdata)
    *(.tdata.*)
    *(.tbss)
    *(.tbss.*)
    *(.rodata_wlog)
    *(.rodata_wlog*)
    _thread_local_end = ABSOLUTE(.);
    _rodata_reserved_end = ABSOLUTE(.);
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)

  #include <zephyr/linker/common-rom/common-rom-cpp.ld>
  #include <zephyr/linker/common-rom/common-rom-kernel-devices.ld>
  #include <zephyr/linker/common-rom/common-rom-ztest.ld>
  #include <zephyr/linker/common-rom/common-rom-net.ld>
  #include <zephyr/linker/common-rom/common-rom-bt.ld>
  #include <zephyr/linker/common-rom/common-rom-debug.ld>
  #include <zephyr/linker/common-rom/common-rom-misc.ld>
  #include <snippets-sections.ld>

  /* Create an explicit section at the end of all the data that shall be mapped into drom.
   * This is used to calculate the size of the _image_drom_size variable */
  SECTION_PROLOGUE(_RODATA_SECTION_END,,ALIGN(0x10))
  {
    . = ALIGN(16);
    _image_rodata_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)

#if defined(CONFIG_ESP_SPIRAM)
  /* This section holds .ext_ram.bss data, and will be put in PSRAM */
  .ext_ram.bss (NOLOAD) :
  {
    _ext_ram_data_start = ABSOLUTE(.);
    _ext_ram_bss_start = ABSOLUTE(.);
    *(.ext_ram.bss*)
    . = ALIGN(4);
    _ext_ram_bss_end = ABSOLUTE(.);
  } > ext_ram_seg

  .ext_ram_noinit (NOLOAD) :
  {
#if defined(CONFIG_ESP32_WIFI_NET_ALLOC_SPIRAM)
    *libdrivers__wifi.a:(.noinit .noinit.*)
    *libsubsys__net__l2__ethernet.a:(.noinit .noinit.*)
    *libsubsys__net__lib__config.a:(.noinit .noinit.*)
    *libsubsys__net__ip.a:(.noinit .noinit.*)
    *libsubsys__net.a:(.noinit .noinit.*)
#endif
    _spiram_heap_start = ABSOLUTE(.);
    . = . + CONFIG_ESP_SPIRAM_HEAP_SIZE;

    _ext_ram_data_end = ABSOLUTE(.);
  } > ext_ram_seg
#endif

  /* Send .iram0 code to iram */
  .iram0.vectors : ALIGN(4)
  {
    _iram_start = ABSOLUTE(.);
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
    _init_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

  .iram0.text : ALIGN(4)
  {
    /* Code marked as running out of IRAM */
    _iram_text_start = ABSOLUTE(.);
    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    *libesp32.a:panic.*(.literal .text .literal.* .text.*)
    *librtc.a:(.literal .text .literal.* .text.*)
    *libarch__xtensa__core.a:(.literal .text .literal.* .text.*)
    *libkernel.a:(.literal .text .literal.* .text.*)
    *libsoc.a:rtc_*.*(.literal .text .literal.* .text.*)
    *libsoc.a:cpu_util.*(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spiram*.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_timing*.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash*.*(.literal .text .literal.* .text.*)
    *libdrivers__flash.a:flash_esp32.*(.literal .text .literal.* .text.*)
    *libzephyr.a:windowspill_asm.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_noos.*(.literal .text .literal.* .text.*)
    *libdrivers__timer.a:xtensa_sys_timer.*(.literal .text .literal.* .text.*)
    *libzephyr.a:systimer_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_core.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cbprintf_complete.*(.literal .text .literal.* .text.*)
    *libzephyr.a:printk.*(.literal.printk .literal.vprintk .literal.char_out .text.printk .text.vprintk .text.char_out)
    *libzephyr.a:log_msg.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_list.*(.literal .text .literal.* .text.*)
    *libdrivers__console.a:uart_console.*(.literal.console_out .text.console_out)
    *libzephyr.a:log_output.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_backend_uart.*(.literal .text .literal.* .text.*)
    *libzephyr.a:loader.*(.literal .text .literal.* .text.*)
    *liblib__libc__minimal.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__newlib.a:string.*(.literal .text .literal.* .text.*)
    *libc.a:*(.literal .text .literal.* .text.*)
    *libphy.a:(.phyiram .phyiram.*)
    *libgcov.a:(.literal .text .literal.* .text.*)

#if defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiorslpiram .wifiorslpiram.*)
#endif

#if defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif

    . = ALIGN(4) + 16;

  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

  /* Marks the end of IRAM code segment */
  .iram0.text_end (NOLOAD) :
  {
    /* ESP32-S3 memprot requires 16B padding for possible CPU
     * prefetch and 256B alignment for PMS split lines */
    . = ALIGN(16);
    _iram_text_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)

  .iram0.data :
  {
    . = ALIGN(16);
    *(.iram.data)
    *(.iram.data*)
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

  .iram0.bss (NOLOAD) :
  {
    . = ALIGN(16);
    *(.iram.bss)
    *(.iram.bss*)

    . = ALIGN(16);
    _iram_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)


  /* This section is required to skip .iram0.text area because iram0_0_seg and
   * dram0_0_seg reflect the same address space on different buses.
   */
  .dram0.dummy (NOLOAD):
  {
    . = ALIGN (8);
    . = ORIGIN(dram0_0_seg) + MAX(_iram_end, SRAM_DIRAM_I_START) - SRAM_DIRAM_I_START;
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Shared RAM */
  SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
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
  } GROUP_LINK_IN(RAMABLE_REGION)

  ASSERT(((__bss_end - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)), "DRAM segment data does not fit.")

  SECTION_DATA_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD),)
  {
    . = ALIGN(8);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(8) ;
  } GROUP_LINK_IN(RAMABLE_REGION)

  .dram0.data :
  {
    . = ALIGN (8);
    __data_start = ABSOLUTE(.);

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
    *libzephyr.a:systimer_hal.*(.rodata .rodata.*)
    *libzephyr.a:log_core.*(.rodata .rodata.*)
    *libzephyr.a:log_backend_uart.*(.rodata .rodata.*)
    *libzephyr.a:log_output.*(.rodata .rodata.*)
    *libzephyr.a:loader.*(.rodata .rodata.*)
    *libdrivers__serial.a:uart_esp32.*(.rodata .rodata.*)
    *libdrivers__flash.a:flash_esp32.*(.rodata  .rodata.*)

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  #include <zephyr/linker/cplusplus-rom.ld>
  #include <snippets-data-sections.ld>
  #include <zephyr/linker/common-ram.ld>
  #include <snippets-ram-sections.ld>
  #include <zephyr/linker/cplusplus-ram.ld>

  /* logging sections should be placed in RAM area to avoid flash cache disabled issues */
  #pragma push_macro("GROUP_ROM_LINK_IN")
  #undef GROUP_ROM_LINK_IN
  #define GROUP_ROM_LINK_IN GROUP_DATA_LINK_IN
  #include <zephyr/linker/common-rom/common-rom-logging.ld>
  #pragma pop_macro("GROUP_ROM_LINK_IN")

  .dram0.end :
  {
    . = ALIGN(4);

    #include <snippets-rwdata.ld>

    . = ALIGN(4);
    _end = ABSOLUTE(.);
    __data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  _image_irom_start = LOADADDR(.flash.text);
  _image_irom_size = LOADADDR(.flash.text) + SIZEOF(.flash.text) - _image_irom_start;
  _image_irom_vaddr = ADDR(.flash.text);

  .flash_text_dummy (NOLOAD): ALIGN(IROM_SEG_ALIGN)
  {
    . = SIZEOF(_RODATA_SECTION_NAME);
    . = ALIGN(IROM_SEG_ALIGN) + 0x20;
    _rodata_reserved_start = .;
  } GROUP_LINK_IN(FLASH_CODE_REGION)

  .flash.text : ALIGN(IROM_SEG_ALIGN)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);

#if !defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiorslpiram .wifiorslpiram.*)
#endif

#if !defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif

    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.fini.literal)
    *(.fini)
    *(.gnu.version)
    *(.literal .text .literal.* .text.*)

    /* CPU will try to prefetch up to 16 bytes of
     * of instructions. This means that any configuration (e.g. MMU, PMS) must allow
     * safe access to up to 16 bytes after the last real instruction, add
     * dummy bytes to ensure this
     */
    . += 16;

    _text_end = ABSOLUTE(.);
    _etext = .;

    /* Similar to _iram_start, this symbol goes here so it is
     * resolved by addr2line in preference to the first symbol in
     * the flash.text segment.
     */
    _flash_cache_start = ABSOLUTE(0);
  } GROUP_DATA_LINK_IN(FLASH_CODE_REGION, ROMABLE_REGION)

  /* RTC fast memory holds RTC wake stub code,
   * including from any source file named rtc_wake_stub*.c
   */
  .rtc.text :
  {
    . = ALIGN(4);
    _rtc_text_start = ABSOLUTE(.);
    *(.rtc.literal .rtc.text)
    *(.rtc.entry.text)
    *rtc_wake_stub*.*(.literal .text .literal.* .text.*)
    _rtc_text_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  /* This section is required to skip rtc.text area because rtc_iram_seg and
   * rtc_data_seg are reflect the same address space on different buses.
   */
  .rtc.dummy :
  {
     _rtc_dummy_start = ABSOLUTE(.);
     _rtc_fast_start = ABSOLUTE(.);
     . = SIZEOF(.rtc.text);
     _rtc_dummy_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_data_seg, ROMABLE_REGION)

  /* This section located in RTC FAST Memory area.
   * It holds data marked with RTC_FAST_ATTR attribute.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.force_fast :
  {
    . = ALIGN(4);
    _rtc_force_fast_start = ABSOLUTE(.);

    *(.rtc.force_fast .rtc.force_fast.*)
    . = ALIGN(4) ;
    _rtc_force_fast_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_data_seg, ROMABLE_REGION)

  /* RTC data section holds RTC wake stub
   * data/rodata, including from any source file
   * named rtc_wake_stub*.c and the data marked with
   * RTC_DATA_ATTR, RTC_RODATA_ATTR attributes.
   */
  .rtc.data :
  {
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data)
    *(.rtc.rodata)
    *rtc_wake_stub*.o(.data .rodata .data.* .rodata.* .bss .bss.*)
    _rtc_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  /* RTC bss, from any source file named rtc_wake_stub*.c */
  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *rtc_wake_stub*.*(.bss .bss.*)
    *rtc_wake_stub*.*(COMMON)

    *(.rtc.data)
    *(.rtc.rodata)

    _rtc_bss_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  /* RTC bss, from any source file named rtc_wake_stub*.c */
  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *rtc_wake_stub*.*(.bss .bss.*)
    *rtc_wake_stub*.*(COMMON)

    _rtc_bss_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  /* This section holds data that should not be initialized at power up
   * and will be retained during deep sleep.
   * User data marked with RTC_NOINIT_ATTR will be placed
   * into this section. See the file "esp_attr.h" for more information.
   */
  .rtc_noinit (NOLOAD):
  {
    . = ALIGN(4);
    _rtc_noinit_start = ABSOLUTE(.);
    *(.rtc_noinit .rtc_noinit.*)
    . = ALIGN(4) ;
    _rtc_noinit_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  /* This section located in RTC SLOW Memory area.
   * It holds data marked with RTC_SLOW_ATTR attribute.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.force_slow :
  {
    . = ALIGN(4);
    _rtc_force_slow_start = ABSOLUTE(.);
    *(.rtc.force_slow .rtc.force_slow.*)
    . = ALIGN(4) ;
    _rtc_force_slow_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  /* Get size of rtc slow data based on rtc_data_location alias */
  _rtc_slow_length = (_rtc_force_slow_end - _rtc_data_start);
  _rtc_fast_length = (_rtc_force_fast_end - _rtc_fast_start);

  ASSERT((_rtc_slow_length <= LENGTH(rtc_slow_seg)), "RTC_SLOW segment data does not fit.")
  ASSERT((_rtc_fast_length <= LENGTH(rtc_data_seg)), "RTC_FAST segment data does not fit.")

#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif

_heap_sentry = 0x3fceb910;

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

}

ASSERT(((_iram_end - ORIGIN(iram0_0_seg)) <= LENGTH(iram0_0_seg)),
          "IRAM0 segment data does not fit.")

#if defined(CONFIG_ESP_SPIRAM)
ASSERT(((_ext_ram_data_end - _ext_ram_data_start) <= CONFIG_ESP_SPIRAM_SIZE),
          "External SPIRAM overflowed.")
#endif /* CONFIG_ESP_SPIRAM */
