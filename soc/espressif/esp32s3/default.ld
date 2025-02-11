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
user_iram_end = (BOOTLOADER_USER_DRAM_END + IRAM_DRAM_OFFSET);
#else
user_iram_end = BOOTLOADER_IRAM_LOADER_SEG_START;
#endif

/* User available SRAM memory segments */
user_dram_seg_org = SRAM1_DRAM_START;
user_iram_seg_org = SRAM0_IRAM_START + CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE;
user_dram_end = BOOTLOADER_IRAM_LOADER_SEG_START - IRAM_DRAM_OFFSET;
user_idram_size = user_dram_end - SRAM1_DRAM_START;
sram0_iram_size = SRAM0_SIZE - CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE;
user_iram_seg_len = user_idram_size + sram0_iram_size;
user_dram_seg_len = user_idram_size;

/* Aliases */
#define FLASH_CODE_REGION  irom0_0_seg
#define RODATA_REGION      drom0_0_seg
#define IRAM_REGION        iram0_0_seg
#define RAMABLE_REGION     dram0_0_seg
#define ROMABLE_REGION     FLASH

/* Flash segments (rodata and text) should be mapped in the virtual address spaces.
 * Executing directly from LMA is not possible. */
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > RODATA_REGION AT > lregion

/* Make sure new sections have consistent alignment between input and output sections */
#undef SECTION_PROLOGUE
#define SECTION_PROLOGUE SECTION_DATA_PROLOGUE

