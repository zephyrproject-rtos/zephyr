/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <soc/soc_memory_layout.h>
#include <string.h>

/* Cross-core cache coexistence: one core erases and writes flash, which
 * suspends the shared cache, while the other core keeps executing code from
 * flash and reading and writing PSRAM through that same cache. Both roles are
 * pinned, then swapped, so the stall protocol is exercised in both directions.
 */

#define SMP_FLASH_PAGE 1022
#define SMP_PSRAM_SIZE (16 * 1024)
#define SMP_PHASE_MS   3000
#define SMP_STACK_SIZE 2048
#define SMP_PRIO       K_PRIO_PREEMPT(6)

static const struct device *const smp_flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

static uint8_t smp_wbuf[1024];
static uint8_t smp_rbuf[1024];
static uint32_t *smp_psram;

static volatile bool smp_stop;
static volatile uint32_t smp_flash_ops;
static volatile uint32_t smp_flash_errs;
static volatile uint32_t smp_hog_iters;
static volatile uint32_t smp_hog_errs;
static volatile int smp_flash_cpu_seen;
static volatile int smp_hog_cpu_seen;

/* Walked by the hog from flash-resident code. */
static const uint32_t smp_table[1024] = {[0 ... 1023] = 0x9e3779b9};

static uint32_t __attribute__((noinline)) smp_xip_work(uint32_t seed)
{
	uint32_t h = seed;

	for (size_t i = 0; i < ARRAY_SIZE(smp_table); i++) {
		h = (h ^ smp_table[i]) * 16777619u;
		h ^= h >> 13;
	}
	return h;
}

static void smp_hog_fn(void *a, void *b, void *c)
{
	uint32_t ref = smp_xip_work(1);
	uint32_t round = 0;

	smp_hog_cpu_seen = arch_curr_cpu()->id;

	while (!smp_stop) {
		uint32_t v = 0x5a5a0000u + round;

		for (size_t i = 0; i < SMP_PSRAM_SIZE / sizeof(uint32_t); i++) {
			smp_psram[i] = v + i;
		}
		if (smp_xip_work(1) != ref) {
			smp_hog_errs++;
		}
		for (size_t i = 0; i < SMP_PSRAM_SIZE / sizeof(uint32_t); i++) {
			if (smp_psram[i] != v + i) {
				smp_hog_errs++;
				break;
			}
		}
		if (arch_curr_cpu()->id != smp_hog_cpu_seen) {
			smp_hog_errs++;
		}
		smp_hog_iters++;
		round++;
	}
}

static void smp_flasher_fn(void *a, void *b, void *c)
{
	struct flash_pages_info info;
	int rc;

	smp_flash_cpu_seen = arch_curr_cpu()->id;

	rc = flash_get_page_info_by_idx(smp_flash_dev, SMP_FLASH_PAGE, &info);
	if (rc) {
		smp_flash_errs++;
		return;
	}

	for (uint32_t r = 0; !smp_stop; r++) {
		memset(smp_wbuf, (uint8_t)(r + smp_flash_cpu_seen), sizeof(smp_wbuf));
		rc = flash_erase(smp_flash_dev, info.start_offset, info.size);
		if (rc == 0) {
			rc = flash_write(smp_flash_dev, info.start_offset, smp_wbuf,
					 sizeof(smp_wbuf));
		}
		if (rc == 0) {
			rc = flash_read(smp_flash_dev, info.start_offset, smp_rbuf,
					sizeof(smp_rbuf));
		}
		if (rc || memcmp(smp_wbuf, smp_rbuf, sizeof(smp_wbuf))) {
			smp_flash_errs++;
		}
		if (arch_curr_cpu()->id != smp_flash_cpu_seen) {
			smp_flash_errs++;
		}
		smp_flash_ops++;
	}
}

K_THREAD_STACK_DEFINE(smp_hog_stack, SMP_STACK_SIZE);
K_THREAD_STACK_DEFINE(smp_flasher_stack, SMP_STACK_SIZE);
static struct k_thread smp_hog_thread;
static struct k_thread smp_flasher_thread;

static void run_phase(int flash_cpu)
{
	k_tid_t hog, flasher;

	smp_stop = false;
	smp_flash_ops = 0;
	smp_flash_errs = 0;
	smp_hog_iters = 0;
	smp_hog_errs = 0;

	hog = k_thread_create(&smp_hog_thread, smp_hog_stack, K_THREAD_STACK_SIZEOF(smp_hog_stack),
			      smp_hog_fn, NULL, NULL, NULL, SMP_PRIO, 0, K_FOREVER);
	flasher = k_thread_create(&smp_flasher_thread, smp_flasher_stack,
				  K_THREAD_STACK_SIZEOF(smp_flasher_stack), smp_flasher_fn, NULL,
				  NULL, NULL, SMP_PRIO, 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(hog, flash_cpu ? 0 : 1), "hog pin failed");
	zassert_ok(k_thread_cpu_pin(flasher, flash_cpu), "flasher pin failed");
	k_thread_start(hog);
	k_thread_start(flasher);

	k_msleep(SMP_PHASE_MS);
	smp_stop = true;
	k_thread_join(flasher, K_FOREVER);
	k_thread_join(hog, K_FOREVER);

	TC_PRINT("flash on cpu%d: flash ops %u errs %u, hog iters %u errs %u\n", flash_cpu,
		 smp_flash_ops, smp_flash_errs, smp_hog_iters, smp_hog_errs);

	zassert_equal(smp_flash_cpu_seen, flash_cpu, "flasher did not run on cpu%d", flash_cpu);
	zassert_equal(smp_hog_cpu_seen, flash_cpu ? 0 : 1, "hog did not run on the other cpu");
	zassert_true(smp_flash_ops > 0, "no flash operation completed");
	zassert_true(smp_hog_iters > 0, "the hog made no progress");
	zassert_equal(smp_flash_errs, 0, "flash integrity errors");
	zassert_equal(smp_hog_errs, 0, "psram or xip integrity errors");
}

ZTEST(cache_coex_smp, test_cross_core_flash_vs_psram)
{
	zassert_true(arch_num_cpus() > 1, "needs two cpus");
	zassert_true(device_is_ready(smp_flash_dev), "flash controller not ready");

	smp_psram = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 32, SMP_PSRAM_SIZE);
	zassert_not_null(smp_psram, "SPIRAM allocation failed");
	zassert_true(esp_ptr_external_ram(smp_psram), "buffer is not in external RAM");

	run_phase(1);
	run_phase(0);

	shared_multi_heap_free(smp_psram);
}

ZTEST_SUITE(cache_coex_smp, NULL, NULL, NULL, NULL, NULL);
