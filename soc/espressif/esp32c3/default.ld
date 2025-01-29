/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#include "memory.h"

/* The "user_iram_end" represents the 2nd stage bootloader
 * "iram_loader_seg" start address (that should not be overlapped).
 * If no bootloader is used, we can extend it to gain more user ram.
 */
#ifdef CONFIG_ESP_SIMPLE_BOOT
user_iram_end = (DRAM_BUFFERS_START + IRAM_DRAM_OFFSET);
#else
user_iram_end = BOOTLOADER_IRAM_LOADER_SEG_START;
#endif

/* User available SRAM memory segments */
user_iram_seg_org = SRAM1_IRAM_START;
user_dram_seg_org = SRAM1_DRAM_START;
user_dram_end = (user_iram_end - IRAM_DRAM_OFFSET);
user_idram_size = (user_dram_end - user_dram_seg_org);
user_iram_seg_len = user_idram_size;
user_dram_seg_len = user_idram_size;

/* Aliases */
#define FLASH_CODE_REGION  irom0_0_seg
#define RODATA_REGION      drom0_0_seg
#define IRAM_REGION        iram0_0_seg
#define DRAM_REGION        dram0_0_seg
#define RAMABLE_REGION     dram0_0_seg
#define ROMABLE_REGION     FLASH

#undef GROUP_DATA_LINK_IN
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > lregion

#undef GROUP_NOLOAD_LINK_IN
#define GROUP_NOLOAD_LINK_IN(vregion, lregion) > vregion

/* Flash segments (rodata and text) should be mapped in the virtual address spaces.
 * Executing directly from LMA is not possible. */
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > RODATA_REGION AT > lregion

/* Make sure new sections have consistent alignment between input and output sections */
#undef SECTION_DATA_PROLOGUE
#define SECTION_DATA_PROLOGUE(name, options, align) name options : ALIGN_WITH_INPUT

#undef SECTION_PROLOGUE
#define SECTION_PROLOGUE SECTION_DATA_PROLOGUE

/* Global symbols required for espressif hal build */
MEMORY
{
#ifdef CONFIG_BOOTLOADER_MCUBOOT
  mcuboot_hdr (R): org = 0x0,  len = 0x20
  metadata (R):    org = 0x20, len = 0x60
  FLASH (R):       org = 0x80, len = FLASH_SIZE - 0x80
#else
  /* Make safety margin in the FLASH memory size so the
   * (esp_img_header + (n*esp_seg_headers)) would fit */
  FLASH (R): org = 0x0, len = FLASH_SIZE - 0x100
#endif

  iram0_0_seg(RX): org = user_iram_seg_org, len = user_iram_seg_len
  dram0_0_seg(RW): org = user_dram_seg_org, len = user_dram_seg_len

  irom0_0_seg(RX): org = IROM_SEG_ORG, len = IROM_SEG_LEN
  drom0_0_seg (R): org = DROM_SEG_ORG, len = DROM_SEG_LEN

  rtc_iram_seg(RWX): org = 0x50000000, len = 0x2000 - CONFIG_RESERVE_RTC_MEM

  /* We reduced the size of rtc_iram_seg by CONFIG_RESERVE_RTC_MEM value.
     It reserves the amount of RTC fast memory that we use for this memory segment.
     This segment is intended for keeping:
       - (lower addr) rtc timer data (s_rtc_timer_retain_mem, see esp_clk.c files).
     The aim of this is to keep data that will not be moved around and have a fixed address.
  */
#if (CONFIG_RESERVE_RTC_MEM > 0)
  rtc_reserved_seg(RW): org = 0x50000000 + 0x2000 - CONFIG_RESERVE_RTC_MEM,
                        len = CONFIG_RESERVE_RTC_MEM
#endif

#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif
}

/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

_rom_store_table = 0;

_iram_dram_offset = IRAM_DRAM_OFFSET;

