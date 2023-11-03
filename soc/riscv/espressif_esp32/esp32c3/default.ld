/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* ICache size is fixed to 16KB on ESP32-C3 */
#define ICACHE_SIZE         0x4000
#define SRAM_DRAM_START     0x3FC7C000
#define SRAM_IRAM_START     0x4037C000
/* SRAM_IRAM_END/SRAM_DRAM_END is equivalent
 * 2nd stage bootloader iram_loader_seg
 * start address (that should not be overlapped)
 * The DRAM end is also determined by the ROM code DRAM usage */
#define SRAM_IRAM_END       0x403DC000

/* I-D offset is used for I to D address conversion */
#define I_D_SRAM_OFFSET     (SRAM_IRAM_START - SRAM_DRAM_START)

#define SRAM_DRAM_ORG       (SRAM_DRAM_START + ICACHE_SIZE)
#define SRAM_IRAM_ORG       (SRAM_IRAM_START + ICACHE_SIZE)
#define SRAM_DRAM_END       (SRAM_IRAM_END - I_D_SRAM_OFFSET)

#define I_D_SRAM_SIZE       (SRAM_DRAM_END - SRAM_DRAM_ORG)
#define SRAM_IRAM_SIZE      I_D_SRAM_SIZE
#define SRAM_DRAM_SIZE      I_D_SRAM_SIZE

/* Naming conventions */ 
#define _RODATA_SECTION_END rodata_end
#define FLASH_CODE_REGION   irom0_0_seg
#define RODATA_REGION       drom0_0_seg
#define IRAM_REGION         iram0_0_seg
#define DRAM_REGION         dram0_0_seg

/* Make Zephyr ld scripts happy */
#define RAMABLE_REGION DRAM_REGION
#define ROMABLE_REGION ROM

#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE 0x400000
#endif /* CONFIG_FLASH_SIZE */

#ifdef CONFIG_BOOTLOADER_ESP_IDF
#define IROM_SEG_ORG 0x42000020
#define IROM_SEG_LEN (FLASH_SIZE-0x20)
#define IROM_SEG_ALIGN 0x4
#else
#define IROM_SEG_ORG 0x42000000
#define IROM_SEG_LEN FLASH_SIZE
#define IROM_SEG_ALIGN 0x10000
#endif /* CONFIG_BOOTLOADER_ESP_IDF */

#ifndef CONFIG_SIMPLE_BOOT
#define DROM_SEG_ORG 0x3C000040
#else
#define DROM_SEG_ORG 0x3C000000
#endif /* CONFIG_SIMPLE_BOOT */

/* Flash segments (rodata and text) should be mapped in virtual address space by providing VMA.
 * Executing directly from LMA is not possible. */
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > RODATA_REGION AT > lregion

/* Global symbols required for espressif hal build */
MEMORY
{
#ifndef CONFIG_SIMPLE_BOOT
  mcuboot_hdr (R): org = 0x0, len = 0x20
  metadata (R): org = 0x20, len = 0x20
#else
  ROM (R): org = 0x0, len = FLASH_SIZE - 0x100
#endif /* CONFIG_SIMPLE_BOOT */

  iram0_0_seg(RX): org = SRAM_IRAM_ORG, len = SRAM_IRAM_SIZE

  dram0_0_seg(RW): org = SRAM_DRAM_ORG, len = SRAM_DRAM_SIZE

  irom0_0_seg(RX): org = IROM_SEG_ORG,  len = IROM_SEG_LEN

  drom0_0_seg (R) : org = DROM_SEG_ORG, len = FLASH_SIZE - 0x40

  rtc_iram_seg(RWX): org = 0x50000000, len = 0x2000

#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif /* CONFIG_GEN_ISR_TABLES */
}

/* The line below defines location alias for .rtc.data section
 * As C3 only has RTC fast memory, this is not configurable like
 * on other targets.
 */
REGION_ALIAS("rtc_slow_seg", rtc_iram_seg);

/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

_rom_store_table = 0;

