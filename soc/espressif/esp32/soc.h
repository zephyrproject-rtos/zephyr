/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ESPRESSIF_ESP32_SOC_H_
#define _ESPRESSIF_ESP32_SOC_H_
#include <soc/dport_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc_caps.h>
#include <esp32/rom/ets_sys.h>
#include <esp_rom_sys.h>

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/arch/xtensa/arch.h>

#include <xtensa/core-macros.h>
#include <esp_attr.h>
#include <esp_private/esp_clk.h>

void __esp_platform_mcuboot_start(void);
void __esp_platform_app_start(void);

static inline uint32_t esp_core_id(void)
{
	uint32_t id;

	__asm__ volatile (
		"rsr.prid %0\n"
		"extui %0,%0,13,1" : "=r" (id));
	return id;
}

extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index,
				    bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

extern void esp_rom_Cache_Flush(int cpu);
extern void esp_rom_Cache_Read_Enable(int cpu);
extern void esp_rom_ets_set_appcpu_boot_addr(void *addr);
void esp_appcpu_start(void *entry_point);

#endif /* _ESPRESSIF_ESP32_SOC_H_ */
