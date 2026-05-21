/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <esp_cache.h>
#include <esp_err.h>

static inline size_t cache_line_size(const void *addr)
{
	size_t line = esp_cache_get_line_size_by_addr(addr);

	return line ? line : sizeof(void *);
}

void cache_data_enable(void)
{
	/* Cache is enabled by the bootloader */
}

void cache_data_disable(void)
{
	/* Not supported */
}

int cache_data_flush_all(void)
{
	return -ENOTSUP;
}

int cache_data_invd_all(void)
{
	return -ENOTSUP;
}

int cache_data_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int cache_data_flush_range(void *addr, size_t size)
{
	esp_err_t ret;

	ret = esp_cache_msync(addr, size,
			      ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);

	return (ret == ESP_OK || ret == ESP_ERR_NOT_SUPPORTED) ? 0 : -EIO;
}

int cache_data_invd_range(void *addr, size_t size)
{
	esp_err_t ret;
	size_t line = cache_line_size(addr);
	uintptr_t aligned_addr = (uintptr_t)addr & ~(line - 1);
	size_t aligned_size = (((uintptr_t)addr + size + line - 1) & ~(line - 1)) - aligned_addr;

	ret = esp_cache_msync((void *)aligned_addr, aligned_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C);

	return (ret == ESP_OK || ret == ESP_ERR_NOT_SUPPORTED) ? 0 : -EIO;
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	esp_err_t ret;

	ret = esp_cache_msync(addr, size,
			      ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE |
				      ESP_CACHE_MSYNC_FLAG_UNALIGNED);

	return (ret == ESP_OK || ret == ESP_ERR_NOT_SUPPORTED) ? 0 : -EIO;
}