SECTIONS
{
  _i_d_sram_offset =  SRAM_IRAM_ORG - SRAM_DRAM_ORG;

#ifndef CONFIG_SIMPLE_BOOT 
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
    LONG(ADDR(.iram0.text))
    LONG(LOADADDR(.iram0.text))
    LONG(SIZEOF(.iram0.text))

    /* DRAM metadata:
     * - Destination address (VMA) for DRAM region
     * - Flash offset (LMA) for start of DRAM region
     * - Size of DRAM region
     */
    LONG(ADDR(.dram0.data))
    LONG(LOADADDR(.dram0.data))
    LONG(LOADADDR(.dram0.end) + SIZEOF(.dram0.end) - LOADADDR(.dram0.data))
  } > metadata
#endif /* CONFIG_SIMPLE_BOOT */

  #include <zephyr/linker/rel-sections.ld>

  _image_drom_start = LOADADDR(_RODATA_SECTION_NAME);
  _image_drom_size = LOADADDR(_RODATA_SECTION_END) + SIZEOF(_RODATA_SECTION_END)  - _image_drom_start;
  _image_drom_vaddr = ADDR(_RODATA_SECTION_NAME);
 
  SECTION_PROLOGUE(_RODATA_SECTION_NAME,,ALIGN(0x10))
  {
    _rodata_reserved_start = ABSOLUTE(.);
    _rodata_start = ABSOLUTE(.);

    *(.rodata_desc .rodata_desc.*)
    *(.rodata_custom_desc .rodata_custom_desc.*)

    __rodata_region_start = .;

    . = ALIGN(4);
    #include <snippets-rodata.ld>

    . = ALIGN(4);
    EXCLUDE_FILE (
      *libkernel.a:fatal.*
      *libkernel.a:init.*
      *libzephyr.a:loader.*
      *libzephyr.a:cbprintf_complete*
      *libzephyr.a:log_core.*
      *libzephyr.a:log_output.*
      *libzephyr.a:log_backend_uart.*
      *libzephyr.a:spi_flash_rom_patch.*
      *libdrivers__flash.a:esp32_mp.*
      *libdrivers__flash.a:flash_esp32.*
      *libdrivers__serial.a:uart_esp32.*
#ifdef CONFIG_SIMPLE_BOOT
      *libzephyr.a:clk.*
      *libzephyr.a:esp_clk.*
      *libzephyr.a:rtc_clk.*
      *libzephyr.a:rtc_clk_init.*
      *libzephyr.a:flash_mmap.*
      *libzephyr.a:flash_ops.*
      *libzephyr.a:flash_ops_esp32c3.*
      *libzephyr.a:cache_utils.*
      *libzephyr.a:esp_flash_api.*

      *libzephyr.a:bootloader_init.*
      *libzephyr.a:bootloader_init_common.*
      *libzephyr.a:bootloader_clock_init.*
      *libzephyr.a:bootloader_wdt.*
      *libzephyr.a:bootloader_mem.*
      *libzephyr.a:bootloader_flash.*
      *libzephyr.a:bootloader_flash_config_esp32c3.*
#endif /* CONFIG_SIMPLE_BOOT */
    ) *(.rodata .rodata.* .srodata .srodata.*);

    /* FIXME Allow the Zephyr dts symbols with __ prefix to pass in */
   /* *(.srodata.__*) */

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

    *(.rodata_wlog)
    *(.rodata_wlog*)
    *(.rodata_wlog.*)

    _thread_local_end = ABSOLUTE(.);
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
    _rodata_reserved_end = ABSOLUTE(.);
    . = ALIGN(16);
    _image_rodata_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)

  .iram0.text : ALIGN(4)
  {
    /* Vectors go to IRAM */
    _iram_start = ABSOLUTE(.);
    _init_start = ABSOLUTE(.);

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
    _init_end = ABSOLUTE(.);
    . = ALIGN(4);
    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    *libzephyr.a:panic.*(.literal .text .literal.* .text.*)
    *libzephyr.a:loader.*(.literal .text .literal.* .text.*)
    *librtc.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__l2__ethernet.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__lib__config.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__ip.a:(.literal .text .literal.* .text.*)
    *libsubsys__net.a:(.literal .text .literal.* .text.*)
    *libkernel.a:(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libdrivers__flash.a:flash_esp32.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_rom_patch.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_noos.*(.literal .text .literal.* .text.*)
    *libdrivers__timer.a:esp32c3_sys_timer.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_core.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cbprintf_complete.*(.literal .text .literal.* .text.*)
    *libzephyr.a:printk.*(.literal.printk .literal.vprintk .literal.char_out .text.printk .text.vprintk .text.char_out)
    *libzephyr.a:log_msg.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_list.*(.literal .text .literal.* .text.*)
    *libdrivers__console.a:uart_console.*(.literal.console_out .text.console_out)
    *libzephyr.a:log_output.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_backend_uart.*(.literal .text .literal.* .text.*)
    *libzephyr.a:rtc_*.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cpu_util.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cpu_util_esp32c3.*(.literal .text .literal.* .text.*)
    *liblib__libc__newlib.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__minimal.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__picolib.a:string.*(.literal .text .literal.* .text.*)
    *libc.a:*(.literal .text .literal.* .text.*)
    *libphy.a:( .phyiram .phyiram.*)
    *libgcov.a:(.literal .text .literal.* .text.*)

#ifdef CONFIG_SIMPLE_BOOT
    *(.fini.literal)
    *(.fini)
#endif

#if defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
#endif /* CONFIG_ESP32_WIFI_IRAM_OPT */

#if defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif /* CONFIG_ESP32_WIFI_RX_IRAM_OPT */

    . = ALIGN(16);
#ifndef CONFIG_SIMPLE_BOOT
    _init_end = ABSOLUTE(.);
#endif /* CONFIG_SIMPLE_BOOT */
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

#ifdef CONFIG_SIMPLE_BOOT 
  .loader.text :
  {
    . =  ALIGN(4);
    _loader_text_start = ABSOLUTE(.);
    *libzephyr.a:bootloader_soc.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_flash.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_flash_config_esp32c3.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_clock_loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_common_loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_mem.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_init_common.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_random.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_disable .text.bootloader_random_disable)
    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_enable .text.bootloader_random_enable)
    *libzephyr.a:bootloader_efuse_esp32c3.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_utility.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_sha.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_console.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_console_loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_panic.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_clock_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_wdt.*(.literal .text .literal.* .text.*)

    *libzephyr.a:esp_image_format.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_flash_apit.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cache_utils.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_ops.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_ops_esp32c3.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_encrypt.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_encryption_secure_features.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_partitions.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot_secure_features.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot_signatures_bootloader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:wdt_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_table.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_fields.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_api.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_utility.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_api_key_esp32xx.*(.literal .text .literal.* .text.*)

    *libzephyr.a:rtc_clk.*(.literal .text .literal.* .text.*)
    *libzephyr.a:rtc_time.*(.literal .text .literal.* .text.*)
    *libzephyr.a:rtc_clk_init.*(.literal .text .literal.* .text.*)

    . = ALIGN(16);
    _loader_text_end = ABSOLUTE(.);
    _init_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)
