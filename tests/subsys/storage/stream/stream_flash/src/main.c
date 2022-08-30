/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/settings/settings.h>

#include <zephyr/storage/stream_flash.h>

#define BUF_LEN 512
#define MAX_PAGE_SIZE 0x1000 /* Max supported page size to run test on */
#define MAX_NUM_PAGES 4      /* Max number of pages used in these tests */
#define TESTBUF_SIZE (MAX_PAGE_SIZE * MAX_NUM_PAGES)
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)
#define FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

/* so that we don't overwrite the application when running on hw */
#define FLASH_BASE (128*1024)
#define FLASH_AVAILABLE (FLASH_SIZE-FLASH_BASE)

static const struct device *const fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
static const struct flash_driver_api *api;
static const struct flash_pages_layout *layout;
static size_t layout_size;
static struct stream_flash_ctx ctx;
static int page_size;
static uint8_t *cb_buf;
static size_t cb_len;
static size_t cb_offset;
static int cb_ret;

static const char progress_key[] = "sf-test/progress";

static uint8_t buf[BUF_LEN];
static uint8_t read_buf[TESTBUF_SIZE];
const static uint8_t write_buf[TESTBUF_SIZE] = {[0 ... TESTBUF_SIZE - 1] = 0xaa};
static uint8_t written_pattern[TESTBUF_SIZE] = {[0 ... TESTBUF_SIZE - 1] = 0xaa};
static uint8_t erased_pattern[TESTBUF_SIZE]  = {[0 ... TESTBUF_SIZE - 1] = 0xff};

#define VERIFY_BUF(start, size, buf) \
do { \
	rc = flash_read(fdev, FLASH_BASE + start, read_buf, size); \
	zassert_equal(rc, 0, "should succeed"); \
	zassert_mem_equal(read_buf, buf, size, "should equal %s", #buf);\
} while (0)

#define VERIFY_WRITTEN(start, size) VERIFY_BUF(start, size, written_pattern)
#define VERIFY_ERASED(start, size) VERIFY_BUF(start, size, erased_pattern)

int stream_flash_callback(uint8_t *buf, size_t len, size_t offset)
{
	if (cb_buf) {
		zassert_equal(cb_buf, buf, "incorrect buf");
		zassert_equal(cb_len, len, "incorrect length");
		zassert_equal(cb_offset, offset, "incorrect offset");
	}

	return cb_ret;
}

static void erase_flash(void)
{
	int rc;

	for (int i = 0; i < MAX_NUM_PAGES; i++) {
		rc = flash_erase(fdev,
				 FLASH_BASE + (i * layout->pages_size),
				 layout->pages_size);
		zassert_equal(rc, 0, "should succeed");
	}
}


static void init_target(void)
{
	int rc;

	/* Ensure that target is clean */
	memset(&ctx, 0, sizeof(ctx));
	memset(buf, 0, BUF_LEN);

	/* Disable callback tests */
	cb_len = 0;
	cb_offset = 0;
	cb_buf = NULL;
	cb_ret = 0;

	erase_flash();

	rc = stream_flash_init(&ctx, fdev, buf, BUF_LEN, FLASH_BASE, 0,
			       stream_flash_callback);
	zassert_equal(rc, 0, "expected success");
}

ZTEST(lib_stream_flash, test_stream_flash_init)
{
	int rc;

	init_target();

	/* End address out of range */
	rc = stream_flash_init(&ctx, fdev, buf, BUF_LEN, FLASH_BASE,
		      FLASH_AVAILABLE + 4, NULL);
	zassert_true(rc < 0, "should fail as size is more than available");

	rc = stream_flash_init(NULL, fdev, buf, BUF_LEN, FLASH_BASE, 0, NULL);
	zassert_true(rc < 0, "should fail as ctx is NULL");

	rc = stream_flash_init(&ctx, NULL, buf, BUF_LEN, FLASH_BASE, 0, NULL);
	zassert_true(rc < 0, "should fail as fdev is NULL");

	rc = stream_flash_init(&ctx, fdev, NULL, BUF_LEN, FLASH_BASE, 0, NULL);
	zassert_true(rc < 0, "should fail as buffer is NULL");

	/* Entering '0' as flash size uses rest of flash. */
	rc = stream_flash_init(&ctx, fdev, buf, BUF_LEN, FLASH_BASE, 0, NULL);
	zassert_equal(rc, 0, "should succeed");
	zassert_equal(FLASH_AVAILABLE, ctx.available, "Wrong size");
}

ZTEST(lib_stream_flash, test_stream_flash_buffered_write)
{
	int rc;

	init_target();

	/* Don't fill up the buffer */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN - 1, false);
	zassert_equal(rc, 0, "expected success");

	/* Verify that no data has been written */
	VERIFY_ERASED(0, BUF_LEN);

	/* Now, write the missing byte, which should trigger a dump to flash */
	rc = stream_flash_buffered_write(&ctx, write_buf, 1, false);
	zassert_equal(rc, 0, "expected success");

	VERIFY_WRITTEN(0, BUF_LEN);
}

