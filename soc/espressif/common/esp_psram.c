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

extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;

struct shared_multi_heap_region smh_psram = {
	.attr = SMH_REG_ATTR_EXTERNAL,
};

int esp_psram_smh_init(void)
{
	shared_multi_heap_pool_init();
	return shared_multi_heap_add(&smh_psram, NULL);
}

void esp_init_psram(void)
{
	intptr_t mapped_vaddr = 0;
	size_t mapped_size = 0;

	if (esp_psram_init()) {
		ets_printf("Failed to Initialize external RAM, aborting.\n");
		return;
	}

	if (esp_psram_get_size() < CONFIG_ESP_SPIRAM_SIZE) {
		ets_printf("External RAM size is less than configured.\n");
	}

	esp_psram_get_mapped_region(&mapped_vaddr, &mapped_size);

	/*
	 * Use the runtime MMU-mapped address for the heap. On SoCs with
	 * unified cache (e.g. C5, C6, H2), the exact virtual address
	 * depends on MMU page allocation at runtime.
	 */
	smh_psram.addr = (uintptr_t)mapped_vaddr;
	smh_psram.size = MIN(mapped_size, CONFIG_ESP_SPIRAM_HEAP_SIZE);

	if (IS_ENABLED(CONFIG_ESP_SPIRAM_MEMTEST)) {
		if (esp_psram_is_initialized()) {
			if (!esp_psram_extram_test()) {
				ets_printf("External RAM failed memory test!");
				return;
			}
		}
	}

	memset(&_ext_ram_bss_start, 0,
	       (&_ext_ram_bss_end - &_ext_ram_bss_start) * sizeof(_ext_ram_bss_start));
}
