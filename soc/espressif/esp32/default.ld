/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#include "memory.h"

/* User available SRAM memory segments */
procpu_iram0_end = USER_IRAM_END - APPCPU_SRAM_SIZE;
procpu_iram0_org = SRAM0_IRAM_START + SRAM0_CACHE_SIZE;
procpu_iram0_len = procpu_iram0_end - procpu_iram0_org;

procpu_dram0_end = SRAM2_DRAM_END;
procpu_dram0_org = SRAM2_DRAM_USER_START + CONFIG_ESP32_BT_RESERVE_DRAM;
procpu_dram0_len = SRAM2_DRAM_USER_SIZE - CONFIG_ESP32_BT_RESERVE_DRAM;

procpu_dram1_org = SRAM1_DRAM_USER_START;
procpu_dram1_len = SRAM1_DRAM_USER_SIZE;

procpu_irom_org = ICACHE0_START;
procpu_irom_len = ICACHE0_SIZE;
procpu_drom_org = DCACHE0_START;
procpu_drom_len = DCACHE0_SIZE;

#ifdef CONFIG_ESP_SPIRAM
procpu_ext_ram_org = DCACHE1_START;
procpu_ext_ram_len = DCACHE1_SIZE;
#endif

/* Aliases */
#define FLASH_CODE_REGION irom0_0_seg
#define RODATA_REGION     drom0_0_seg
#define IRAM_REGION       iram0_0_seg
#define DRAM_REGION       dram0_0_seg
#define RAMABLE_REGION    dram0_0_seg

#ifndef CONFIG_ESP32_REGION_1_NOINIT
#define RAMABLE_REGION_2  dram0_0_seg
#else
#define RAMABLE_REGION_2  dram1_0_seg
#endif

#define ROMABLE_REGION      FLASH

#undef GROUP_DATA_LINK_IN
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > lregion

#undef GROUP_NOLOAD_LINK_IN
#define GROUP_NOLOAD_LINK_IN(vregion, lregion) > vregion

/* Flash segments (rodata and text) should be mapped in virtual address space by providing VMA.
 * Executing directly from LMA is not possible. */
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > RODATA_REGION AT > lregion

/* Make sure new sections have consistent alignment between input and output sections */
#undef SECTION_DATA_PROLOGUE
#define SECTION_DATA_PROLOGUE(name, options, align) name options : ALIGN_WITH_INPUT

#undef SECTION_PROLOGUE
#define SECTION_PROLOGUE SECTION_DATA_PROLOGUE

MEMORY
{
#ifdef CONFIG_BOOTLOADER_MCUBOOT
  mcuboot_hdr (R): org = 0x0,  len = 0x20
  metadata (R):    org = 0x20, len = 0x60
  FLASH (R):       org = 0x80, len = FLASH_SIZE - 0x80
#else
  /* Make safety margin in the FLASH memory size so the
   * (esp_img_header + (n*esp_seg_headers)) would fit
   */
  FLASH (R):       org = 0x0,  len = FLASH_SIZE - 0x100
#endif /* CONFIG_BOOTLOADER_MCUBOOT */

  iram0_0_seg(RX): org = procpu_iram0_org, len = procpu_iram0_len
  dram0_0_seg(RW): org = procpu_dram0_org, len = procpu_dram0_len
  dram1_0_seg(RW): org = procpu_dram1_org, len = procpu_dram1_len

  irom0_0_seg(RX): org = procpu_irom_org, len = procpu_irom_len
  drom0_0_seg(R):  org = procpu_drom_org, len = procpu_drom_len

  rtc_iram_seg(RWX): org = 0x400c0000, len = 0x2000
  rtc_slow_seg(RW):  org = 0x50000000, len = 0x2000 - CONFIG_RESERVE_RTC_MEM
  rtc_data_seg(RW):  org = 0x3ff80000, len = 0x2000

  /* We reduced the size of rtc_slow_seg by CONFIG_RESERVE_RTC_MEM value.
   *  It reserves the amount of RTC slow memory that we use for this memory segment.
   *  This segment is intended for keeping rtc timer data (s_rtc_timer_retain_mem, see esp_clk.c files).
   *  The aim of this is to keep data that will not be moved around and have a fixed address.
   *  org = 0x50000000 + 0x2000 - CONFIG_RESERVE_RTC_MEM
   */
#if (CONFIG_RESERVE_RTC_MEM > 0)
  rtc_slow_reserved_seg(RW): org = 0x50000000 + 0x2000 - CONFIG_RESERVE_RTC_MEM,
                             len = CONFIG_RESERVE_RTC_MEM
#endif

#ifdef CONFIG_ESP_SPIRAM
  ext_ram_seg(RW): org = procpu_ext_ram_org, len = procpu_ext_ram_len
#endif /* CONFIG_ESP_SPIRAM */

#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif /* CONFIG_GEN_ISR_TABLES */
}