MEMORY
{
#ifdef CONFIG_BOOTLOADER_MCUBOOT
  mcuboot_hdr (R):  org = 0x0,  len = 0x20
  metadata (R):     org = 0x20, len = 0x20
  FLASH (R):        org = 0x40, len = FLASH_SIZE - 0x40
#else
  /* Make safety margin in the FLASH memory size so the
   * (esp_img_header + (n*esp_seg_headers)) would fit */
  FLASH (R):        org = 0x0, len = FLASH_SIZE - 0x100
#endif /* CONFIG_BOOTLOADER_MCUBOOT */

  iram0_0_seg(RX): org = user_iram_seg_org, len = user_iram_seg_len - APPCPU_IRAM_SIZE
  dram0_0_seg(RW): org = user_dram_seg_org, len = user_dram_seg_len - APPCPU_DRAM_SIZE

  irom0_0_seg(RX): org = IROM_SEG_ORG, len = IROM_SEG_LEN
  drom0_0_seg(R):  org = DROM_SEG_ORG, len = DROM_SEG_LEN

  /**
   * `ext_ram_seg` and `drom0_0_seg` share the same bus and the address region.
   * A dummy section is used to avoid overlap. See `.ext_ram.dummy` in `sections.ld.in`
   */
#if defined(CONFIG_ESP_SPIRAM)
  ext_ram_seg(RWX): org = DROM_SEG_ORG, len = CONFIG_ESP_SPIRAM_SIZE - 0x40
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

/* Used as a pointer to the heap end */
_heap_sentry = DRAM_BUFFERS_START;

SECTIONS
{
  _iram_dram_offset =  IRAM_DRAM_OFFSET;

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
    LONG(ADDR(.iram0.vectors))
    LONG(LOADADDR(.iram0.vectors))
    LONG(LOADADDR(.iram0.text) + SIZEOF(.iram0.text) - LOADADDR(.iram0.vectors))

    /* DRAM metadata:
     * 5. Destination address (VMA) for DRAM region
     * 6. Flash offset (LMA) for start of DRAM region
     * 7. Size of DRAM region
     */
    LONG(ADDR(.dram0.data))
    LONG(LOADADDR(.dram0.data))
    LONG(LOADADDR(.dram0.end) + SIZEOF(.dram0.end) - LOADADDR(.dram0.data))
  } > metadata
#endif /* CONFIG_BOOTLOADER_MCUBOOT */


/* Virtual non-loadable sections */
#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

  /* --- START OF RTC --- */

  /* RTC fast memory holds RTC wake stub code */
  .rtc.text :
  {
    . = ALIGN(4);
    _rtc_fast_start = ABSOLUTE(.);
    _rtc_text_start = ABSOLUTE(.);
    *(.rtc.literal .rtc.text)
    *(.rtc.entry.text)
    _rtc_text_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_iram_seg, ROMABLE_REGION)

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

  /* RTC data section holds data marked with
   * RTC_DATA_ATTR, RTC_RODATA_ATTR attributes.
   */
  .rtc.data :
  {
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data)
    *(.rtc.rodata)
    _rtc_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);

    *(.rtc.data)
    *(.rtc.rodata)

    _rtc_bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(rtc_slow_seg)

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
  } GROUP_LINK_IN(rtc_slow_seg)

  /* This section located in RTC SLOW Memory area.
   * It holds data marked with RTC_SLOW_ATTR attribute.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.force_slow :
  {
    . = ALIGN(4);
    _rtc_force_slow_start = ABSOLUTE(.);
    *(.rtc.force_slow .rtc.force_slow.*)
    . = ALIGN(4);
    _rtc_force_slow_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(rtc_slow_seg, ROMABLE_REGION)

  /* Get size of rtc slow data based on rtc_data_location alias */
  _rtc_slow_length = (_rtc_force_slow_end - _rtc_data_start);
  _rtc_fast_length = (_rtc_force_fast_end - _rtc_fast_start);

  ASSERT((_rtc_slow_length <= LENGTH(rtc_slow_seg)), "RTC_SLOW segment data does not fit.")
  ASSERT((_rtc_fast_length <= LENGTH(rtc_data_seg)), "RTC_FAST segment data does not fit.")

  /* --- END OF RTC --- */

  /* --- START OF IRAM --- */

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
    *libarch__xtensa__core.a:(.literal .text .literal.* .text.*)
    *libkernel.a:(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cbprintf_packaged.*(.literal .text .literal.* .text.*)
    *libdrivers__flash.a:flash_esp32.*(.literal .text .literal.* .text.*)
    *libzephyr.a:windowspill_asm.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_noos.*(.literal .text .literal.* .text.*)
    *libdrivers__timer.a:xtensa_sys_timer.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_core.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cbprintf_complete.*(.literal .text .literal.* .text.*)
    *libzephyr.a:printk.*(.literal.printk .literal.vprintk .literal.char_out .text.printk .text.vprintk .text.char_out)
    *libzephyr.a:log_msg.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_list.*(.literal .text .literal.* .text.*)
    *libdrivers__console.a:uart_console.*(.literal.console_out .text.console_out)
    *libzephyr.a:log_output.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_backend_uart.*(.literal .text .literal.* .text.*)
    *libzephyr.a:log_minimal.*(.literal .literal.* .text .text.*)
    *libzephyr.a:loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_mmu_map.*(.literal .literal.* .text .text.*)
    *libdrivers__interrupt_controller.a:(.literal .literal.* .text .text.*)
    *liblib__libc__minimal.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__newlib.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__picolibc.a:string.*(.literal .text .literal.* .text.*)
    *libphy.a:(.phyiram .phyiram.*)
    *libgcov.a:(.literal .text .literal.* .text.*)

    /* [mapping:esp_psram] */
    *libzephyr.a:mmu_psram_flash.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_psram_impl_quad.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_psram_impl_octal.*(.literal .literal.* .text .text.*)

    /* [mapping:hal] */
    *libzephyr.a:mmu_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_encrypt_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cache_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:ledc_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:i2c_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:wdt_hal_iram.*(.literal .text .literal.* .text.*)
    *libzephyr.a:systimer_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:spi_flash_hal_gpspi.*(.literal .text .literal.* .text.*)

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
    *libzephyr.a:spi_flash_oct_flash_init*(.literal .literal.* .text .text.*)

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
    *libzephyr.a:rtc_clk_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:rtc_sleep.*(.literal .literal.* .text .text.*)
    *libzephyr.a:rtc_time.*(.literal .literal.* .text .text.*)
    *libzephyr.a:systimer.*(.literal .literal.* .text .text.*)
    *libzephyr.a:mspi_timing_config.*(.literal .literal.* .text .text.*)
    *libzephyr.a:mspi_timing_tuning.*(.literal .literal.* .text .text.*)
    *(.literal.sar_periph_ctrl_power_enable .text.sar_periph_ctrl_power_enable)

    /* [mapping:soc_pm] */
    *(.literal.GPIO_HOLD_MASK .text.GPIO_HOLD_MASK)

    /* [mapping:esp_rom] */
    *libzephyr.a:esp_rom_cache_esp32s2_esp32s3.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_spiflash.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_systimer.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_wdt.*(.literal .literal.* .text .text.*)

    /* [mapping:esp_mm] */
    *libzephyr.a:esp_cache.*(.literal .literal.* .text .text.*)

    *libzephyr.a:bootloader_soc.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_disable .text.bootloader_random_disable)
    *libzephyr.a:bootloader_random*.*(.literal.bootloader_random_enable .text.bootloader_random_enable)

