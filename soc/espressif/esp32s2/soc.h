/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__
#include <soc/syscon_reg.h>
#include <soc/system_reg.h>
#include <soc/dport_access.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc_caps.h>
#include <esp32s2/rom/ets_sys.h>
#include <esp32s2/rom/spi_flash.h>
#include <esp32s2/rom/cache.h>
#include <esp_private/esp_clk.h>
#include <esp_rom_sys.h>

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/arch/xtensa/arch.h>
#include <stdlib.h>

void __esp_platform_mcuboot_start(void);
void __esp_platform_app_start(void);

static inline uint32_t esp_core_id(void)
{
	return 0;
}
extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_no);
extern int esp_rom_uart_tx_one_char(uint8_t chr);
extern int esp_rom_uart_rx_one_char(uint8_t *chr);

extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index, bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				bool out_invrted, bool out_enabled_inverted);

/* cache related rom functions */
extern uint32_t esp_rom_Cache_Disable_ICache(void);
extern uint32_t esp_rom_Cache_Disable_DCache(void);

extern void esp_rom_Cache_Allocate_SRAM(cache_layout_t sram0_layout, cache_layout_t sram1_layout,
					cache_layout_t sram2_layout, cache_layout_t sram3_layout);

extern uint32_t esp_rom_Cache_Suspend_ICache(void);

extern void esp_rom_Cache_Set_ICache_Mode(cache_size_t cache_size, cache_ways_t ways,
					cache_line_size_t cache_line_size);

extern void esp_rom_Cache_Invalidate_ICache_All(void);
extern void esp_rom_Cache_Resume_ICache(uint32_t autoload);
extern int esp_rom_Cache_Invalidate_Addr(uint32_t addr, uint32_t size);

/* data-cache related rom functions */
extern void esp_rom_Cache_Set_DCache_Mode(cache_size_t cache_size, cache_ways_t ways,
					cache_line_size_t cache_line_size);

extern void esp_rom_Cache_Invalidate_DCache_All(void);
extern void esp_rom_Cache_Enable_DCache(uint32_t autoload);

extern void esp_rom_Cache_Set_DCache_Mode(cache_size_t cache_size, cache_ways_t ways,
					cache_line_size_t cache_line_size);

extern int esp_rom_Cache_Ibus_MMU_Set(uint32_t ext_ram, uint32_t vaddr, uint32_t paddr,
					uint32_t psize, uint32_t num, uint32_t fixed);

/* ROM information related to SPI Flash chip timing and device */
extern esp_rom_spiflash_chip_t g_rom_flashchip;
extern uint8_t g_rom_spiflash_dummy_len_plus[];

extern uint32_t esp_rom_g_ticks_per_us_pro;

/* cache initialization functions */
void esp_config_instruction_cache_mode(void);
void esp_config_data_cache_mode(void);

#endif /* __SOC_H__ */