/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

_rom_store_table = 0;

PROVIDE(_memmap_vecbase_reset = 0x40000450);
PROVIDE(_memmap_reset_vector = 0x40000400);

/* Heap size calculations for PROCPU is also valid for AMP scenario */
_heap_sentry = SRAM2_DRAM_END;
_libc_heap_size = _heap_sentry - _end;

SECTIONS
{
#ifdef CONFIG_BOOTLOADER_MCUBOOT
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
    /* 0. Magic byte for load header */
    LONG(0xace637d3)

    /* 1. Application entry point address */
    KEEP(*(.entry_addr))

    /* IRAM load:
     * 2. Destination address (VMA) for IRAM region
     * 3. Flash offset (LMA) for start of IRAM region
     * 4. Size of IRAM region
     */
    LONG(ADDR(.iram0.vectors))
    LONG(LOADADDR(.iram0.vectors))
    LONG(LOADADDR(.iram0.text) + SIZEOF(.iram0.text) - LOADADDR(.iram0.vectors))

    /* DRAM load:
     * 5. Destination address (VMA) for DRAM region
     * 6. Flash offset (LMA) for start of DRAM region
     * 7. Size of DRAM region
     */
    LONG(ADDR(.dram0.data))
    LONG(LOADADDR(.dram0.data))
    LONG(LOADADDR(.dram0.end) + SIZEOF(.dram0.end) - LOADADDR(.dram0.data))

    /* RTC_IRAM metadata:
     * 8. Destination address (VMA) for RTC_IRAM region
     * 9. Flash offset (LMA) for start of RTC_IRAM region
     * 10. Size of RTC_IRAM region
     */
    LONG(ADDR(.rtc.text))
    LONG(LOADADDR(.rtc.text))
    LONG(SIZEOF(.rtc.text))

    /* RTC_DRAM metadata:
     * 11. Destination address (VMA) for RTC_DRAM region
     * 12. Flash offset (LMA) for start of RTC_DRAM region
     * 13. Size of RTC_DRAM region
     */
    LONG(ADDR(.rtc.data))
    LONG(LOADADDR(.rtc.data))
    LONG(SIZEOF(.rtc.data))

    /* IROM metadata:
     * 14. Destination address (VMA) for IROM region
     * 15. Flash offset (LMA) for start of IROM region
     * 16. Size of IROM region
     */
    LONG(ADDR(.flash.text))
    LONG(LOADADDR(.flash.text))
    LONG(SIZEOF(.flash.text))

    /* DROM metadata:
     * 17. Destination address (VMA) for DROM region
     * 18. Flash offset (LMA) for start of DROM region
     * 19. Size of DROM region
     */
    LONG(ADDR(.flash.rodata))
    LONG(LOADADDR(.flash.rodata))
    LONG(LOADADDR(.flash.rodata_end) - LOADADDR(.flash.rodata))
  } > metadata
#endif /* CONFIG_BOOTLOADER_MCUBOOT */

  #include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
  #include <zephyr/linker/llext-sections.ld>
