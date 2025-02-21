/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#if !defined(CONFIG_BOOTLOADER_MCUBOOT)
#error "APPCPU image must use MCUboot image format."
#endif /* CONFIG_BOOTLOADER_MCUBOOT */

#include "memory.h"

/* User available SRAM memory segments */
appcpu_iram_end = USER_IRAM_END;
appcpu_dram_end = USER_DRAM_END;

appcpu_iram_org = appcpu_iram_end - APPCPU_SRAM_SIZE;
appcpu_dram_org = appcpu_dram_end - APPCPU_SRAM_SIZE;

appcpu_iram_len = APPCPU_SRAM_SIZE;
appcpu_dram_len = APPCPU_SRAM_SIZE;

/* User available ROM memory segments */
appcpu_irom_org = (ICACHE0_START + ICACHE0_SIZE) - APPCPU_ROM_SIZE;
appcpu_drom_org = (DCACHE0_START + DCACHE0_SIZE) - APPCPU_ROM_SIZE;

appcpu_irom_len = APPCPU_ROM_SIZE;
appcpu_drom_len = APPCPU_ROM_SIZE;

/* Aliases */
#define FLASH_CODE_REGION irom0_1_seg
#define RODATA_REGION     drom0_1_seg
#define IRAM_REGION       iram0_1_seg
#define RAMABLE_REGION    dram0_1_seg
#define ROMABLE_REGION    FLASH

/* Zephyr macro re-definitions */
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

MEMORY
{
  mcuboot_hdr (R): org = 0x0,  len = 0x20
  metadata (R):    org = 0x20, len = 0x60
  FLASH (R):       org = 0x80, len = FLASH_SIZE - 0x80

  iram0_1_seg(RX): org = appcpu_iram_org, len = appcpu_iram_len
  dram0_1_seg(RW): org = appcpu_dram_org, len = appcpu_dram_len

  irom0_1_seg(RX): org = appcpu_irom_org, len = appcpu_irom_len
  drom0_1_seg(R):  org = appcpu_drom_org, len = appcpu_drom_len

#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST(RW): org = 0x3ebfe010, len = 0x2000
#endif
}

/*  Default entry point:  */
ENTRY(__appcpu_start)

/* Heap size calculations for APPCPU */
_heap_sentry = DRAM_RESERVED_START;
_libc_heap_size = _heap_sentry - _end;

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
  /* Image load table */
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
    LONG(_iram_end - _init_start);

    /* DRAM metadata:
     * 5. Destination address (VMA) for DRAM region
     * 6. Flash offset (LMA) for start of DRAM region
     * 7. Size of DRAM region
     */
    LONG(ADDR(.dram0.data))
    LONG(LOADADDR(.dram0.data))
    LONG(_data_end - _data_start)

    /* RTC_IRAM metadata:
     * 8. Destination address (VMA) for RTC_IRAM region
     * 9. Flash offset (LMA) for start of RTC_IRAM region
     * 10. Size of RTC_IRAM region
     */
    LONG(0);
    LONG(0);
    LONG(0);

    /* RTC_DRAM metadata:
     * 11. Destination address (VMA) for RTC_DRAM region
     * 12. Flash offset (LMA) for start of RTC_DRAM region
     * 13. Size of RTC_DRAM region
     */
    LONG(0);
    LONG(0);
    LONG(0);

    /* IROM metadata:
     * 14. Destination address (VMA) for IROM region
     * 15. Flash offset (LMA) for start of IROM region
     * 16. Size of IROM region
     */
    LONG(_image_irom_vaddr);
    LONG(_image_irom_start);
    LONG(_image_irom_size);

    /* DROM metadata:
     * 17. Destination address (VMA) for DROM region
     * 18. Flash offset (LMA) for start of DROM region
     * 19. Size of DROM region
     */
    LONG(_image_drom_vaddr);
    LONG(_image_drom_start);
    LONG(_image_drom_size);
  } > metadata

  #include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
  #include <zephyr/linker/llext-sections.ld>