#if defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    *libnet80211.a:(.wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.*)
    *libpp.a:(.wifi0iram  .wifi0iram.* .wifislpiram .wifislpiram.* .wifiorslpiram .wifiorslpiram.*)
    *libcoexist.a:(.wifi_slp_iram  .wifi_slp_iram.*)

    /* [mapping:esp_wifi] */
    *(.literal.wifi_clock_enable_wrapper .text.wifi_clock_enable_wrapper)
    *(.literal.wifi_clock_disable_wrapper .text.wifi_clock_disable_wrapper)

    /* [mapping:esp_phy] */
    *(.literal.esp_phy_enable .text.esp_phy_enable)
    *(.literal.esp_phy_disable .text.esp_phy_disable)
    *(.literal.esp_wifi_bt_power_domain_off .text.esp_wifi_bt_power_domain_off)
#endif

#if defined(CONFIG_ESP32_WIFI_RX_IRAM_OPT)
    *libnet80211.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
    *libpp.a:( .wifirxiram  .wifirxiram.* .wifislprxiram .wifislprxiram.*)
#endif
    . = ALIGN(4);

  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

#ifdef CONFIG_ESP_SIMPLE_BOOT
  .loader.text :
  {
    . =  ALIGN(4);
    _loader_text_start = ABSOLUTE(.);
    *libzephyr.a:bootloader_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_esp32s3.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_clock_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_wdt.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_flash.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_flash_config_esp32s3.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_clock_loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_common_loader.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_mem.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_random.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_efuse.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_utility.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_sha.*(.literal .text .literal.* .text.*)
    *libzephyr.a:bootloader_console.*(.literal .text .literal.* .text.*)
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
    *libzephyr.a:esp_efuse_api_key_esp32xx.*(.literal .text .literal.* .text.*)
    *libzephyr.a:mpu_hal.*(.literal .text .literal.* .text.*)
    *libzephyr.a:cpu_region_protect.*(.literal .text .literal.* .text.*)

    *(.fini.literal)
    *(.fini)

    . = ALIGN(4);
    _loader_text_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)
