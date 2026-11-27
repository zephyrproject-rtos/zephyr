/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

struct nvmc_calls {
	uint32_t calls;
	uint32_t preemptible;
};

static struct nvmc_calls word_write;
static struct nvmc_calls page_erase;
static struct nvmc_calls partial_erase;
static struct nvmc_calls bytes_write;

/*
 * Each NVMC call wakes a thread of higher priority than the caller, which
 * must have run by the time the next call starts: the scheduler is locked
 * per NVMC call, not across the whole flash operation.
 */
#define HELPER_PRIO (CONFIG_ZTEST_THREAD_PRIORITY - 1)
#define HELPER_STACK_SIZE 1024U

static K_SEM_DEFINE(helper_wake, 0, 1);
static volatile uint32_t helper_runs;
static uint32_t nvmc_ops;
static uint32_t helper_late;

static void helper(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		/* k_sem_reset() aborts the take with -EAGAIN */
		if (k_sem_take(&helper_wake, K_FOREVER) == 0) {
			helper_runs++;
		}
	}
}

K_THREAD_DEFINE(helper_tid, HELPER_STACK_SIZE, helper, NULL, NULL, NULL, HELPER_PRIO, 0, 0);

static void nvmc_call(struct nvmc_calls *c)
{
	if (helper_runs != nvmc_ops) {
		helper_late++;
	}
	nvmc_ops++;
	k_sem_give(&helper_wake);

	c->calls++;
	if (k_is_preempt_thread()) {
		c->preemptible++;
	}
}

void __real_nrfx_nvmc_word_write(uint32_t addr, uint32_t value);
int __real_nrfx_nvmc_page_erase(uint32_t addr);
void __wrap_nrfx_nvmc_word_write(uint32_t addr, uint32_t value);
int __wrap_nrfx_nvmc_page_erase(uint32_t addr);

void __wrap_nrfx_nvmc_word_write(uint32_t addr, uint32_t value)
{
	nvmc_call(&word_write);
	__real_nrfx_nvmc_word_write(addr, value);
}

int __wrap_nrfx_nvmc_page_erase(uint32_t addr)
{
	nvmc_call(&page_erase);
	return __real_nrfx_nvmc_page_erase(addr);
}

#if defined(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)
bool __real_nrfx_nvmc_page_partial_erase_continue(void);
bool __wrap_nrfx_nvmc_page_partial_erase_continue(void);

bool __wrap_nrfx_nvmc_page_partial_erase_continue(void)
{
	nvmc_call(&partial_erase);
	return __real_nrfx_nvmc_page_partial_erase_continue();
}
#endif

#if defined(CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS)
void __real_nrfx_nvmc_bytes_write(uint32_t addr, void const *src, uint32_t num_bytes);
void __wrap_nrfx_nvmc_bytes_write(uint32_t addr, void const *src, uint32_t num_bytes);

void __wrap_nrfx_nvmc_bytes_write(uint32_t addr, void const *src, uint32_t num_bytes)
{
	nvmc_call(&bytes_write);
	__real_nrfx_nvmc_bytes_write(addr, src, num_bytes);
}
#endif

static const struct device *const flash = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

static struct flash_pages_info last_page(void)
{
	struct flash_pages_info info;

	zassert_ok(flash_get_page_info_by_idx(flash, flash_get_page_count(flash) - 1, &info));
	return info;
}

static void reset(void *f)
{
	ARG_UNUSED(f);

	(void)memset(&word_write, 0, sizeof(word_write));
	(void)memset(&page_erase, 0, sizeof(page_erase));
	(void)memset(&partial_erase, 0, sizeof(partial_erase));
	(void)memset(&bytes_write, 0, sizeof(bytes_write));
	k_sem_reset(&helper_wake);
	helper_runs = 0;
	nvmc_ops = 0;
	helper_late = 0;
}