#endif

  /* --- START OF IRAM --- */

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
    _invalid_pc_placeholder = ABSOLUTE(.);
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
    *libzephyr.a:flash_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_flash_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:console_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:hw_init.*(.literal .text .literal.* .text.*)
    *libzephyr.a:soc_random.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_mmu_map.*(.literal .literal.* .text .text.*)
    *libdrivers__interrupt_controller.a:(.literal .literal.* .text .text.*)
    *liblib__libc__minimal.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__newlib.a:string.*(.literal .text .literal.* .text.*)
    *liblib__libc__picolibc.a:string.*(.literal .text .literal.* .text.*)
    *libphy.a:(.phyiram .phyiram.*)
    *libgcov.a:(.literal .text .literal.* .text.*)

    /* APPCPU_ENABLED */
    *libzephyr.a:bootloader_flash.*(.literal .text .literal.* .text.*)
    *libzephyr.a:flash_mmap.*(.literal .text .literal.* .text.*)

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
    *libzephyr.a:spi_flash_oct_flash_init.*(.literal .literal.* .text .text.*)
    *libzephyr.a:flash_ops.*(.literal .literal.* .text .text.*)

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
    *libzephyr.a:cache_utils.*(.literal .text .literal.* .text.*)
    *libzephyr.a:esp_rom_spiflash.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_systimer.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_wdt.*(.literal .literal.* .text .text.*)
    *libzephyr.a:esp_rom_efuse.*(.literal .literal.* .text .text.*)

    /* [mapping:esp_mm] */
    *libzephyr.a:esp_cache.*(.literal .literal.* .text .text.*)

    . = ALIGN(16);

  } GROUP_DATA_LINK_IN(IRAM_REGION, ROMABLE_REGION)

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
    . = ORIGIN(dram0_1_seg) + (MAX(_iram_end, _init_start) - _init_start);
    . = ALIGN(16);
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
    *libzephyr.a:flash_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_flash_init.*(.rodata .rodata.*)
    *libzephyr.a:console_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_init.*(.rodata .rodata.*)
    *libzephyr.a:hw_init.*(.rodata .rodata.*)
    *libzephyr.a:soc_random.*(.rodata .rodata.*)
    *libdrivers__serial.a:uart_esp32.*(.rodata .rodata.*)
    *libdrivers__flash.a:flash_esp32.*(.rodata .rodata.*)
    *libzephyr.a:esp_mmu_map.*(.rodata .rodata.*)
    *libdrivers__interrupt_controller.a:(.rodata .rodata.*)

    /* APPCPU_ENABLE */
    *libzephyr.a:esp32s3-mp.*(.rodata .rodata.*)
    *libzephyr.a:bootloader_flash.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)
    *libzephyr.a:flash_mmap.*(.rodata .rodata.* .sdata2 .sdata2.* .srodata .srodata.*)

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
    *libzephyr.a:flash_qio_mode.*(.rodata .rodata.*)
    *libzephyr.a:flash_ops.*(.rodata .rodata.*)

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
    *libzephyr.a:cache_utils.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_spiflash.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_systimer.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_wdt.*(.rodata .rodata.*)
    *libzephyr.a:esp_rom_efuse.*(.rodata .rodata.*)

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

  .dram0.data_end :
  {
    __data_end = ABSOLUTE(.);
    _data_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  .dram0.noinit (NOLOAD):
  {
    . = ALIGN(8);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(8) ;
  } GROUP_LINK_IN(RAMABLE_REGION)

  /* Shared RAM */
  .dram0.bss (NOLOAD) :
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

  /* Provide total SRAM usage, including IRAM and DRAM */
  _image_ram_start = _init_start - IRAM_DRAM_OFFSET;
  #include <zephyr/linker/ram-end.ld>

  ASSERT(((__bss_end - ORIGIN(dram0_1_seg)) <= LENGTH(dram0_1_seg)), "DRAM segment data does not fit.")

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

  .flash.text : ALIGN(16)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);

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
    . = ALIGN(4);
    _flash_cache_start = ABSOLUTE(0);

    . = ALIGN(4);

  } GROUP_DATA_LINK_IN(FLASH_CODE_REGION, ROMABLE_REGION)

  /* --- END OF IROM --- */

  /* --- START OF DROM --- */

  /* This dummy section represents the .flash.text section but in default_rodata_seg.
   * Thus, it must have its alignment and (at least) its size.
   */
  .cache.rodata_dummy (NOLOAD):
  {
    _cache_rodata_dummy_start = ABSOLUTE(.);
    . += SIZEOF(.flash.text);
    . = ALIGN(CACHE_ALIGN);
    _cache_rodata_dummy_end = ABSOLUTE(.);
  } GROUP_LINK_IN(RODATA_REGION)

  .flash.rodata_dummy (NOLOAD):
  {
    _flash_rodata_dummy_start = ABSOLUTE(.);
    . += SIZEOF(.flash.text);
    . = ALIGN(CACHE_ALIGN);
    _flash_rodata_dummy_end = ABSOLUTE(.);
  } GROUP_LINK_IN(ROMABLE_REGION)

  _image_drom_start = LOADADDR(.flash.rodata);
  _image_drom_size = LOADADDR(.flash.rodata_end) + SIZEOF(.flash.rodata_end) - _image_drom_start;
  _image_drom_vaddr = ADDR(.flash.rodata);

  .flash.rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);

    *(.rodata_desc .rodata_desc.*)               /* Should be the first.  App version info.        DO NOT PUT ANYTHING BEFORE IT! */
    *(.rodata_custom_desc .rodata_custom_desc.*) /* Should be the second. Custom app version info. DO NOT PUT ANYTHING BEFORE IT! */

    __rodata_region_start = ABSOLUTE(.);
    . = ALIGN(4);
    #include <snippets-rodata.ld>

    . = ALIGN(4);

    *(.rodata)
    *(.rodata.*)
    . = ALIGN(4);

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
  .flash.rodata_end : ALIGN(16)
  {
    . = ALIGN(16);
    _rodata_reserved_end = ABSOLUTE(.);
    _image_rodata_end = ABSOLUTE(.);
  } GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)

  /* --- END OF DROM --- */

  /* --- XTENSA GLUE AND DEBUG BEGIN --- */

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

  /* --- XTENSA GLUE AND DEBUG END --- */

ASSERT(((_iram_end - ORIGIN(iram0_1_seg)) <= LENGTH(iram0_1_seg)),
          "IRAM0 segment data does not fit.")

ASSERT(((_end - ORIGIN(dram0_1_seg)) <= LENGTH(dram0_1_seg)),
          "DRAM segment data does not fit.")
