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
#include <esp32s2/rom/ets_sys.h>
#include <esp32s2/rom/spi_flash.h>
#include <esp32s2/rom/cache.h>

#include <zephyr/types.h>
#include <stdbool.h>
#include <arch/xtensa/arch.h>

extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_no);
extern STATUS esp_rom_uart_tx_one_char(uint8_t chr);
extern STATUS esp_rom_uart_rx_one_char(uint8_t *chr);

/* cache related rom functions */
extern uint32_t esp_rom_Cache_Disable_ICache(void);
extern uint32_t esp_rom_Cache_Disable_DCache(void);

extern void esp_rom_Cache_Allocate_SRAM(cache_layout_t sram0_layout, cache_layout_t sram1_layout,
					cache_layout_t sram2_layout, cache_layout_t sram3_layout);

extern uint32_t esp_rom_Cache_Suspend_ICache(void);

extern void esp_rom_Cache_Set_ICache_Mode(cache_size_t cache_size, cache_ways_t ways,
					cache_line_size_t cache_line_size);

extern void esp_rom_Cache_Invalidate_ICache_All(void);
void esp_rom_Cache_Resume_ICache(uint32_t autoload);

#endif /* __SOC_H__ */