static void assert_lock_per_call(void)
{
	zassert_true(nvmc_ops > 0U, "no NVMC call was made");
	zassert_equal(helper_late, 0U, "scheduler stayed locked between NVMC calls");
	zassert_equal(helper_runs, nvmc_ops, "scheduler stayed locked after the last NVMC call");
	zassert_true(k_is_preempt_thread(), "scheduler lock leaked");
}

static void assert_erased(off_t offset, size_t len)
{
	uint8_t back[16];

	zassert_true(len <= sizeof(back));
	zassert_ok(flash_read(flash, offset, back, len));
	for (size_t i = 0; i < len; i++) {
		zassert_equal(back[i], 0xffU, "byte %zu not erased", i);
	}
}

ZTEST(soc_flash_nrf, test_erase_not_preemptible)
{
	static const uint32_t data[4] = {0x01234567U, 0x89abcdefU, 0xdeadbeefU, 0x5a5aa5a5U};
	struct flash_pages_info page;
	off_t first;

	zassert_true(k_is_preempt_thread(), "test thread must be preemptible");

	/* Two pages, so a lock held across pages shows up */
	page = last_page();
	first = page.start_offset - page.size;
	zassert_ok(flash_erase(flash, first, 2 * page.size));
	zassert_ok(flash_write(flash, first, data, sizeof(data)));
	zassert_ok(flash_write(flash, page.start_offset, data, sizeof(data)));
	reset(NULL);

	zassert_ok(flash_erase(flash, first, 2 * page.size));
	if (IS_ENABLED(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)) {
		zassert_equal(page_erase.calls, 0U);
		zassert_true(partial_erase.calls > 1U);
		zassert_equal(partial_erase.preemptible, 0U, "partial erase ran preemptible");
	} else {
		zassert_equal(page_erase.calls, 2U);
		zassert_equal(page_erase.preemptible, 0U, "page erase ran preemptible");
	}
	assert_lock_per_call();
	assert_erased(first, sizeof(data));
	assert_erased(page.start_offset, sizeof(data));
}

ZTEST(soc_flash_nrf, test_write_not_preemptible)
{
	static const uint32_t data[4] = {0x01234567U, 0x89abcdefU, 0xdeadbeefU, 0x5a5aa5a5U};
	uint32_t back[4];
	struct flash_pages_info page;

	zassert_true(k_is_preempt_thread(), "test thread must be preemptible");

	page = last_page();
	zassert_ok(flash_erase(flash, page.start_offset, page.size));
	reset(NULL);

	zassert_ok(flash_write(flash, page.start_offset, data, sizeof(data)));
	zassert_equal(word_write.calls, 4U);
	zassert_equal(word_write.preemptible, 0U, "word write ran preemptible");
	assert_lock_per_call();

	zassert_ok(flash_read(flash, page.start_offset, back, sizeof(back)));
	zassert_mem_equal(back, data, sizeof(data));
}

#if defined(CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS)
ZTEST(soc_flash_nrf, test_unaligned_write_not_preemptible)
{
	/* One leading byte, then 3 head bytes, one word and 2 tail bytes */
	static const uint8_t data[9] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
	uint8_t back[sizeof(data) + 2];
	struct flash_pages_info page;

	zassert_true(k_is_preempt_thread(), "test thread must be preemptible");

	page = last_page();
	zassert_ok(flash_erase(flash, page.start_offset, page.size));
	reset(NULL);

	zassert_ok(flash_write(flash, page.start_offset + 1, data, sizeof(data)));
	zassert_equal(bytes_write.calls, 2U);
	zassert_equal(word_write.calls, 1U);
	zassert_equal(bytes_write.preemptible, 0U, "bytes write ran preemptible");
	zassert_equal(word_write.preemptible, 0U, "word write ran preemptible");
	assert_lock_per_call();

	zassert_ok(flash_read(flash, page.start_offset, back, sizeof(back)));
	zassert_equal(back[0], 0xffU);
	zassert_mem_equal(&back[1], data, sizeof(data));
	zassert_equal(back[sizeof(back) - 1], 0xffU);
}
#endif

ZTEST_SUITE(soc_flash_nrf, NULL, NULL, reset, NULL, NULL);