ZTEST(lib_stream_flash, test_stream_flash_buffered_write_cross_buf_border)
{
	int rc;

	init_target();

	/* Test when write crosses border of the buffer */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN + 128, false);
	zassert_equal(rc, 0, "expected success");

	/* 1xBuffer should be dumped to flash */
	VERIFY_WRITTEN(0, BUF_LEN);

	/* Fill rest of the buffer */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN - 128, false);
	zassert_equal(rc, 0, "expected success");
	VERIFY_WRITTEN(BUF_LEN, BUF_LEN);

	/* Fill half of the buffer */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN/2, false);
	zassert_equal(rc, 0, "expected success");

	/* Flush the buffer */
	rc = stream_flash_buffered_write(&ctx, write_buf, 0, true);
	zassert_equal(rc, 0, "expected success");

	/* Two and a half buffers should be written */
	VERIFY_WRITTEN(0, BUF_LEN * 2 + BUF_LEN / 2);
}

ZTEST(lib_stream_flash, test_stream_flash_buffered_write_unaligned)
{
	int rc;

	if (flash_get_write_block_size(fdev) == 1) {
		ztest_test_skip();
	}

	init_target();

	/* Test unaligned data size */
	rc = stream_flash_buffered_write(&ctx, write_buf, 1, true);
	zassert_equal(rc, 0, "expected success (%d)", rc);

	/* 1 byte should be dumped to flash */
	VERIFY_WRITTEN(0, 1);

	rc = stream_flash_init(&ctx, fdev, buf, BUF_LEN, FLASH_BASE + BUF_LEN,
			       0, stream_flash_callback);
	zassert_equal(rc, 0, "expected success");

	/* Trigger verification in callback */
	cb_buf = buf;
	cb_len = BUF_LEN - 1;
	cb_offset = FLASH_BASE + BUF_LEN;

	/* Test unaligned data size */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN - 1, true);
	zassert_equal(rc, 0, "expected success");

	/* BUF_LEN-1 bytes should be dumped to flash */
	VERIFY_WRITTEN(BUF_LEN, BUF_LEN - 1);
}

ZTEST(lib_stream_flash, test_stream_flash_buffered_write_multi_page)
{
	int rc;
	int num_pages = MAX_NUM_PAGES - 1;

	init_target();

	/* Test when write spans multiple pages crosses border of page */
	rc = stream_flash_buffered_write(&ctx, write_buf,
					 (page_size * num_pages) + 128, false);
	zassert_equal(rc, 0, "expected success");

	/* First three pages should be written */
	VERIFY_WRITTEN(0, page_size * num_pages);

	/* Fill rest of the page */
	rc = stream_flash_buffered_write(&ctx, write_buf,
					 page_size - 128, false);
	zassert_equal(rc, 0, "expected success");

	/* First four pages should be written */
	VERIFY_WRITTEN(0, BUF_LEN * (num_pages + 1));
}

ZTEST(lib_stream_flash, test_stream_flash_bytes_written)
{
	int rc;
	size_t offset;

	init_target();

	/* Verify that the offset is retained across failed downloads */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN + 128, false);
	zassert_equal(rc, 0, "expected success");

	/* First page should be written */
	VERIFY_WRITTEN(0, BUF_LEN);

	/* Fill rest of the page */
	offset = stream_flash_bytes_written(&ctx);
	zassert_equal(offset, BUF_LEN, "offset should match buf size");

	/* Fill up the buffer MINUS 128 to verify that write_buf_pos is kept */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN - 128, false);
	zassert_equal(rc, 0, "expected success");

	/* Second page should be written */
	VERIFY_WRITTEN(BUF_LEN, BUF_LEN);
}

