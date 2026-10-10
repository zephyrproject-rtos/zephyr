/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static int writes;
static int writes_preemptible;
static int erases;
static int erases_preemptible;

void __real_nrfx_nvmc_word_write(uint32_t addr, uint32_t value);
int __real_nrfx_nvmc_page_erase(uint32_t addr);

void __wrap_nrfx_nvmc_word_write(uint32_t addr, uint32_t value)
{
	writes++;
	if (k_is_preempt_thread()) {
		writes_preemptible++;
	}
	__real_nrfx_nvmc_word_write(addr, value);
}

int __wrap_nrfx_nvmc_page_erase(uint32_t addr)
{
	erases++;
	if (k_is_preempt_thread()) {
		erases_preemptible++;
	}
	return __real_nrfx_nvmc_page_erase(addr);
}

static const struct device *const flash = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

static struct flash_pages_info last_page(void)
{
	struct flash_pages_info info;

	zassert_ok(flash_get_page_info_by_idx(flash, flash_get_page_count(flash) - 1, &info));
	return info;
}

static void reset(void *f)
{
	writes = 0;
	writes_preemptible = 0;
	erases = 0;
	erases_preemptible = 0;
}

ZTEST(soc_flash_nrf, test_erase_not_preemptible)
{
	struct flash_pages_info page;

	if (!k_is_preempt_thread()) {
		ztest_test_skip();
	}

	page = last_page();
	zassert_ok(flash_erase(flash, page.start_offset, page.size));
	zassert_equal(erases, 1);
	zassert_equal(erases_preemptible, 0);
	zassert_true(k_is_preempt_thread());
}

ZTEST(soc_flash_nrf, test_write_not_preemptible)
{
	static const uint32_t data[4] = {0x01234567, 0x89abcdef, 0xdeadbeef, 0x5a5aa5a5};
	uint32_t back[4];
	struct flash_pages_info page;

	if (!k_is_preempt_thread()) {
		ztest_test_skip();
	}

	page = last_page();
	zassert_ok(flash_erase(flash, page.start_offset, page.size));
	reset(NULL);

	zassert_ok(flash_write(flash, page.start_offset, data, sizeof(data)));
	zassert_equal(writes, 4);
	zassert_equal(writes_preemptible, 0);
	zassert_true(k_is_preempt_thread());

	zassert_ok(flash_read(flash, page.start_offset, back, sizeof(back)));
	zassert_mem_equal(back, data, sizeof(data));
}

ZTEST_SUITE(soc_flash_nrf, NULL, NULL, reset, NULL, NULL);
