/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <esp_memory_utils.h>

ZTEST(psram_heap_alloc, test_alloc_ext_memory)
{

	uint32_t *ext_buf = NULL;
	uint32_t max_heap_size = CONFIG_ESP_SPIRAM_HEAP_SIZE;

	while (!(ext_buf = k_malloc(max_heap_size)) && max_heap_size) {
		max_heap_size--;
	}

	zassert_true(max_heap_size && ext_buf, "Failed to allocate external memory");

	TC_PRINT("Allocating %d bytes of external memory\n", max_heap_size);

	intptr_t start_ptr = (intptr_t)ext_buf;
	intptr_t end_ptr = (intptr_t)ext_buf + max_heap_size;

	zassert_true(esp_ptr_external_ram((void *)start_ptr),
		     "External memory start address is not in external memory");
	zassert_true(esp_ptr_external_ram((void *)end_ptr),
		     "External memory end address is not in external memory");

	for (int i = 0; i < max_heap_size / sizeof(uint32_t); i++) {
		ext_buf[i] = (i + 1) ^ 0xAAAAAAAA;
	}

	for (int i = 0; i < max_heap_size / sizeof(uint32_t); i++) {
		zassert_equal(ext_buf[i], (i + 1) ^ 0xAAAAAAAA, "External memory content mismatch");
	}

	k_free(ext_buf);
}

ZTEST(psram_heap_alloc, test_ext_mem_thr)
{
	uint32_t *ext_buf = NULL;
	uint32_t *int_buf = NULL;

	ext_buf = k_malloc(CONFIG_ESP_HEAP_MIN_EXTRAM_THRESHOLD);
	zassert_not_null(ext_buf, "Failed to allocate external memory");

	int_buf = k_malloc(CONFIG_ESP_HEAP_MIN_EXTRAM_THRESHOLD - 1);
	zassert_not_null(ext_buf, "Failed to allocate external memory");

	zassert_true(esp_ptr_external_ram((void *)ext_buf),
		     "External buffer is not in external memory");
	zassert_false(esp_ptr_external_ram((void *)int_buf),
		      "Internal buffer is in external memory");

	k_free(ext_buf);
	k_free(int_buf);
}

ZTEST_SUITE(psram_heap_alloc, NULL, NULL, NULL, NULL, NULL);
