/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ESPRESSIF_ESP32S2_SOC_H_
#define _ESPRESSIF_ESP32S2_SOC_H_
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
#include <esp_attr.h>
#include <zephyr/arch/xtensa/arch.h>
#include <stdlib.h>

void __esp_platform_mcuboot_start(void);
void __esp_platform_app_start(void);

static inline uint32_t esp_core_id(void)
{
	return 0;
}

extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index, bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				bool out_invrted, bool out_enabled_inverted);

#endif /* _ESPRESSIF_ESP32S2_SOC_H_ */
