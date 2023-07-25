/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/random/rand32.h>
#include <soc/soc_memory_layout.h>

/* definitions used in Flash & RAM operations */
#define SPIRAM_ALLOC_SIZE               (24 * 1024)
#define FLASH_PAGE_TESTED               (1023)
#define FLASH_PAGE_OFFSET               (0)
#define FLASH_BYTE_PATTERN              (0x38)
#define FLASH_READBACK_LEN              (1024)
#define FLASH_ITERATIONS                (10)
#define PSRAM_ITERATIONS                (10)

/* common thread definitions */
#define STACKSIZE 1024
#define PRIORITY 7

static const struct device *const flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
static struct flash_pages_info page_info;
static int *mem;
uint8_t flash_fill_buff[FLASH_READBACK_LEN];
uint8_t flash_read_buff[FLASH_READBACK_LEN];

static uint8_t flash_val = FLASH_BYTE_PATTERN;
static bool buffer_ready;
static bool needs_fill = true;
static bool unfinished_tasks = true;
static struct k_spinlock lock;

struct coex_test_results {
	bool using_ext_ram;
	int flash_cnt;
	bool psram_ok;
};
static struct coex_test_results coex_result;

static void buffer_fill(void)
{
	while (needs_fill) {
		if (!buffer_ready) {
			k_spinlock_key_t key = k_spin_lock(&lock);

			memset(flash_fill_buff, ++flash_val, sizeof(flash_fill_buff));
			buffer_ready = true;
			k_spin_unlock(&lock, key);
		}
		k_usleep(10);
	}
	while (true) {
		k_usleep(1);
	}
}

static void check_flash(void)
{
	int cnt = 0;

	for (size_t i = 0; i < sizeof(flash_read_buff); ++i) {
		if (flash_read_buff[i] == (FLASH_BYTE_PATTERN + FLASH_ITERATIONS)) {
			++cnt;
		}
	}
	coex_result.flash_cnt = cnt;
	unfinished_tasks = false;
}

static int do_erase(off_t offset, size_t size)
{
	int rc;

	rc = flash_erase(flash_dev, offset, size);
	if (rc) {
		TC_ERROR("flash erase has failed\n");
		return rc;
	}

	return rc;
}

static int page_erase(void)
{
	int rc;

	rc = flash_get_page_info_by_idx(flash_dev, FLASH_PAGE_TESTED, &page_info);
	if (rc) {
		return rc;
	}

	rc = do_erase(page_info.start_offset, page_info.size);
	if (rc) {
		return rc;
	}

	return rc;
}

static int page_read(void)
{
	int rc = flash_get_page_info_by_idx(flash_dev, FLASH_PAGE_TESTED, &page_info);

	if (rc) {
		TC_ERROR("could not read flash info\n");
		return rc;
	}
	rc = flash_read(flash_dev,
			page_info.start_offset + FLASH_PAGE_OFFSET,
			flash_read_buff,
			sizeof(flash_read_buff));
	if (rc) {
		TC_ERROR("flash read back has failed\n");
		return rc;
	}

	return 0;
}

static int page_write(void)
{
	int rc = flash_get_page_info_by_idx(flash_dev, FLASH_PAGE_TESTED, &page_info);

	if (rc) {
		TC_ERROR("could not retrieve flash info\n");
		return rc;
	}

	rc = flash_write(flash_dev,
			page_info.start_offset + FLASH_PAGE_OFFSET,
			flash_fill_buff,
			sizeof(flash_fill_buff));
	if (rc) {
		TC_ERROR("could not write to flash\n");
		return rc;
	}

	return 0;
}

static bool do_flashop(void)
{
	int rc = page_write();

	if (rc) {
		return false;
	}

	return true;
}

static void fill_value(int value)
{
	int i;

	/* fills external RAM with given value */
	for (i = 0; i < (SPIRAM_ALLOC_SIZE / sizeof(int)); ++i) {
		mem[i] = value;
	}
}

static bool check_psram(int value)
{
	int *ptr = mem;

	/* checks if external RAM is filled with the expected value */
	for (size_t i = 0; i < (SPIRAM_ALLOC_SIZE / sizeof(int)); ++i) {
		if (*ptr++ != value) {
			return false;
		}
	}
	return true;
}

static void psram_test(void)
{
	uint32_t sleep_ms = 10;
	int rand_val;

	for (size_t i = 0; i < PSRAM_ITERATIONS; ++i) {
		rand_val = (int)sys_rand32_get();
		fill_value(rand_val);
		k_msleep(sleep_ms);
	}
	if (check_psram(rand_val)) {
		coex_result.psram_ok = true;
	} else {
		coex_result.psram_ok = false;
	}
	while (true) {
		k_usleep(1);
	}
}

static void psram_init(void)
{
	mem = k_malloc(SPIRAM_ALLOC_SIZE);
	if (!mem) {
		TC_ERROR("SPIRAM allocation has failed\n");
	}

	if (!esp_ptr_external_ram(mem)) {
		coex_result.using_ext_ram = false;
		TC_ERROR("allocation is not within specified bounds\n");
		return;
	}

	coex_result.using_ext_ram = true;
	psram_test();
}

static void flash_test(void)
{
	uint32_t sleep_ms = 15;

	for (size_t i = 0; i < FLASH_ITERATIONS; ++i) {
		page_erase();
		if (buffer_ready) {
			k_spinlock_key_t key = k_spin_lock(&lock);

			do_flashop();
			buffer_ready = false;
			k_spin_unlock(&lock, key);
		}
		k_msleep(sleep_ms);
	}
	needs_fill = false;
	page_read();
	check_flash();
	while (true) {
		k_usleep(1);
	}
}

static void flash_init(void)
{
	if (!device_is_ready(flash_dev)) {
		TC_ERROR("flash controller not ready\n");
	}
	flash_test();
}

ZTEST(cache_coex, test_using_spiram)
{
	zassert_equal(true, coex_result.using_ext_ram, "external RAM is not being used");
}

ZTEST(cache_coex, test_flash_integrity)
{
	zassert_equal(FLASH_READBACK_LEN, coex_result.flash_cnt, "flash integrity test failed");
}

ZTEST(cache_coex, test_ram_integrity)
{
	zassert_equal(true, coex_result.psram_ok, "SPIRAM integrity test failed");
}

void *cache_coex_setup(void)
{
	while (unfinished_tasks) {
		k_usleep(1);
	}

	return NULL;
}

K_THREAD_DEFINE(psram_id, STACKSIZE, psram_init, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(flash_id, STACKSIZE, flash_init, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(buffer_id, STACKSIZE, buffer_fill, NULL, NULL, NULL, PRIORITY, 0, 0);

ZTEST_SUITE(cache_coex, NULL, cache_coex_setup, NULL, NULL, NULL);
