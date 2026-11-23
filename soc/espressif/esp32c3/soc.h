/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ESPRESSIF_ESP32C3_SOC_H_
#define _ESPRESSIF_ESP32C3_SOC_H_

#ifndef _ASMLANGUAGE
#include <soc/soc.h>
#include <rom/ets_sys.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <esp_attr.h>
#include <esp_private/esp_clk.h>
#endif

#ifndef _ASMLANGUAGE

void __esp_platform_mcuboot_start(void);
void __esp_platform_app_start(void);

static inline uint32_t esp_core_id(void)
{
	return 0;
}

extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index,
				    bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

#endif /* _ASMLANGUAGE */

#endif /* _ESPRESSIF_ESP32C3_SOC_H_ */
