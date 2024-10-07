/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <esp_err.h>
#include <esp_heap_runtime.h>
#include "esp_log.h"

#define TAG "heap_runtime"

/* ESP dynamic pool heap */
extern unsigned int z_mapped_end;
extern unsigned int _heap_sentry;
static void *esp_heap_runtime_init_mem = &z_mapped_end;

#define ESP_HEAP_RUNTIME_MAX_SIZE ((uintptr_t)&_heap_sentry - (uintptr_t)&z_mapped_end)

static struct k_heap esp_heap_runtime;

static int esp_heap_runtime_init(void)
{
	ESP_EARLY_LOGI(TAG, "ESP heap runtime init at 0x%x size %d kB.\n",
		       esp_heap_runtime_init_mem, ESP_HEAP_RUNTIME_MAX_SIZE / 1024);

	k_heap_init(&esp_heap_runtime, esp_heap_runtime_init_mem, ESP_HEAP_RUNTIME_MAX_SIZE);

#if defined(CONFIG_WIFI_ESP32) && defined(CONFIG_BT_ESP32)
	assert(ESP_HEAP_RUNTIME_MAX_SIZE > 65535);
#elif defined(CONFIG_WIFI_ESP32)
	assert(ESP_HEAP_RUNTIME_MAX_SIZE > 51200);
#elif defined(CONFIG_BT_ESP32)
	assert(ESP_HEAP_RUNTIME_MAX_SIZE > 40960);
#endif

	return 0;
}

void *esp_heap_runtime_malloc(size_t size)
{
	return k_heap_alloc(&esp_heap_runtime, size, K_NO_WAIT);
}

void *esp_heap_runtime_calloc(size_t n, size_t size)
{
	size_t sz;

	if (__builtin_mul_overflow(n, size, &sz)) {
		return NULL;
	}
	void *ptr = k_heap_alloc(&esp_heap_runtime, sz, K_NO_WAIT);

	if (ptr) {
		memset(ptr, 0, sz);
	}

	return ptr;
}

void *esp_heap_runtime_realloc(void *ptr, size_t bytes)
{
	return k_heap_realloc(&esp_heap_runtime, ptr, bytes, K_NO_WAIT);
}

void esp_heap_runtime_free(void *mem)
{
	k_heap_free(&esp_heap_runtime, mem);
}

SYS_INIT(esp_heap_runtime_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