ZTEST(lib_stream_flash, test_stream_flash_buf_size_greater_than_page_size)
{
	int rc;

	/* To illustrate that other params does not trigger error */
	rc = stream_flash_init(&ctx, fdev, buf, 0x10, 0, 0, NULL);
	zassert_equal(rc, 0, "expected success");

	/* Only change buf_len param */
	rc = stream_flash_init(&ctx, fdev, buf, 0x10000, 0, 0, NULL);
	zassert_true(rc < 0, "expected failure");
}

static int bad_read(const struct device *dev, off_t off, void *data, size_t len)
{
	return -EINVAL;
}

static int fake_write(const struct device *dev, off_t off, const void *data, size_t len)
{
	return 0;
}

static int bad_write(const struct device *dev, off_t off, const void *data, size_t len)
{
	return -EINVAL;
}

ZTEST(lib_stream_flash, test_stream_flash_buffered_write_callback)
{
	int rc;

	init_target();

	/* Trigger verification in callback */
	cb_buf = buf;
	cb_len = BUF_LEN;
	cb_offset = FLASH_BASE;

	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN + 128, false);
	zassert_equal(rc, 0, "expected success");

	cb_len = BUF_LEN;
	cb_offset = FLASH_BASE + BUF_LEN;

	/* Fill rest of the buffer */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN - 128, false);
	zassert_equal(rc, 0, "expected success");
	VERIFY_WRITTEN(BUF_LEN, BUF_LEN);

	/* Fill half of the buffer and flush it to flash */
	cb_len = BUF_LEN/2;
	cb_offset = FLASH_BASE + (2 * BUF_LEN);

	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN/2, true);
	zassert_equal(rc, 0, "expected success");

	/* Ensure that failing callback trickles up to caller */
	cb_ret = -EFAULT;
	cb_buf = NULL; /* Don't verify other parameters of the callback */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN, true);
	zassert_equal(rc, -EFAULT, "expected failure from callback");
	/* Expect that the BUF_LEN of bytes got stuck in buffer as the  verification callback
	 * failed.
	 */
	zassert_equal(ctx.buf_bytes, BUF_LEN, "Expected bytes to be left in buffer");

	struct device fake_dev = *ctx.fdev;
	struct flash_driver_api fake_api = *(struct flash_driver_api *)ctx.fdev->api;
	struct stream_flash_ctx bad_ctx = ctx;
	struct stream_flash_ctx cmp_ctx;

	fake_api.read = bad_read;
	/* Using fake write here because after previous write, with faked callback failure,
	 * the flash is already written and real flash_write would cause failure.
	 */
	fake_api.write = fake_write;
	fake_dev.api = &fake_api;
	bad_ctx.fdev = &fake_dev;
	/* Trigger erase attempt */
	cmp_ctx = bad_ctx;
	/* Just flush buffer */
	rc = stream_flash_buffered_write(&bad_ctx, write_buf, 0, true);
	zassert_equal(rc, -EINVAL, "expected failure from flash_sync", rc);
	zassert_equal(ctx.buf_bytes, BUF_LEN, "Expected bytes to be left in buffer");

	/* Pretend flashed context and attempt write write block - 1 bytes to trigger unaligned
	 * write; the write needs to fail so that we could check that context does not get modified.
	 */
	fake_api.write = bad_write;
	bad_ctx.callback = NULL;
	bad_ctx.buf_bytes = 0;
	cmp_ctx = bad_ctx;
	size_t wblock = flash_get_write_block_size(ctx.fdev);
	size_t tow = (wblock == 1) ? 1 : wblock - 1;

	rc = stream_flash_buffered_write(&bad_ctx, write_buf, tow, true);
	zassert_equal(rc, -EINVAL, "expected failure from flash_sync", rc);
	zassert_equal(cmp_ctx.bytes_written, bad_ctx.bytes_written,
		      "Expected bytes_written not modified");
	/* The write failed but bytes have already been added to buffer and buffer offset
	 * increased.
	 */
	zassert_equal(bad_ctx.buf_bytes, cmp_ctx.buf_bytes + tow,
		      "Expected %d bytes added to buffer", tow);
}

ZTEST(lib_stream_flash, test_stream_flash_flush)
{
	int rc;

	init_target();

	/* Perform flush with NULL data pointer and 0 length */
	rc = stream_flash_buffered_write(&ctx, NULL, 0, true);
	zassert_equal(rc, 0, "expected success");
}