#endif

  /* --- RTC BEGIN --- */

  .rtc.text :
  {
    . = ALIGN(4);
    *(.rtc.literal .rtc.literal.*)
    *(.rtc.text .rtc.text.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  /*
    This section is required to skip rtc.text area because rtc_iram_seg and
    rtc_data_seg reflect the same address space on different buses.
  */
  .rtc.dummy :
  {
    . = SIZEOF(.rtc.text);
  } GROUP_DATA_LINK_IN(rtc_data_seg, ROMABLE_REGION)

  /* This section located in RTC FAST Memory area.
     It holds data marked with RTC_FAST_ATTR attribute.
     See the file "esp_attr.h" for more information.
  */
  .rtc.force_fast :
  {
    . = ALIGN(4);
    *(.rtc.force_fast .rtc.force_fast.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(rtc_data_seg, ROMABLE_REGION)

  /* RTC data section holds RTC wake stub
     data/rodata, including from any source file
     named rtc_wake_stub*.c and the data marked with
     RTC_DATA_ATTR, RTC_RODATA_ATTR attributes.
  */
  .rtc.data :
  {
    . = ALIGN(4);
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data .rtc.data.*)
    *(.rtc.rodata .rtc.rodata.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *(.rtc.bss .rtc.bss.*)
    _rtc_bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(rtc_slow_seg)

  .rtc_noinit (NOLOAD) :
  {
    . = ALIGN(4);
    *(.rtc_noinit .rtc_noinit.*)
    . = ALIGN(4) ;
 } GROUP_LINK_IN(rtc_slow_seg)

  /* This section located in RTC SLOW Memory area.
   * It holds data marked with RTC_SLOW_ATTR attribute.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.force_slow :
  {
    . = ALIGN(4);
    *(.rtc.force_slow .rtc.force_slow.*)
    . = ALIGN(4) ;
    _rtc_force_slow_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  _rtc_slow_length = (_rtc_force_slow_end - _rtc_data_start);

  /**
   * This section holds RTC SLOW data that should have fixed addresses.
   * The data are not initialized at power-up and are retained during deep sleep.
   */
#if (CONFIG_RESERVE_RTC_MEM > 0)
  .rtc_slow_reserved (NOLOAD) :
  {
    . = ALIGN(4);
    _rtc_slow_reserved_start = ABSOLUTE(.);
    *(.rtc_timer_data_in_rtc_mem .rtc_timer_data_in_rtc_mem.*)
    _rtc_slow_reserved_end = ABSOLUTE(.);
  } GROUP_LINK_IN(rtc_slow_reserved_seg)
#endif

  /* --- RTC END --- */

  /* --- IRAM BEGIN --- */

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
    _iram_start = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

  .iram0.text : ALIGN(4)
  {
    /* Code marked as running out of IRAM */
    _iram_text_start = ABSOLUTE(.);
    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    *libarch__xtensa__core.a:(.literal .text .literal.* .text.*)
    *libkernel.a:(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libzephyr.a:windowspill_asm.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cbprintf_complete.*(.literal .text .literal.* .text.*)
    *libzephyr.a:printk.*(.literal.printk .literal.vprintk .literal.char_out .text.printk .text.vprintk .text.char_out)
    *libzephyr.a:log_noos.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_core.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_msg.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_list.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_output.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_backend_uart.*(.literal .text .literal.* .text.*)
    *libzephyr.a:loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_flash_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:console_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_random.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:hw_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:rtc_*.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cpu_util.*(.literal .text .literal.* .text.*)
    *libdrivers__flash.a:flash_esp32.*(.literal .text .literal.* .text.*)
    *libdrivers__timer.a:xtensa_sys_timer.*(.literal .text .literal.* .text.*)
    *libdrivers__console.a:uart_console.*(.literal.console_out .text.console_out)
    *liblib__libc__picolib.a:abort.*(.literal .text .literal.* .text.*)
    *liblib__libc__minimal.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__newlib.a:string.*(.literal .text .literal.* .text.*)
    *libc.a:*(.literal .text .literal.* .text.*)
    *libphy.a:( .phyiram .phyiram.*)
    *libgcov.a:(.literal .text .literal.* .text.*)

    /* [mapping:esp_psram] */
    *libzephyr.a:mmu_psram_flash.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_psram_impl_quad.*(.literal .literal.* .text .text.*)

    /* [mapping:hal] */
    *libzephyr.a:efuse_hal.*(.literal .literal.* .text .text.*)
    *libzephyr.a:mmu_hal.*(.literal .literal.* .text .text.*)
    *libzephyr.a:cache_utils.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cache_esp32.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cache_hal_esp32.*(.literal .text .literal.* .text.*)
    *libzephyr.a:ledc_hal_iram.*(.literal .literal.* .text .text.*)
    *libzephyr.a:i2c_hal_iram.*(.literal .literal.* .text .text.*)
    *libzephyr.a:wdt_hal_iram.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_hal_iram.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_encrypt_hal_iram.*(.literal .literal.* .text .text.*)

    /* [mapping:soc] */
    *libzephyr.a:lldesc.*(.literal .literal.* .text .text.*)

    /* [mapping:log] */
    *(.literal.esp_log_write .text.esp_log_write)
    *(.literal.esp_log_timestamp .text.esp_log_timestamp)
    *(.literal.esp_log_early_timestamp .text.esp_log_early_timestamp)
    *(.literal.esp_log_impl_lock .text.esp_log_impl_lock)
    *(.literal.esp_log_impl_lock_timeout .text.esp_log_impl_lock_timeout)
    *(.literal.esp_log_impl_unlock .text.esp_log_impl_unlock)

    /* [mapping:spi_flash] */
    *libzephyr.a:spi_flash_chip_boya.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_chip_gd.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_chip_generic.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_chip_issi.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_chip_mxic.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_chip_mxic_opi.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_chip_th.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_chip_winbond.*(.literal .literal.* .text .text.*)
    *libzephyr.a:memspi_host_driver.*(.literal .literal.* .text .text.*)
    *libzephyr.a:flash_brownout_hook.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_wrap.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_hpm_enable.*(.literal .literal.* .text .text.*)
    *libzephyr.a:flash_ops.*(.literal .literal.* .text .text.*)
    *libzephyr.a:flash_qio_mode.*(.literal .literal.* .text .text.*)

    /* [mapping:esp_system] */
    *libzephyr.a:esp_err.*(.literal .literal.* .text .text.*)
    *(.literal.esp_system_abort .text.esp_system_abort)

    /* [mapping:esp_hw_support] */
    *(.literal.esp_cpu_stall .text.esp_cpu_stall)
    *(.literal.esp_cpu_unstall .text.esp_cpu_unstall)
    *(.literal.esp_cpu_reset .text.esp_cpu_reset)
    *(.literal.esp_cpu_wait_for_intr .text.esp_cpu_wait_for_intr)
    *(.literal.esp_cpu_compare_and_set .text.esp_cpu_compare_and_set)
    *(.literal.esp_gpio_reserve_pins .text.esp_gpio_reserve_pins)
    *(.literal.esp_gpio_is_pin_reserved .text.esp_gpio_is_pin_reserved)
    *(.literal.rtc_vddsdio_get_config .text.rtc_vddsdio_get_config)
    *(.literal.rtc_vddsdio_set_config .text.rtc_vddsdio_set_config)
    *libzephyr.a:esp_memory_utils.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_init.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_clk.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_clk_init.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_sleep.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_time.*(.literal .literal.* .text .text.*)
    *libzephyr.a:systimer.*(.literal .literal.* .text .text.*)
    *libzephyr.a:mspi_timing_config.*(.literal .literal.* .text .text.*)
    *libzephyr.a:mspi_timing_tuning.*(.literal .literal.* .text .text.*)
    *libzephyr.a:periph_ctrl.*(.literal .text .literal.* .text.*)
    *libzephyr.a:regi2c_ctrl.*(.literal .text .literal.* .text.*)
    *(.literal.sar_periph_ctrl_power_enable .text.sar_periph_ctrl_power_enable)

    *libzephyr.a:esp_system_chip.*(.literal.esp_system_abort .text.esp_system_abort)
    *libzephyr.a:spi_hal_iram.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_slave_hal_iram.*(.literal .literal.* .text .text.*)
    *libzephyr.a:flash_brownout_hook.*(.literal .literal.* .text .text.*)
    *libzephyr.a:heap_caps_zephyr.*(.literal .literal.* .text .text.*)

    /* [mapping:soc_pm] */
    *(.literal.GPIO_HOLD_MASK .text.GPIO_HOLD_MASK)

    /* [mapping:esp_rom] */
    *libzephyr.a:esp_rom_spiflash.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_systimer.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_wdt.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_efuse.*(.literal .literal.* .text .text.*)

    /* [mapping:esp_mm] */
    *libzephyr.a:esp_cache.*(.literal .literal.* .text .text.*)
    *libzephyr.a:cache_esp32.*(.literal .literal.* .text .text.*)

    *libzephyr.a:bootloader_soc.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_disable .text.bootloader_random_disable)
    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_enable .text.bootloader_random_enable)

#if defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiextrairam .wifiextrairam.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiorslpiram .wifiorslpiram.* .wifiextrairam .wifiextrairam.*)
    *libcoexist.a:(.wifi_slp_iram  .wifi_slp_iram.* .coexiram .coexiram.* .coexsleepiram .coexsleepiram.*)

    /* [mapping:esp_wifi] */
    *(.literal.wifi_clock_enable_wrapper .text.wifi_clock_enable_wrapper)
    *(.literal.wifi_clock_disable_wrapper .text.wifi_clock_disable_wrapper)

    /* [mapping:esp_phy] */
    *(.literal.esp_phy_enable .text.esp_phy_enable)
    *(.literal.esp_phy_disable .text.esp_phy_disable)
    *(.literal.esp_wifi_bt_power_domain_off .text.esp_wifi_bt_power_domain_off)
#endif /* CONFIG_ESP32_WIFI_IRAM_OPT */

#if defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif /* CONFIG_ESP32_WIFI_RX_IRAM_OPT */

    . = ALIGN(4);

  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

#ifdef CONFIG_ESP_SIMPLE_BOOT
  .loader.text :
  {
    . =  ALIGN(4);
    *libzephyr.a:bootloader_clock_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_wdt.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_flash.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_clock_loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_random.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_efuse.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_utility.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_sha.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_panic.*(.literal .text .literal.* .text.*)

    *libzephyr.a:esp_image_format.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_encrypt.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_encryption_secure_features.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_partitions.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_hal.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_hal_common.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_flash_api.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_flash_spi_init.*(.literal .text .literal.* .text.*)

    *libzephyr.a:secure_boot.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot_secure_features.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot_signatures_bootloader.*(.literal .text .literal.* .text.*)

    *libzephyr.a:esp_efuse_table.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_fields.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_api.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_utility.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_api_key_esp32.*(.literal .text .literal.* .text.*)

    *libzephyr.a:app_cpu_start.*(.literal .text .literal.* .text.*)
    *libzephyr.a:mpu_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cpu_region_protect.*(.literal .text .literal.* .text.*)

    /* NOTE: to overcome the bug in esptool making Simple boot compatible image */
    . += 16;
    . = ALIGN(16);
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)
#endif /* CONFIG_ESP_SIMPLE_BOOT */

  /* Marks the end of IRAM code segment */
  .iram0.text_end (NOLOAD) :
  {
    /* ESP32 memprot requires 16B padding for possible CPU
     * prefetch and 256B alignment for PMS split lines */
    . += 16;
    . = ALIGN(0x100);
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
    _iram_bss_start = ABSOLUTE(.);
    *(.iram.bss)
    *(.iram.bss.*)
    _iram_bss_end = ABSOLUTE(.);

    . = ALIGN(4);
    _iram_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)

  /* This symbol is here to check SRAM1 usage in case it is used for data too */
  _iram_end_at_dram_addr = SRAM1_IRAM_DRAM_CALC(_iram_end);
  _unallocated_iram_memory_size = ORIGIN(iram0_0_seg) + LENGTH(iram0_0_seg) - _iram_end;

  ASSERT(((_iram_end - ORIGIN(iram0_0_seg)) <= LENGTH(iram0_0_seg)), "IRAM code does not fit.")

  /* --- IRAM END --- */

  /* --- DRAM BEGIN --- */

  .dram0.data :
  {
    _dram_data_start = ABSOLUTE(.);
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
    *libzephyr.a:loader.*(.rodata .rodata.*)
    *libzephyr.a:flash_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_flash_init.*(.rodata .rodata.*)
    *libzephyr.a:console_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_random.*(.rodata .rodata.*)
    *libzephyr.a:soc_init.*(.rodata .rodata.*)
    *libzephyr.a:hw_init.*(.rodata .rodata.*)
    *libdrivers__flash.a:flash_esp32.*(.rodata .rodata.*)
    *libdrivers__serial.a:uart_esp32.*(.rodata .rodata.*)
    *libzephyr.a:esp_memory_utils.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    /* [mapping:esp_psram] */
    *libzephyr.a:mmu_psram_flash.*(.rodata .rodata.*)
    *libzephyr.a:esp_psram_impl_quad.*(.rodata .rodata.*)

    /* [mapping:hal] */
    *libzephyr.a:efuse_hal.*(.rodata .rodata.*)
    *libzephyr.a:mmu_hal.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_hal_iram.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_encrypt_hal_iram.*(.rodata .rodata.*)
    *libzephyr.a:cache_utils.*(.rodata .rodata.*)
    *libzephyr.a:cache_esp32.*(.rodata .rodata.*)
    *libzephyr.a:cache_hal_esp32.*(.rodata .rodata.*)
    *libzephyr.a:ledc_hal_iram.*(.rodata .rodata.*)
    *libzephyr.a:i2c_hal_iram.*(.rodata .rodata.*)
    *libzephyr.a:wdt_hal_iram.*(.rodata .rodata.*)
    *libzephyr.a:systimer_hal.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_hal_gpspi.*(.rodata .rodata.*)

    /* [mapping:soc] */
    *libzephyr.a:lldesc.*(.rodata .rodata.*)

    /* [mapping:log] */
    *(.rodata.esp_log_write)
    *(.rodata.esp_log_timestamp)
    *(.rodata.esp_log_early_timestamp)
    *(.rodata.esp_log_impl_lock)
    *(.rodata.esp_log_impl_lock_timeout)
    *(.rodata.esp_log_impl_unlock)

    /* [mapping:spi_flash] */
    *libzephyr.a:spi_flash_chip_boya.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_gd.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_generic.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_issi.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_mxic.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_mxic_opi.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_th.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_winbond.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:memspi_host_driver.*(.rodata .rodata.*)
    *libzephyr.a:flash_brownout_hook.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_wrap.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_hpm_enable.*(.rodata .rodata.*)
    *libzephyr.a:flash_ops.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:flash_qio_mode.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    /* [mapping:esp_mm] */
    *libzephyr.a:esp_cache.*(.rodata .rodata.*)

    /* [mapping:esp_hw_support] */
    *(.rodata.esp_cpu_stall)
    *(.rodata.esp_cpu_unstall)
    *(.rodata.esp_cpu_reset)
    *(.rodata.esp_cpu_wait_for_intr)
    *(.rodata.esp_cpu_compare_and_set)
    *(.rodata.esp_gpio_reserve_pins)
    *(.rodata.esp_gpio_is_pin_reserved)
    *(.rodata.rtc_vddsdio_get_config)
    *(.rodata.rtc_vddsdio_set_config)
    *libzephyr.a:esp_memory_utils.*(.rodata .rodata.*)
    *libzephyr.a:rtc_clk.*(.rodata .rodata.*)
    *libzephyr.a:rtc_clk_init.*(.rodata .rodata.*)
    *libzephyr.a:systimer.*(.rodata .rodata.*)
    *libzephyr.a:mspi_timing_config.*(.rodata .rodata.*)
    *libzephyr.a:mspi_timing_tuning.*(.rodata .rodata.*)
    *(.rodata.sar_periph_ctrl_power_enable)

    *libzephyr.a:cache_esp32.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_cache.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_err.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:i2c_hal_iram.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:ledc_hal_iram.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:mmu_hal.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_encrypt_hal_iram.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal_iram.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal_common.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_hal_iram.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_slave_hal_iram.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:flash_brownout_hook.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:memspi_host_driver.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_wrap.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    /* TODO: fix other socs */
    *libzephyr.a:heap_caps_zephyr.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    /* [mapping:esp_rom] */
    *libzephyr.a:esp_rom_spiflash.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_rom_systimer.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_wdt.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_efuse.*(.rodata .rodata.*)

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)

    . = ALIGN(4);
    #include <snippets-rwdata.ld>
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_ESP_SIMPLE_BOOT
  .loader.data :
  {
    . = ALIGN(4);
    _loader_data_start = ABSOLUTE(.);
    *libzephyr.a:bootloader_esp32.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_clock_init.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_wdt.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_flash.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_efuse.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    *libzephyr.a:cpu_util.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:clk.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_clk.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:rtc_clk_init.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:rtc_time.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:cpu_region_protect.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    *libzephyr.a:periph_ctrl.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    *libzephyr.a:esp_flash_api.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_flash_spi_init.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    . = ALIGN(4);
    _loader_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(DRAM_REGION, ROMABLE_REGION)
#endif /* CONFIG_ESP_SIMPLE_BOOT */

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
    __data_end = ABSOLUTE(.);
    _data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  /* Shared RAM */
  .dram0.bss (NOLOAD) :
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.); /* required by bluetooth library */
    __bss_start = ABSOLUTE(.);

    _btdm_bss_start = ABSOLUTE(.);
    *libbtdm_app.a:(.bss .bss.* COMMON)
    . = ALIGN (4);
    _btdm_bss_end = ABSOLUTE(.);

    /* Buffer for system heap should be placed in dram0_0_seg */
    *libkernel.a:mempool.*(.noinit.kheap_buf__system_heap .noinit.*.kheap_buf__system_heap)

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

  .dram0.noinit (NOLOAD) :
  {
    . = ALIGN (4);
    __dram_noinit_start = ABSOLUTE(.);
#ifdef CONFIG_ESP32_WIFI_NET_ALLOC_SPIRAM
    *(EXCLUDE_FILE(
      *libdrivers__wifi.a:*
      *libsubsys__net__l2__ethernet.a:*
      *libsubsys__net__lib__config.a:*
      *libsubsys__net__ip.a:*
      *libsubsys__net.a:* ) .noinit)
    *(EXCLUDE_FILE(
      *libdrivers__wifi.a:*
      *libsubsys__net__l2__ethernet.a:*
      *libsubsys__net__lib__config.a:*
      *libsubsys__net__ip.a:*
      *libsubsys__net.a:* ) .noinit.*)
#else
    *(.noinit)
    *(.noinit.*)
#endif /* CONFIG_ESP32_WIFI_NET_ALLOC_SPIRAM */
    __dram_noinit_end = ABSOLUTE(.);
    . = ALIGN (4);
  } GROUP_LINK_IN(RAMABLE_REGION_2)

  /* Provide total SRAM usage, including IRAM and DRAM */
  _image_ram_start = _dram_data_start;
  #include <zephyr/linker/ram-end.ld>
  _image_ram_size += _iram_end - _init_start;

  ASSERT(((_end - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)), "DRAM data does not fit.")

  /* --- DRAM END --- */

  /* --- SPIRAM BEGIN --- */

  /* The SPIRAM section needs to be spilled before the dram0.noinit */
#ifdef CONFIG_ESP_SPIRAM
  .ext_ram (NOLOAD):
  {
    _ext_ram_start = ABSOLUTE(.);
    _ext_ram_noinit_start = ABSOLUTE(.);

#ifdef CONFIG_ESP32_WIFI_NET_ALLOC_SPIRAM
    *libdrivers__wifi.a:(.noinit .noinit.*)
    *libsubsys__net__l2__ethernet.a:(.noinit .noinit.*)
    *libsubsys__net__lib__config.a:(.noinit .noinit.*)
    *libsubsys__net__ip.a:(.noinit .noinit.*)
    *libsubsys__net.a:(.noinit .noinit.*)
#endif
    . = ALIGN(16);
    *(.ext_ram_noinit*)
    . = ALIGN(16);
    _ext_ram_noinit_end = ABSOLUTE(.);

    _ext_ram_bss_start = ABSOLUTE(.);
    *(.ext_ram.bss*)
    . = ALIGN(16);
    _ext_ram_bss_end = ABSOLUTE(.);

    _ext_ram_heap_start = ABSOLUTE(.);
    . = . + CONFIG_ESP_SPIRAM_HEAP_SIZE;
    . = ALIGN(16);
    _ext_ram_heap_end = ABSOLUTE(.);

    _ext_ram_end = ABSOLUTE(.);
  } GROUP_LINK_IN(ext_ram_seg)
#endif /* CONFIG_ESP_SPIRAM */

  /* --- SPIRAM END --- */

  /* --- RODATA BEGIN --- */

  .flash.rodata_dummy (NOLOAD) :
  {
    . = ALIGN(CACHE_ALIGN);
  } GROUP_LINK_IN(ROMABLE_REGION)

  _image_drom_start = LOADADDR(.flash.rodata);
  _image_drom_size = LOADADDR(.flash.rodata_end) - LOADADDR(.flash.rodata);
  _image_drom_vaddr = ADDR(.flash.rodata);

  .flash.rodata : ALIGN(CACHE_ALIGN)
  {
    _rodata_start = ABSOLUTE(.);
    _rodata_reserved_start = ABSOLUTE(.);

    . = ALIGN(4);
    #include <snippets-rodata.ld>
    . = ALIGN(4);

    *(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.* )

    _flash_rodata_start = ABSOLUTE(.);
    __rodata_region_start = ABSOLUTE(.);

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

    /* C++ exception handlers table:  */
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
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)

  #include <zephyr/linker/cplusplus-rom.ld>
  #include <zephyr/linker/common-rom/common-rom-init.ld>
  #include <zephyr/linker/common-rom/common-rom-kernel-devices.ld>
  #include <zephyr/linker/common-rom/common-rom-ztest.ld>
  #include <zephyr/linker/common-rom/common-rom-net.ld>
  #include <zephyr/linker/common-rom/common-rom-bt.ld>
  #include <zephyr/linker/common-rom/common-rom-debug.ld>
  #include <zephyr/linker/common-rom/common-rom-misc.ld>
  #include <zephyr/linker/thread-local-storage.ld>
  #include <snippets-sections.ld>

  /* Create an explicit section at the end of all the data that shall be mapped into drom.
   * This is used to calculate the size of the _image_drom_size variable */
  .flash.rodata_end :
  {
    /* This is a symbol marking the flash.rodata end, this
     * can be used for mmu driver to maintain virtual address
     * We don't need to include the noload rodata in this section
     */
    . = ALIGN(CONFIG_MMU_PAGE_SIZE);

    _rodata_end = ABSOLUTE(.);
    _rodata_reserved_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)

  /* --- RODATA END --- */

  /* --- FLASH TEXT BEGIN --- */

  .flash.text_dummy (NOLOAD):
  {
    . = ALIGN(CACHE_ALIGN);
  } GROUP_LINK_IN(ROMABLE_REGION)

  _image_irom_start = LOADADDR(.flash.text);
  _image_irom_size = LOADADDR(.flash.text) + SIZEOF(.flash.text) - _image_irom_start;
  _image_irom_vaddr = ADDR(.flash.text);

  .flash.text : ALIGN(CACHE_ALIGN)
  {
    _stext = .;
    _instruction_reserved_start = ABSOLUTE(.);  /* This is a symbol marking the flash.text start, this can be used for mmu driver to maintain virtual address */
    _text_start = ABSOLUTE(.);
    __text_region_start = ABSOLUTE(.);
    __rom_region_start = ABSOLUTE(.);

#ifndef CONFIG_ESP32_WIFI_IRAM_OPT
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiextrairam .wifiextrairam.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiorslpiram .wifiorslpiram.* .wifiextrairam .wifiextrairam.*)
    *libcoexist.a:(.wifi_slp_iram  .wifi_slp_iram.* .coexiram .coexiram.* .coexsleepiram .coexsleepiram.*)
#endif

#ifndef CONFIG_ESP32_WIFI_RX_IRAM_OPT
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif

    *(.fini.literal)
    *(.fini)

    *(.literal .text .literal.* .text.*)
    . = ALIGN(4);
    _text_end = ABSOLUTE(.);
    _instruction_reserved_end = ABSOLUTE(.);  /* This is a symbol marking the flash.text end, this can be used for mmu driver to maintain virtual address */
    __text_region_end = ABSOLUTE(.);
    __rom_region_end = ABSOLUTE(.);
    _etext = .;
  } GROUP_DATA_LINK_IN(FLASH_CODE_REGION, ROMABLE_REGION)

  /* --- FLASH TEXT END --- */

  /* --- XTENSA GLUE AND DEBUG BEGIN --- */

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

  /* --- XTENSA GLUE AND DEBUG END --- */

#ifdef CONFIG_ESP_SPIRAM
ASSERT(((_ext_ram_end - _ext_ram_start) <= CONFIG_ESP_SPIRAM_SIZE),
          "External SPIRAM overflowed.")
#endif /* CONFIG_ESP_SPIRAM */