#endif /* CONFIG_SIMPLE_BOOT */

  .iram0.text_end (NOLOAD) :
  {
    /* C3 memprot requires 512 B alignment for split lines */
    . = ALIGN (16);
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

  .dram0.dummy (NOLOAD):
  {
    /**
    * This section is required to skip .iram0.text area because iram0_0_seg and
    * dram0_0_seg reflect the same address space on different buses.
    */
    . = ALIGN(16);
    . = ORIGIN(dram0_0_seg) + MAX(_iram_end, SRAM_IRAM_ORG) - SRAM_IRAM_ORG;
    . = ALIGN(16);
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Shared RAM */
  SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
  {
    . = ALIGN (8);
    __bss_start = ABSOLUTE(.);
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
    __bss_end = ABSOLUTE(.);
    _bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(RAMABLE_REGION)

  ASSERT(((__bss_end - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)), "DRAM segment data does not fit.")

  SECTION_DATA_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD),)
  {
    . = ALIGN(4);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(4);
  } GROUP_LINK_IN(RAMABLE_REGION)

  .dram0.data :
  {
    . = ALIGN(4);
    _data_start = ABSOLUTE(.);
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

    /* All dependent functions should be placed in DRAM to avoid issue
     * when flash cache is disabled */
    *libkernel.a:fatal.*(.rodata .rodata.* .srodata .srodata.*)
    *libkernel.a:init.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:cbprintf_complete*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:log_core.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:log_backend_uart.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:log_output.*(.rodata .rodata.* .srodata .srodata.*)
    *libdrivers__flash.a:flash_esp32.*(.rodata  .rodata.* .srodata .srodata.*)
    *libdrivers__serial.a:uart_esp32.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_rom_patch.*(.rodata  .rodata.* .srodata .srodata.*)
#ifndef CONFIG_SIMPLE_BOOT
    *libzephyr.a:loader.*(.rodata .rodata.* .srodata .srodata.*) 
#endif /* CONFIG_SIMPLE_BOOT */
    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_SIMPLE_BOOT 
  /* Secondary loader sections */
  .loader.data :
  {
    . = ALIGN(4);
    _loader_data_start = ABSOLUTE(.);

    *libzephyr.a:loader.*(.rodata .rodata.* .srodata .srodata.*) 

    *libzephyr.a:cpu_util_esp32c3.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:clk.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:esp_clk.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:rtc_clk.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:rtc_clk_init.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:cache_utils.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_mmap.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_ops.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_ops_esp32c3.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:esp_flash_api.*(.rodata .rodata.* .srodata .srodata.*)

    *libzephyr.a:bootloader_init.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:bootloader_init_common.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:bootloader_clock_init.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:bootloader_wdt.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:bootloader_flash.*(.srodata .srodata.* .rodata .rodata.*)
    *libzephyr.a:bootloader_flash_config_esp32c3.*(.rodata .rodata.* .srodata .srodata.*)
    . = ALIGN(4);
    _loader_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_SIMPLE_BOOT */

  #include <zephyr/linker/cplusplus-rom.ld>
  #include <zephyr/linker/thread-local-storage.ld>
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
    _data_end = ABSOLUTE(.);
    __data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  _image_irom_start = LOADADDR(.flash.text);
  _image_irom_size = LOADADDR(.flash.text) + SIZEOF(.flash.text) - _image_irom_start;
  _image_irom_vaddr = ADDR(.flash.text);

  .flash_text_dummy (NOLOAD): ALIGN(IROM_SEG_ALIGN)
  {
    . = SIZEOF(_RODATA_SECTION_NAME);
    #ifndef CONFIG_SIMPLE_BOOT
    . = ALIGN(0x10000) + 0x20;
    #else
    . = ALIGN(0x10000);
    #endif
  } GROUP_LINK_IN(FLASH_CODE_REGION)

  .flash.text : ALIGN(IROM_SEG_ALIGN)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);

#if !defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
#endif /* CONFIG_ESP32_WIFI_IRAM_OPT */

#if !defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif /* CONFIG_ESP32_WIFI_RX_IRAM_OPT */

    *(.literal .text .literal.* .text.*)
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
#ifndef CONFIG_SIMPLE_BOOT
    *(.fini.literal)
    *(.fini)
#endif
    *(.gnu.version)

    /** CPU will try to prefetch up to 16 bytes of
      * of instructions. This means that any configuration (e.g. MMU, PMS) must allow
      * safe access to up to 16 bytes after the last real instruction, add
      * dummy bytes to ensure this
      */
    . += 16;

    _text_end = ABSOLUTE(.);
    _etext = .;

    /**
     * Similar to _iram_start, this symbol goes here so it is
     * resolved by addr2line in preference to the first symbol in
     * the flash.text segment.
     */
    _flash_cache_start = ABSOLUTE(0);
  } GROUP_DATA_LINK_IN(FLASH_CODE_REGION, ROMABLE_REGION)

  .rtc.text :
  {
    . = ALIGN(4);
    *(.rtc.literal .rtc.text)
    *rtc_wake_stub*.o(.literal .text .literal.* .text.*)
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  /* This section is required to skip rtc.text area because the text and
   * data segments reflect the same address space on different buses.
   */
  .rtc.dummy (NOLOAD):
  {
    . = SIZEOF(.rtc.text);
  } GROUP_LINK_IN(rtc_iram_seg)

  .rtc.data :
  {
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data)
    *(.rtc.rodata)
    *rtc_wake_stub*.o(.data .rodata .data.* .rodata.* .bss .bss.*)
    _rtc_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *rtc_wake_stub*.o(.bss .bss.*)
    *rtc_wake_stub*.o(COMMON)
    _rtc_bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(rtc_iram_seg)

  /**
   * This section located in RTC SLOW Memory area.
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
  } > rtc_slow_seg

  /* Get size of rtc slow data */
  _rtc_slow_length = (_rtc_force_slow_end - _rtc_data_start);

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