#ifdef CONFIG_STREAM_FLASH_ERASE
ZTEST(lib_stream_flash, test_stream_flash_buffered_write_whole_page)
{
	int rc;

	init_target();

	/* Write all bytes of a page, verify that next page is not erased */

	/* First fill two pages with data */
	rc = stream_flash_buffered_write(&ctx, write_buf, page_size * 2, true);
	zassert_equal(rc, 0, "expected success");

	VERIFY_WRITTEN(0, page_size);
	VERIFY_WRITTEN(page_size, page_size);

	/* Reset stream_flash context */
	memset(&ctx, 0, sizeof(ctx));
	memset(buf, 0, BUF_LEN);
	rc = stream_flash_init(&ctx, fdev, buf, BUF_LEN, FLASH_BASE, 0,
			       stream_flash_callback);
	zassert_equal(rc, 0, "expected success");

	/* Write all bytes of a page, verify that next page is not erased */
	rc = stream_flash_buffered_write(&ctx, write_buf, page_size, true);
	zassert_equal(rc, 0, "expected success");

	/* Second page should not be erased */
	VERIFY_WRITTEN(page_size, page_size);
}

/* Erase that never completes successfully */
static int bad_erase(const struct device *dev, off_t offset, size_t size)
{
	return -EINVAL;
}

ZTEST(lib_stream_flash, test_stream_flash_erase_page)
{
	int rc;

	init_target();

	/* Write out one buf */
	rc = stream_flash_buffered_write(&ctx, write_buf, BUF_LEN, false);
	zassert_equal(rc, 0, "expected success");

	rc = stream_flash_erase_page(&ctx, FLASH_BASE);
	zassert_equal(rc, 0, "expected success");

	VERIFY_ERASED(FLASH_BASE, page_size);

	/*
	 * Test failure in erase does not change context.
	 * The test is done by replacing erase function of device API with fake
	 * one that returns with an error, invoking the erase procedure
	 * and than comparing state of context prior to call to the one after.
	 */
	struct device fake_dev = *ctx.fdev;
	struct flash_driver_api fake_api = *(struct flash_driver_api *)ctx.fdev->api;
	struct stream_flash_ctx bad_ctx = ctx;
	struct stream_flash_ctx cmp_ctx;

	fake_api.erase = bad_erase;
	fake_dev.api = &fake_api;
	bad_ctx.fdev = &fake_dev;
	/* Triger erase attempt */
	bad_ctx.last_erased_page_start_offset = FLASH_BASE - 16;
	cmp_ctx = bad_ctx;

	rc = stream_flash_erase_page(&bad_ctx, FLASH_BASE);
	zassert_equal(memcmp(&bad_ctx, &cmp_ctx, sizeof(bad_ctx)), 0,
		      "Ctx should not get altered");
	zassert_equal(rc, -EINVAL, "Expected failure");
}
#else
ZTEST(lib_stream_flash, test_stream_flash_erase_page)
{
	ztest_test_skip();
}

ZTEST(lib_stream_flash, test_stream_flash_buffered_write_whole_page)
{
	ztest_test_skip();
}
#endif

static size_t write_and_save_progress(size_t bytes, const char *save_key)
{
	int rc;
	size_t bytes_written;

	rc = stream_flash_buffered_write(&ctx, write_buf, bytes, true);
	zassert_equal(rc, 0, "expected success");

	bytes_written = stream_flash_bytes_written(&ctx);
	zassert_true(bytes_written > 0, "expected bytes to be written");

	if (save_key) {
		rc = stream_flash_progress_save(&ctx, save_key);
		zassert_equal(rc, 0, "expected success");
	}

	return bytes_written;
}

static void clear_all_progress(void)
{
	(void) settings_delete(progress_key);
}

