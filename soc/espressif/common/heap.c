/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <esp_err.h>
#include "esp_log.h"

#define TAG "heap"

/* ESP dynamic pool heap */
extern unsigned int z_mapped_end;
extern unsigned int _heap_sentry;
static void *esp_runtime_heap_init_mem = &z_mapped_end;

#define ESP_RUNTIME_HEAP_MAX_SIZE  ((uintptr_t)&_heap_sentry - (uintptr_t)&z_mapped_end)

struct k_heap esp_runtime_heap;

static int esp_runtime_heap_init(void)
{
	ESP_EARLY_LOGI(TAG, "ESP runtime heap init at 0x%x size %d kB.\n",
			esp_runtime_heap_init_mem, ESP_RUNTIME_HEAP_MAX_SIZE/1024);

	k_heap_init(&esp_runtime_heap, esp_runtime_heap_init_mem, ESP_RUNTIME_HEAP_MAX_SIZE);

#ifdef CONFIG_ESP_WIFI_HEAP_RUNTIME

#if defined(CONFIG_WIFI) && defined(CONFIG_BT)
	assert(ESP_RUNTIME_HEAP_MAX_SIZE > 65535);
#elif defined(CONFIG_WIFI)
	assert(ESP_RUNTIME_HEAP_MAX_SIZE > 51200);
#elif defined(CONFIG_BT)
	assert(ESP_RUNTIME_HEAP_MAX_SIZE > 40960);
#endif

#endif /* CONFIG_ESP_WIFI_HEAP_RUNTIME */

	return 0;
}

SYS_INIT(esp_runtime_heap_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
