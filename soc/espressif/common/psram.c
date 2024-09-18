/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <rom/ets_sys.h>
#include <esp_psram.h>
#include <esp_private/esp_psram_extram.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

#define PSRAM_ADDR (DT_REG_ADDR(DT_NODELABEL(psram0)))

extern int _spiram_heap_start;
extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;

struct shared_multi_heap_region smh_psram = {
	.addr = (uintptr_t)&_spiram_heap_start,
	.size = CONFIG_ESP_SPIRAM_SIZE,
	.attr = SMH_REG_ATTR_EXTERNAL,
};

int esp_psram_smh_init(void)
{
	shared_multi_heap_pool_init();
	smh_psram.size = CONFIG_ESP_SPIRAM_SIZE - ((int)&_spiram_heap_start - PSRAM_ADDR);
	return shared_multi_heap_add(&smh_psram, NULL);
}

void esp_init_psram(void)
{
	if (esp_psram_init()) {
		ets_printf("Failed to Initialize external RAM, aborting.\n");
		return;
	}

	if (esp_psram_get_size() < CONFIG_ESP_SPIRAM_SIZE) {
		ets_printf("External RAM size is less than configured.\n");
	}

	if (esp_psram_is_initialized()) {
		if (!esp_psram_extram_test()) {
			ets_printf("External RAM failed memory test!");
			return;
		}
	}

	memset(&_ext_ram_bss_start, 0,
	       (&_ext_ram_bss_end - &_ext_ram_bss_start) * sizeof(_ext_ram_bss_start));
}
