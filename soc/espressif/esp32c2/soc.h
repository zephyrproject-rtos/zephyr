/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#ifndef _ASMLANGUAGE
#include <soc/soc.h>
#include <rom/ets_sys.h>
#include <rom/spi_flash.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <esp_private/esp_clk.h>
#endif

#ifndef _ASMLANGUAGE

void __esp_platform_start(void);

static inline uint32_t esp_core_id(void)
{
	return 0;
}

extern void esp_rom_intr_matrix_set(int cpu_no, uint32_t model_num, uint32_t intr_num);
extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_no);
extern int esp_rom_uart_tx_one_char(uint8_t chr);
extern int esp_rom_uart_rx_one_char(uint8_t *chr);
extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index, bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				bool out_invrted, bool out_enabled_inverted);
extern void esp_rom_ets_set_user_start(uint32_t start);
extern void esprv_intc_int_set_threshold(int priority_threshold);
uint32_t soc_intr_get_next_source(void);
extern void esp_rom_Cache_Resume_ICache(uint32_t autoload);
extern int esp_rom_Cache_Invalidate_Addr(uint32_t addr, uint32_t size);
extern uint32_t esp_rom_Cache_Suspend_ICache(void);
extern void esp_rom_Cache_Invalidate_ICache_All(void);
extern int esp_rom_Cache_Dbus_MMU_Set(uint32_t ext_ram, uint32_t vaddr, uint32_t paddr,
				uint32_t psize, uint32_t num, uint32_t fixed);
extern int esp_rom_Cache_Ibus_MMU_Set(uint32_t ext_ram, uint32_t vaddr, uint32_t paddr,
				uint32_t psize, uint32_t num, uint32_t fixed);
extern void esp_rom_Cache_Resume_ICache(uint32_t autoload);
extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index,
				    bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

#endif /* _ASMLANGUAGE */

#endif /* __SOC_H__ */