static size_t load_progress(const char *load_key)
{
	int rc;

	rc = stream_flash_progress_load(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");

	return stream_flash_bytes_written(&ctx);
}

ZTEST(lib_stream_flash, test_stream_flash_progress_api)
{
	int rc;

	clear_all_progress();
	init_target();

	/* Test save parameter validation */
	rc = stream_flash_progress_save(NULL, progress_key);
	zassert_true(rc < 0, "expected error since ctx is NULL");

	rc = stream_flash_progress_save(&ctx, NULL);
	zassert_true(rc < 0, "expected error since key is NULL");

	rc = stream_flash_progress_save(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");

	(void) write_and_save_progress(BUF_LEN, progress_key);

	/* Test load parameter validation */
	rc = stream_flash_progress_load(NULL, progress_key);
	zassert_true(rc < 0, "expected error since ctx is NULL");

	rc = stream_flash_progress_load(&ctx, NULL);
	zassert_true(rc < 0, "expected error since key is NULL");

	rc = stream_flash_progress_load(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");

	/* Test clear parameter validation */
	rc = stream_flash_progress_clear(NULL, progress_key);
	zassert_true(rc < 0, "expected error since ctx is NULL");

	rc = stream_flash_progress_clear(&ctx, NULL);
	zassert_true(rc < 0, "expected error since key is NULL");

	rc = stream_flash_progress_clear(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");
}

ZTEST(lib_stream_flash, test_stream_flash_progress_resume)
{
	int rc;
	size_t bytes_written_old;
	size_t bytes_written;
#ifdef CONFIG_STREAM_FLASH_ERASE
	off_t erase_offset_old;
	off_t erase_offset;
#endif

	clear_all_progress();
	init_target();

	bytes_written_old = stream_flash_bytes_written(&ctx);
#ifdef CONFIG_STREAM_FLASH_ERASE
	erase_offset_old = ctx.last_erased_page_start_offset;
#endif

	/* Test load with zero bytes_written */
	rc = stream_flash_progress_save(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");

	rc = stream_flash_progress_load(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");

	bytes_written = stream_flash_bytes_written(&ctx);
	zassert_equal(bytes_written, bytes_written_old,
		      "expected bytes_written to be unchanged");
#ifdef CONFIG_STREAM_FLASH_ERASE
	erase_offset = ctx.last_erased_page_start_offset;
	zassert_equal(erase_offset, erase_offset_old,
		      "expected erase offset to be unchanged");
#endif

	clear_all_progress();
	init_target();

	/* Write some data and save the progress */
	bytes_written_old = write_and_save_progress(page_size * 2,
						    progress_key);
#ifdef CONFIG_STREAM_FLASH_ERASE
	erase_offset_old = ctx.last_erased_page_start_offset;
	zassert_true(erase_offset_old != 0, "expected pages to be erased");
#endif

	init_target();

	/* Load the previous progress */
	bytes_written = load_progress(progress_key);
	zassert_equal(bytes_written, bytes_written_old,
		      "expected bytes_written to be loaded");
#ifdef CONFIG_STREAM_FLASH_ERASE
	zassert_equal(erase_offset_old, ctx.last_erased_page_start_offset,
		      "expected last erased page offset to be loaded");
#endif

	/* Check that outdated progress does not overwrite current progress */
	init_target();

	(void) write_and_save_progress(BUF_LEN, progress_key);
	bytes_written_old = write_and_save_progress(BUF_LEN, NULL);
	bytes_written = load_progress(progress_key);
	zassert_equal(bytes_written, bytes_written_old,
		      "expected bytes_written to not be overwritten");
}

ZTEST(lib_stream_flash, test_stream_flash_progress_clear)
{
	int rc;
	size_t bytes_written_old;
	size_t bytes_written;
#ifdef CONFIG_STREAM_FLASH_ERASE
	off_t erase_offset_old;
	off_t erase_offset;
#endif

	clear_all_progress();
	init_target();

	/* Test that progress is cleared. */
	(void) write_and_save_progress(BUF_LEN, progress_key);

	rc = stream_flash_progress_clear(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");

	init_target();

	bytes_written_old = stream_flash_bytes_written(&ctx);
#ifdef CONFIG_STREAM_FLASH_ERASE
	erase_offset_old = ctx.last_erased_page_start_offset;
#endif

	rc = stream_flash_progress_load(&ctx, progress_key);
	zassert_equal(rc, 0, "expected success");

	bytes_written = stream_flash_bytes_written(&ctx);
	zassert_equal(bytes_written, bytes_written_old,
		      "expected bytes_written to be unchanged");

#ifdef CONFIG_STREAM_FLASH_ERASE
	erase_offset = ctx.last_erased_page_start_offset;
	zassert_equal(erase_offset, erase_offset_old,
		      "expected erase offset to be unchanged");
#endif
}

void lib_stream_flash_before(void *data)
{
	zassume_true(device_is_ready(fdev), "Device is not ready");

	api = fdev->api;
	api->page_layout(fdev, &layout, &layout_size);

	page_size = layout->pages_size;
	zassume_true((page_size > BUF_LEN), "page size is not enough");
}

ZTEST_SUITE(lib_stream_flash, NULL, NULL, lib_stream_flash_before, NULL, NULL);
