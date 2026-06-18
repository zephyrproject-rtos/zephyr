/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <rom/ets_sys.h>
#include <esp_psram.h>
#include <esp_private/esp_psram_extram.h>
#include <hal/cache_hal.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

extern int _instruction_reserved_start;
extern int _instruction_reserved_end;
extern int _rodata_reserved_start;
extern int _rodata_reserved_end;

extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;
extern int _ext_ram_heap_start;
extern int _ext_ram_heap_end;

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

	/*
	 * PSRAM chip init transitions the MSPI clock through low and high
	 * speed modes and reprograms the flash/PSRAM tuning registers.
	 * Cache lines fetched during that window can hold garbage. Run the
	 * chip-init step first, invalidate the flash IROM/DROM ranges, then
	 * run the rest of PSRAM init so the next flash fetch reloads with
	 * the final MSPI settings. ESP32 cache has no per-address
	 * invalidate primitive, so the invalidate step is skipped there.
	 */
	if (esp_psram_chip_init()) {
		ets_printf("Failed to Initialize external RAM, aborting.\n");
		return;
	}

#if !defined(CONFIG_SOC_SERIES_ESP32)
	cache_hal_invalidate_addr((uint32_t)&_instruction_reserved_start,
				  (uint32_t)&_instruction_reserved_end -
					  (uint32_t)&_instruction_reserved_start);
	cache_hal_invalidate_addr((uint32_t)&_rodata_reserved_start,
				  (uint32_t)&_rodata_reserved_end -
					  (uint32_t)&_rodata_reserved_start);
#endif

	if (esp_psram_init()) {
		ets_printf("Failed to Initialize external RAM, aborting.\n");
		return;
	}

	if (esp_psram_get_size() < CONFIG_ESP_SPIRAM_SIZE) {
		ets_printf("External RAM size is less than configured.\n");
	}

	esp_psram_get_mapped_region(&mapped_vaddr, &mapped_size);
	ARG_UNUSED(mapped_vaddr);
	ARG_UNUSED(mapped_size);

	/*
	 * Use the linker-reserved heap window inside the PSRAM-backed
	 * .ext_ram.data output section. The linker places ext_ram_noinit
	 * and ext_ram_bss content before the heap, so starting the SMH
	 * region at the raw MMU-mapped base would overlap and clobber
	 * those allocations (e.g. relocated thread stacks and net packet
	 * pools under CONFIG_ESP32_WIFI_NET_ALLOC_SPIRAM).
	 */
	smh_psram.addr = (uintptr_t)&_ext_ram_heap_start;
	smh_psram.size = (uintptr_t)&_ext_ram_heap_end -
			 (uintptr_t)&_ext_ram_heap_start;

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