/* Heap size calculations */
_heap_sentry = DRAM_RESERVED_START;
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

    /* IRAM metadata:
     * 2. Destination address (VMA) for IRAM region
     * 3. Flash offset (LMA) for start of IRAM region
     * 4. Size of IRAM region
     */
    LONG(ADDR(.iram0.text))
    LONG(LOADADDR(.iram0.text))
    LONG(LOADADDR(.iram0.data) - LOADADDR(.iram0.text))

    /* DRAM metadata:
     * 5. Destination address (VMA) for DRAM region
     * 6. Flash offset (LMA) for start of DRAM region
     * 7. Size of DRAM region
     */
    LONG(ADDR(.dram0.data))
    LONG(LOADADDR(.dram0.data))
    LONG(LOADADDR(.dram0.end) - LOADADDR(.dram0.data))

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

  /* --- START OF RTC --- */

  /* RTC fast memory holds RTC wake stub code */
  .rtc.text :
  {
    . = ALIGN(4);
    *(.rtc.literal .rtc.literal.*)
    *(.rtc.text .rtc.text.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  /**
   * This section located in RTC FAST Memory area.
   * It holds data marked with RTC_FAST_ATTR attribute.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.force_fast :
  {
    . = ALIGN(4);
    *(.rtc.force_fast .rtc.force_fast.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  /**
   * RTC data section holds data marked with
   * RTC_DATA_ATTR, RTC_RODATA_ATTR attributes.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.data :
  {
    . = ALIGN(4);
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data .rtc.data.*)
    *(.rtc.rodata .rtc.rodata.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *(.rtc.bss .rtc.bss.*)
    _rtc_bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(rtc_iram_seg)

  /**
   * This section holds data that should not be initialized at power up
   * and will be retained during deep sleep.
   * User data marked with RTC_NOINIT_ATTR will be placed
   * into this section. See the file "esp_attr.h" for more information.
   */
  .rtc_noinit (NOLOAD) :
  {
    . = ALIGN(4);
    *(.rtc_noinit .rtc_noinit.*)
    . = ALIGN(4);
  } GROUP_LINK_IN(rtc_iram_seg)

  /* This section located in RTC SLOW Memory area.
   * It holds data marked with RTC_SLOW_ATTR attribute.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.force_slow :
  {
    . = ALIGN(4);
    *(.rtc.force_slow .rtc.force_slow.*)
    . = ALIGN(4);
    _rtc_force_slow_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

  /* Get size of rtc slow data */
  _rtc_slow_length = (_rtc_force_slow_end - _rtc_data_start);

 /**
   * This section holds RTC data that should have fixed addresses.
   * The data are not initialized at power-up and are retained during deep sleep.
   */
#if (CONFIG_RESERVE_RTC_MEM > 0)
  .rtc_reserved (NOLOAD) :
  {
    . = ALIGN(4);
    _rtc_reserved_start = ABSOLUTE(.);
    *(.rtc_timer_data_in_rtc_mem .rtc_timer_data_in_rtc_mem.*)
    _rtc_reserved_end = ABSOLUTE(.);
  } GROUP_LINK_IN(rtc_reserved_seg)
#endif

  /* --- END OF RTC --- */

  /* --- START OF IRAM --- */

  .iram0.text : ALIGN(4)
  {
    /* Vectors go to IRAM */
    _iram_start = ABSOLUTE(.);
    _init_start = ABSOLUTE(.);

    KEEP(*(.exception_vectors.text));
    . = ALIGN(256);

    _invalid_pc_placeholder = ABSOLUTE(.);

    KEEP(*(.exception.entry*)); /* contains _isr_wrapper */
    *(.exception.other*)
    . = ALIGN(4);

    *(.entry.text)
    *(.init.literal)
    *(.init)
    . = ALIGN(4);

    _init_end = ABSOLUTE(.);
    _iram_text_start = ABSOLUTE(.);

    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    *libzephyr.a:panic.*(.literal .text .literal.* .text.*)
    *libzephyr.a:loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_flash_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:console_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:hw_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_random.*(.literal .text .literal.* .text.*)

    *libarch__riscv__core.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__l2__ethernet.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__lib__config.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__ip.a:(.literal .text .literal.* .text.*)
    *libsubsys__net.a:(.literal .text .literal.* .text.*)
    *libkernel.a:(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libdrivers__flash.a:flash_esp32.*(.literal .text .literal.* .text.*)
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
    *liblib__libc__newlib.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__minimal.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__picolib.a:string.*(.literal .text .literal.* .text.*)
    *libzephyr.a:periph_ctrl.*(.literal .text .literal.* .text.*)
    *libgcov.a:(.literal .text .literal.* .text.*)
    *libphy.a:( .phyiram .phyiram.*)
    *libc.a:*(.literal .text .literal.* .text.*)

    /* [mapping:hal] */
    *libzephyr.a:mmu_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_hal_iram.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_encrypt_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cache_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:ledc_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:i2c_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:wdt_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:systimer_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_hal_gpspi.*(.literal .literal.* .text .text.*)

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
    *libzephyr.a:flash_ops.*(.literal .text .literal.* .text.*)

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
    *libzephyr.a:rtc_clk.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_clk_init.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_time.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_sleep.*(.literal .literal.* .text .text.*)
    *libzephyr.a:systimer.*(.literal .literal.* .text .text.*)
    *libzephyr.a:mspi_timing_config.*(.literal .literal.* .text .text.*)
    *libzephyr.a:mspi_timing_tuning.*(.literal .literal.* .text .text.*)
    *(.literal.sar_periph_ctrl_power_enable .text.sar_periph_ctrl_power_enable)

    /* [mapping:soc_pm] */
    *(.literal.GPIO_HOLD_MASK .text.GPIO_HOLD_MASK)

    /* [mapping:esp_rom] */
    *libzephyr.a:esp_rom_spiflash.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_systimer.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_wdt.*(.literal .literal.* .text .text.*)

    /* [mapping:esp_mm] */
    *libzephyr.a:esp_cache.*(.literal .literal.* .text .text.*)
    *libzephyr.a:cache_utils.*(.literal .text .literal.* .text.*)

    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_disable .text.bootloader_random_disable)
    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_enable .text.bootloader_random_enable)

#if defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiextrairam .wifiextrairam.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiextrairam .wifiextrairam.*)
    *libcoexist.a:(.wifi_slp_iram .wifi_slp_iram.*)

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

    . = ALIGN(4) + 16;

  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

#ifdef CONFIG_ESP_SIMPLE_BOOT
  .loader.text :
  {
    . =  ALIGN(4);
    _loader_text_start = ABSOLUTE(.);
    *libzephyr.a:bootloader_clock_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_wdt.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_flash.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_clock_loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_panic.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_random.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_efuse.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_utility.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_sha.*(.literal .text .literal.* .text.*)

    *libzephyr.a:esp_image_format.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_encrypt.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_encryption_secure_features.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_partitions.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_qio_mode.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_hal.*(.literal .literal.* .text .text.*)
    *libzephyr.a:spi_flash_hal_common.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_flash_api.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_flash_spi_init.*(.literal .text .literal.* .text.*)

    *libzephyr.a:esp_efuse_table.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_fields.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_api.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_utility.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_efuse_api_key_esp32xx.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot_secure_features.*(.literal .text .literal.* .text.*)
    *libzephyr.a:secure_boot_signatures_bootloader.*(.literal .text .literal.* .text.*)

    *libzephyr.a:cpu_region_protect.*(.literal .text .literal.* .text.*)

    /* ??? */
    *libzephyr.a:esp_gpio_reserve.*(.literal .text .literal.* .text.*)

    . = ALIGN(0x10) + 0x10;
    _loader_text_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)
#endif /* CONFIG_ESP_SIMPLE_BOOT */

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

  /* --- END OF IRAM --- */

  /* --- START OF DRAM --- */

  .dram0.dummy (NOLOAD):
  {
    /* Spacer section is required to skip .iram0.text area because
     * iram0_0_seg and dram0_0_seg reflect the same address space on different buses.
     */
    . = ORIGIN(dram0_0_seg) + MAX(_iram_end, user_iram_seg_org) - user_iram_seg_org;
    . = ALIGN(16) + 16;
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
    . = ALIGN(8);
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
    *libzephyr.a:periph_ctrl.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:loader.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_flash_init.*(.rodata .rodata.*)
    *libzephyr.a:console_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_init.*(.rodata .rodata.*)
    *libzephyr.a:hw_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_random.*(.rodata .rodata.*)

    *libzephyr.a:cache_utils.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:hal] */
    *libzephyr.a:mmu_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_encrypt_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:cache_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:ledc_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:i2c_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:wdt_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:systimer_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal_gpspi.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:soc] */
    *libzephyr.a:lldesc.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:log] */
    *(.rodata.esp_log_write)
    *(.rodata.esp_log_timestamp)
    *(.rodata.esp_log_early_timestamp)
    *(.rodata.esp_log_impl_lock)
    *(.rodata.esp_log_impl_lock_timeout)
    *(.rodata.esp_log_impl_unlock)

    /* [mapping:spi_flash] */
    *libzephyr.a:spi_flash_chip_boya.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_gd.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_generic.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_issi.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_mxic.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_mxic_opi.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_th.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_chip_winbond.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:memspi_host_driver.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_brownout_hook.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_wrap.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hpm_enable.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_ops.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_qio_mode.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:esp_mm] */
    *libzephyr.a:esp_cache.*(.rodata .rodata.* .srodata .srodata.*)

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
    *libzephyr.a:esp_memory_utils.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:rtc_clk.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:rtc_clk_init.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:systimer.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:mspi_timing_config.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:mspi_timing_tuning.*(.rodata .rodata.* .srodata .srodata.*)
    *(.rodata.sar_periph_ctrl_power_enable)

    /* [mapping:esp_system] */
    *libzephyr.a:esp_err.*(.rodata .rodata.*)
    *(.rodata.esp_system_abort)

    . = ALIGN(4);
    #include <snippets-rwdata.ld>
    . = ALIGN(4);

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_ESP_SIMPLE_BOOT
  /* Secondary loader sections */
  .loader.data :
  {
    . = ALIGN(4);
    _loader_data_start = ABSOLUTE(.);
    *libzephyr.a:bootloader_clock_init.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:bootloader_wdt.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:bootloader_flash.*(.srodata .srodata.* .rodata .rodata.*)
    *libzephyr.a:bootloader_clock_loader.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:bootloader_panic.*(.rodata .rodata.* .srodata .srodata.*)

    *libzephyr.a:cpu_region_protect.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:clk.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:esp_clk.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_mmap.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:flash_ops.*(.rodata .rodata.* .srodata .srodata.*)

    *libzephyr.a:esp_gpio_reserve.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal_common.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:esp_flash_api.*(.rodata .rodata.* .srodata .srodata.*)
    *libzephyr.a:esp_flash_spi_init.*(.rodata .rodata.* .srodata .srodata.*)

    . = ALIGN(16);
    _loader_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
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
    . = ALIGN(4);
    _data_end = ABSOLUTE(.);
    __data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  .dram0.noinit (NOLOAD):
  {
    . = ALIGN(4);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(4);
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Shared RAM */
  .dram0.bss (NOLOAD) :
  {
    . = ALIGN (8);
    __bss_start = ABSOLUTE(.);
    _bss_start = ABSOLUTE(.);

    /*  bluetooth library requires this symbol to be defined */
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
    . = ALIGN (16);
    __bss_end = ABSOLUTE(.);
    _bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Provide total SRAM usage, including IRAM and DRAM */
  _image_ram_start = _iram_start - IRAM_DRAM_OFFSET;
  #include <zephyr/linker/ram-end.ld>

  ASSERT(((_end - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)), "SRAM instruction/data does not fit.")

  /* --- END OF DRAM --- */

  /* --- START OF .flash.text --- */

  .flash.text_dummy (NOLOAD):
  {
    . = ALIGN(CACHE_ALIGN);
  } GROUP_LINK_IN(ROMABLE_REGION)

  /* Symbols used during the application memory mapping */
  _image_irom_start = LOADADDR(.flash.text);
  _image_irom_size = SIZEOF(.flash.text);
  _image_irom_vaddr = ADDR(.flash.text);

  .flash.text : ALIGN(CACHE_ALIGN)
  {
    _stext = .;
    _instruction_reserved_start = ABSOLUTE(.);
    _text_start = ABSOLUTE(.);
    _instruction_reserved_start = ABSOLUTE(.);
    __text_region_start = ABSOLUTE(.);
    __rom_region_start = ABSOLUTE(.);

#if !defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiextrairam .wifiextrairam.*)
    *libpp.a:( .wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiextrairam .wifiextrairam.*)
#endif /* CONFIG_ESP32_WIFI_IRAM_OPT */

#if !defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif /* CONFIG_ESP32_WIFI_RX_IRAM_OPT */

    *(.literal .text .literal.* .text.*)
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */

    *(.fini.literal)
    *(.fini)

    *(.gnu.version)

    /* CPU will try to prefetch up to 16 bytes of
     * of instructions. This means that any configuration (e.g. MMU, PMS) must allow
     * safe access to up to 16 bytes after the last real instruction, add
     * dummy bytes to ensure this
     */
    . += 16;

    _instruction_reserved_end = ABSOLUTE(.);
    _text_end = ABSOLUTE(.);
    _instruction_reserved_end = ABSOLUTE(.);
    __text_region_end = ABSOLUTE(.);
    __rom_region_end = ABSOLUTE(.);
    _etext = .;

  } GROUP_DATA_LINK_IN(FLASH_CODE_REGION, ROMABLE_REGION)

  /* --- END OF .flash.text --- */

  /* --- START OF .rodata --- */

  /* Align next section to 64k to allow mapping */
  .flash.dummy (NOLOAD) :
  {
    . = ALIGN(CACHE_ALIGN);
  } GROUP_LINK_IN(ROMABLE_REGION)

  .flash.rodata_dummy (NOLOAD) :
  {
    /* Spacer in the IROM address to avoid interfering with the DROM address
     * because DROM and IROM regions share the same address space */
    . += SIZEOF(.flash.text);
    . = ALIGN(CACHE_ALIGN);
  } GROUP_LINK_IN(RODATA_REGION)

  /* Symbols used during the application memory mapping */
  _image_drom_start = LOADADDR(.flash.rodata);
  _image_drom_size = _image_rodata_end - _image_rodata_start;
  _image_drom_vaddr = ADDR(.flash.rodata);

  .flash.rodata : ALIGN(0x10)
  {
    _rodata_reserved_start = ABSOLUTE(.);
    _image_rodata_start = ABSOLUTE(.);

    *(.rodata_desc .rodata_desc.*)
    *(.rodata_custom_desc .rodata_custom_desc.*)

    __rodata_region_start = ABSOLUTE(.);

    . = ALIGN(4);
    #include <snippets-rodata.ld>

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
    __rodata_region_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
    /* Literals are also RO data. */
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
    . = ALIGN(4);
    *(.srodata)
    *(.srodata.*)
    *(.rodata)
    *(.rodata.*)
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
  .flash.rodata_end : ALIGN(0x10)
  {
    . = ALIGN(4);
    _rodata_reserved_end = ABSOLUTE(.);
    _image_rodata_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)

  /* --- END OF .rodata --- */

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