#endif /* CONFIG_ESP_SIMPLE_BOOT */

  /* Marks the end of IRAM code segment */
  .iram0.text_end (NOLOAD) :
  {
    /* ESP32-S3 memprot requires 16B padding for possible CPU prefetch and 256B alignment for PMS split lines */
    . = ALIGN(4) + 16;
    _iram_text_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)

  .iram0.data :
  {
    . = ALIGN(4);
    _iram_data_start = ABSOLUTE(.);
    *(.iram.data)
    *(.iram.data*)
    _iram_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

  .iram0.bss (NOLOAD) :
  {
    . = ALIGN(4);
    _iram_bss_start = ABSOLUTE(.);
    *(.iram.bss)
    *(.iram.bss*)
    _iram_bss_end = ABSOLUTE(.);
    . = ALIGN(4);
    _iram_end = ABSOLUTE(.);
  } GROUP_LINK_IN(IRAM_REGION)

  /* --- END OF IRAM --- */

  /* --- START OF DRAM --- */

  .dram0.dummy (NOLOAD):
  {
    /* Spacer section is required to skip .iram0.text area because
     * iram0_0_seg and dram0_0_seg reflect the same address space on different buses.
     */
    . = ORIGIN(dram0_0_seg) + MAX(_iram_end, SRAM1_IRAM_START) - SRAM1_IRAM_START;
    . = ALIGN(4) + 16;
  } GROUP_LINK_IN(RAMABLE_REGION)

  .dram0.data :
  {
    . = ALIGN (8);
    _data_start = ABSOLUTE(.);
    __data_start = ABSOLUTE(.);

    /*  bluetooth library requires this symbol to be defined */
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
    *(.srodata)
    *(.srodata.*)
    /* rodata for panic handler(libarch__xtensa__core.a) and all
     * dependent functions should be placed in DRAM to avoid issue
     * when flash cache is disabled */
    *libarch__xtensa__core.a:(.rodata .rodata.*)
    *libkernel.a:fatal.*(.rodata .rodata.*)
    *libkernel.a:init.*(.rodata .rodata.*)
    *libzephyr.a:cbprintf_complete.*(.rodata .rodata.*)
    *libzephyr.a:log_core.*(.rodata .rodata.*)
    *libzephyr.a:log_backend_uart.*(.rodata .rodata.*)
    *libzephyr.a:log_output.*(.rodata .rodata.*)
    *libzephyr.a:log_minimal.*(.rodata .rodata.*)
    *libzephyr.a:loader.*(.rodata .rodata.*)
    *libdrivers__serial.a:uart_esp32.*(.rodata .rodata.*)
    *libdrivers__flash.a:flash_esp32.*(.rodata .rodata.*)
    *libzephyr.a:esp_mmu_map.*(.rodata .rodata.*)
    *libdrivers__interrupt_controller.a:(.rodata .rodata.*)

    /* [mapping:esp_psram] */
    *libzephyr.a:mmu_psram_flash.*(.rodata .rodata.*)
    *libzephyr.a:esp_psram_impl_octal.*(.rodata .rodata.*)
    *libzephyr.a:esp_psram_impl_quad.*(.rodata .rodata.*)

    /* [mapping:hal] */
    *libzephyr.a:mmu_hal.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_hal_iram.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_encrypt_hal_iram.*(.rodata .rodata.*)
    *libzephyr.a:cache_hal.*(.rodata .rodata.*)
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
    *libzephyr.a:spi_flash_chip_boya.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_chip_gd.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_chip_generic.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_chip_issi.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_chip_mxic.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_chip_mxic_opi.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_chip_th.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_chip_winbond.*(.rodata .rodata.*)
    *libzephyr.a:memspi_host_driver.*(.rodata .rodata.*)
    *libzephyr.a:flash_brownout_hook.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_wrap.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_hpm_enable.*(.rodata .rodata.*)
    *libzephyr.a:spi_flash_oct_flash_init.*(.rodata .rodata.*)

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

    /* [mapping:soc_pm] */
    *(.rodata.GPIO_HOLD_MASK)

    /* [mapping:esp_rom] */
    *libzephyr.a:esp_rom_cache_esp32s2_esp32s3.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_spiflash.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_systimer.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_wdt.*(.rodata .rodata.*)

    /* [mapping:esp_system] */
    *libzephyr.a:esp_err.*(.rodata .rodata.*)
    *(.rodata.esp_system_abort)

#if defined(CONFIG_ESP32_WIFI_IRAM_OPT)
    /* [mapping:esp_wifi] */
    *(.rodata.wifi_clock_enable_wrapper)
    *(.rodata.wifi_clock_disable_wrapper)

    /* [mapping:esp_phy] */
    *(.rodata.esp_phy_enable)
    *(.rodata.esp_phy_disable)
    *(.rodata.esp_wifi_bt_power_domain_off)
#endif

    . = ALIGN(4);
    #include <snippets-rwdata.ld>
    . = ALIGN(4);

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    . = ALIGN(4);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_ESP_SIMPLE_BOOT
  .loader.data :
  {
    . = ALIGN(4);
    _loader_data_start = ABSOLUTE(.);
    *libzephyr.a:bootloader_init.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_esp32s3.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_clock_init.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_wdt.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_flash.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_flash_config_esp32s3.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:bootloader_efuse.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    *libzephyr.a:cpu_util.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:clk.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_clk.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:cpu_region_protect.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    *libzephyr.a:spi_flash_hal.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:spi_flash_hal_common.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_flash_api.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:esp_flash_spi_init.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

    . = ALIGN(4);
    _loader_data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_ESP_SIMPLE_BOOT */

  #include <snippets-data-sections.ld>
  #include <zephyr/linker/common-ram.ld>
  #include <snippets-ram-sections.ld>
  #include <zephyr/linker/cplusplus-ram.ld>
  #include <zephyr/linker/kobject-data.ld>

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

  .dram0.noinit (NOLOAD):
  {
    . = ALIGN(4);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(4) ;
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Shared RAM */
  .dram0.bss (NOLOAD) :
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.); /* required by bluetooth library */
    __bss_start = ABSOLUTE(.);

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
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    __bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(RAMABLE_REGION)

  .dram0.heap_start (NOLOAD) :
  {
    . = ALIGN (8);
    /* Lowest possible start address for the heap */
    _heap_start = ABSOLUTE(.);
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Provide total SRAM usage, including IRAM and DRAM */
  _image_ram_start = _iram_start - IRAM_DRAM_OFFSET;
  #include <zephyr/linker/ram-end.ld>

  ASSERT(((__bss_end - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)), "DRAM segment data does not fit.")

  /* --- END OF DRAM --- */

  /* --- START OF IROM --- */

  /* Symbols used during the application memory mapping */
  _image_irom_start = LOADADDR(.flash.text);
  _image_irom_size = LOADADDR(.flash.text) + SIZEOF(.flash.text) - _image_irom_start;
  _image_irom_vaddr = ADDR(.flash.text);

  /* Align next section to 64k to allow mapping */
  .flash.text_dummy (NOLOAD) :
  {
    . = ALIGN(CACHE_ALIGN);
  } GROUP_LINK_IN(ROMABLE_REGION)

  .flash.text : ALIGN(0x10)
  {
    _stext = .;
    _instruction_reserved_start = ABSOLUTE(.);
    _text_start = ABSOLUTE(.);
    __text_region_start = ABSOLUTE(.);
    __rom_region_start = ABSOLUTE(.);

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
    _instruction_reserved_end = ABSOLUTE(.);  /* This is a symbol marking the flash.text end, this can be used for mmu driver to maintain virtual address */
    __text_region_end = ABSOLUTE(.);
    __rom_region_end = ABSOLUTE(.);
    _etext = .;

  } GROUP_DATA_LINK_IN(FLASH_CODE_REGION, ROMABLE_REGION)

  /* This dummy section represents the .flash.text section but in default_rodata_seg.
   * Thus, it must have its alignment and (at least) its size.
   */
  .flash.rodata_dummy (NOLOAD):
  {
    _flash_rodata_dummy_start = ABSOLUTE(.);
    . += SIZEOF(.flash.text);
    . = ALIGN(CACHE_ALIGN);
  } GROUP_LINK_IN(RODATA_REGION)

  _image_drom_start = LOADADDR(.flash.rodata);
  _image_drom_size = LOADADDR(.flash.rodata_end) + SIZEOF(.flash.rodata_end) - _image_drom_start;
  _image_drom_vaddr = ADDR(.flash.rodata);

  .flash.rodata : ALIGN(CACHE_ALIGN)
  {
    _flash_rodata_start = ABSOLUTE(.);
    _rodata_reserved_start = ABSOLUTE(.);  /* This is a symbol marking the flash.rodata start, this can be used for mmu driver to maintain virtual address */
    _rodata_start = ABSOLUTE(.);
    __rodata_region_start = ABSOLUTE(.);

    . = ALIGN(4);
    #include <snippets-rodata.ld>

    *(.irom1.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.gnu.linkonce.r.*)
    *(.rodata)
    *(.rodata.*)
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

  /**
   * This section is required to skip flash rodata sections, because `ext_ram_seg`
   * and `drom0_0_seg` are on the same bus
   */
#if defined(CONFIG_ESP_SPIRAM)

  .ext_ram.dummy (NOLOAD):
  {
    . = ORIGIN(ext_ram_seg) + (_rodata_reserved_end - _flash_rodata_dummy_start);
    . = ALIGN (CACHE_ALIGN);
  } GROUP_LINK_IN(ext_ram_seg)

  /* This section holds .ext_ram.bss data, and will be put in PSRAM */
  .ext_ram.bss (NOLOAD) :
  {
    _ext_ram_bss_start = ABSOLUTE(.);
    *(.ext_ram.bss*)
    . = ALIGN(4);

    _spiram_heap_start = ABSOLUTE(.);
    . = . + CONFIG_ESP_SPIRAM_HEAP_SIZE - (_spiram_heap_start - _ext_ram_bss_start);
    . = ALIGN(4);

    _ext_ram_bss_end = ABSOLUTE(.);
  } GROUP_LINK_IN(ext_ram_seg)

#endif /* CONFIG_ESP_SPIRAM */


#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif

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

ASSERT(((_heap_start - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)),
          "DRAM segment data does not fit.")

#if defined(CONFIG_ESP_SPIRAM)
ASSERT(((_ext_ram_bss_end - _ext_ram_bss_start) <= CONFIG_ESP_SPIRAM_SIZE),
          "External SPIRAM overflowed.")
#endif
