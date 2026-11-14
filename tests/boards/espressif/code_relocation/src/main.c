/* SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>
#include <esp_memory_utils.h>

#include "reloc.h"

extern char __ram_text_reloc_start[];
extern char __ram_text_reloc_end[];
extern char __ram_rodata_reloc_start[];
extern char __ram_rodata_reloc_end[];
extern char __ram_data_reloc_start[];
extern char __ram_data_reloc_end[];
extern char __ram_bss_reloc_start[];
extern char __ram_bss_reloc_end[];
extern char __ram_noinit_reloc_start[];
extern char __ram_noinit_reloc_end[];

#define IN_RELOC(ptr, mem, kind)                                                                   \
	(((uintptr_t)(ptr) >= (uintptr_t)__##mem##_##kind##_reloc_start) &&                        \
	 ((uintptr_t)(ptr) < (uintptr_t)__##mem##_##kind##_reloc_end))

ZTEST(code_relocation, test_ram_text)
{
	uint32_t first;
	uint32_t second;

	zassert_true(esp_ptr_in_iram((void *)reloc_func), "reloc_func not in IRAM: %p",
		     (void *)reloc_func);

	first = reloc_func(5U);
	second = reloc_func(0U);
	zassert_equal(first, 25U + RELOC_DATA_INIT + 5U);
	zassert_equal(second, RELOC_DATA_INIT + 5U);
}

ZTEST(code_relocation, test_ram_text_cache_off)
{
	zassert_equal(reloc_func_no_cache(0U), reloc_func(0U));
}

ZTEST(code_relocation, test_ram_text_filter)
{
	static const uint8_t buf[] = "123456789";

	zassert_true(esp_ptr_in_iram((void *)crc32_ieee), "crc32_ieee not in IRAM: %p",
		     (void *)crc32_ieee);
	zassert_false(esp_ptr_in_iram((void *)crc32_ieee_update),
		      "crc32_ieee_update relocated despite FILTER: %p", (void *)crc32_ieee_update);
	zassert_equal(crc32_ieee(buf, sizeof(buf) - 1U), 0xcbf43926U);
}

ZTEST(code_relocation, test_ram_text_kernel)
{
	struct k_sem sem;

	zassert_true(IN_RELOC(z_impl_k_sem_give, ram, text), "z_impl_k_sem_give not relocated: %p",
		     (void *)z_impl_k_sem_give);

	k_sem_init(&sem, 0U, 1U);
	k_sem_give(&sem);
	zassert_ok(k_sem_take(&sem, K_NO_WAIT));
}

ZTEST(code_relocation, test_ram_data)
{
	zassert_true(IN_RELOC(reloc_rodata, ram, rodata), "reloc_rodata not relocated: %p",
		     (void *)reloc_rodata);
	zassert_true(IN_RELOC(&reloc_data, ram, data), "reloc_data not relocated: %p",
		     (void *)&reloc_data);
	zassert_true(IN_RELOC(&reloc_bss, ram, bss), "reloc_bss not relocated: %p",
		     (void *)&reloc_bss);
	zassert_true(IN_RELOC(&reloc_noinit, ram, noinit), "reloc_noinit not relocated: %p",
		     (void *)&reloc_noinit);
	zassert_equal(reloc_rodata[3], 11U);
}

#if defined(CONFIG_ESP_SPIRAM)
ZTEST(code_relocation, test_extmem_noinit)
{
	zassert_true(esp_ptr_external_ram(ext_buf), "ext_buf not in PSRAM: %p", (void *)ext_buf);

	for (uint32_t i = 0U; i < EXT_BUF_WORDS; i++) {
		ext_buf[i] = i ^ 0xa5a5a5a5U;
	}
	for (uint32_t i = 0U; i < EXT_BUF_WORDS; i++) {
		zassert_equal(ext_buf[i], i ^ 0xa5a5a5a5U, "mismatch at %u", i);
	}
}

ZTEST(code_relocation, test_extmem_rule_kept)
{
	zassert_true(esp_ptr_external_ram(reloc_ext_bss), "reloc_ext_bss not in PSRAM: %p",
		     (void *)reloc_ext_bss);
	zassert_equal(reloc_ext_bss[0], 0U);
}

ZTEST(code_relocation, test_extmem_kernel_objects)
{
	uint8_t *p;
	void *blk;

	p = k_heap_alloc(&ext_heap, EXT_HEAP_SIZE / 2U, K_NO_WAIT);
	zassert_not_null(p, "k_heap_alloc failed");
	zassert_true(esp_ptr_external_ram(p), "heap block not in PSRAM: %p", (void *)p);
	memset(p, 0x5a, EXT_HEAP_SIZE / 2U);
	zassert_equal(p[EXT_HEAP_SIZE / 2U - 1U], 0x5aU);
	k_heap_free(&ext_heap, p);

	zassert_ok(k_mem_slab_alloc(&ext_slab, &blk, K_NO_WAIT));
	zassert_true(esp_ptr_external_ram(blk), "slab block not in PSRAM: %p", blk);
	k_mem_slab_free(&ext_slab, blk);
}
#endif /* CONFIG_ESP_SPIRAM */

static void *code_relocation_setup(void)
{
	zassert_equal(reloc_data, RELOC_DATA_INIT, "reloc_data not initialized");
	zassert_equal(reloc_bss, 0U, "reloc_bss not zeroed");

	return NULL;
}

ZTEST_SUITE(code_relocation, NULL, code_relocation_setup, NULL, NULL, NULL);
