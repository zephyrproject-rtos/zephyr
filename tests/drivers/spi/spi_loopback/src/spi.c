/*
 * Copyright 2025 NXP
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* To run this loopback test, connect MOSI pin to the MISO of the SPI */

/*
 ************************
 * Include dependencies *
 ************************
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdarg.h>

/*
 **********************
 * SPI configurations *
 **********************
 */

#define FRAME_SIZE COND_CODE_1(CONFIG_SPI_LOOPBACK_16BITS_FRAMES, (16), (8))
#define MODE_LOOP  COND_CODE_1(CONFIG_SPI_LOOPBACK_MODE_LOOP, (SPI_MODE_LOOP), (0))

#define SPI_OP(frame_size)                                                                         \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | MODE_LOOP | SPI_MODE_CPHA |                           \
		SPI_WORD_SET(frame_size) | SPI_LINES_SINGLE

#define SPI_FAST_DEV DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback_fast)
static struct spi_dt_spec spi_fast = SPI_DT_SPEC_GET(SPI_FAST_DEV, SPI_OP(FRAME_SIZE), 0);

#define SPI_SLOW_DEV DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback_slow)
static struct spi_dt_spec spi_slow = SPI_DT_SPEC_GET(SPI_SLOW_DEV, SPI_OP(FRAME_SIZE), 0);

static struct spi_dt_spec *loopback_specs[2] = {&spi_slow, &spi_fast};
static char *spec_names[2] = {"SLOW", "FAST"};
static int spec_idx;

/*
 ********************
 * SPI test buffers *
 ********************
 */

#if CONFIG_NOCACHE_MEMORY
#define __NOCACHE	__attribute__((__section__(".nocache")))
#elif defined(CONFIG_DT_DEFINED_NOCACHE)
#define __NOCACHE	__attribute__((__section__(CONFIG_DT_DEFINED_NOCACHE_NAME)))
#else /* CONFIG_NOCACHE_MEMORY */
#define __NOCACHE
#endif /* CONFIG_NOCACHE_MEMORY */

#define BUF_SIZE 18
static const char tx_data[BUF_SIZE] = "0123456789abcdef-\0";
static __aligned(32) char buffer_tx[BUF_SIZE] __NOCACHE;
static __aligned(32) char buffer_rx[BUF_SIZE] __NOCACHE;

#define BUF2_SIZE 36
static const char tx2_data[BUF2_SIZE] = "Thequickbrownfoxjumpsoverthelazydog\0";
static __aligned(32) char buffer2_tx[BUF2_SIZE] __NOCACHE;
static __aligned(32) char buffer2_rx[BUF2_SIZE] __NOCACHE;

#define BUF3_SIZE CONFIG_SPI_LARGE_BUFFER_SIZE
static const char large_tx_data[BUF3_SIZE] = "Thequickbrownfoxjumpsoverthelazydog\0";
static __aligned(32) char large_buffer_tx[BUF3_SIZE] __NOCACHE;
static __aligned(32) char large_buffer_rx[BUF3_SIZE] __NOCACHE;

/*
 ********************
 * Helper functions *
 ********************
 */

/*
 * We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
#define PRINT_BUF_SIZE(size) ((size * 5) + 1)

static uint8_t buffer_print_tx[PRINT_BUF_SIZE(BUF_SIZE)];
static uint8_t buffer_print_rx[PRINT_BUF_SIZE(BUF_SIZE)];

static uint8_t buffer_print_tx2[PRINT_BUF_SIZE(BUF2_SIZE)];
static uint8_t buffer_print_rx2[PRINT_BUF_SIZE(BUF2_SIZE)];

/* function for displaying the data in the buffers */
static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

/* just a wrapper of the driver transceive call with ztest error assert */
static void spi_loopback_transceive(struct spi_dt_spec *const spec,
				    const struct spi_buf_set *const tx,
				    const struct spi_buf_set *const rx)
{
	int ret = spi_transceive_dt(spec, tx, rx);

	zassert_false(ret, "SPI transceive failed, code %d", ret);
}

/* The most spi buf currently used by any test case is 4, change if needed */
#define MAX_SPI_BUF_COUNT 4
struct spi_buf tx_bufs_pool[MAX_SPI_BUF_COUNT];
struct spi_buf rx_bufs_pool[MAX_SPI_BUF_COUNT];

/* A function for creating a spi_buf_set. Simply provide the spi_buf pool (either rx or tx),
 * the number of bufs that will be in the set, and then an ordered list of pairs of buf
 * pointer (void *) and buf size (size_t).
 */
static const struct spi_buf_set spi_loopback_setup_xfer(struct spi_buf *pool, size_t num_bufs, ...)
{
	struct spi_buf_set buf_set;

	zassert_true(num_bufs <= MAX_SPI_BUF_COUNT, "SPI xfer need more buf in test");
	zassert_true(pool == tx_bufs_pool || pool == rx_bufs_pool, "Invalid spi buf pool");

	va_list args;

	va_start(args, num_bufs);

	for (int i = 0; i < num_bufs; i++) {
		pool[i].buf = va_arg(args, void *);
		pool[i].len = va_arg(args, size_t);
	}

	va_end(args);

	buf_set.buffers = pool;
	buf_set.count = num_bufs;

	return buf_set;
}

/* compare two buffers and print fail if they are not the same */
static void spi_loopback_compare_bufs(const uint8_t *buf1, const uint8_t *buf2, size_t size,
				      uint8_t *printbuf1, uint8_t *printbuf2)
{
	if (memcmp(buf1, buf2, size)) {
		to_display_format(buf1, size, printbuf1);
		to_display_format(buf2, size, printbuf2);
		TC_PRINT("Buffer contents are different:\n %s\nvs:\n %s\n",
				buffer_print_tx, buffer_print_rx);
		ztest_test_fail();
	}
}

/*
 **************
 * Test cases *
 **************
 */

/* test transferring different buffers on the same dma channels */
ZTEST(spi_loopback, test_spi_complete_multiple)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 2,
							      buffer_tx, BUF_SIZE,
							      buffer2_tx, BUF2_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 2,
							      buffer_rx, BUF_SIZE,
							      buffer2_rx, BUF2_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, BUF_SIZE,
				  buffer_print_tx, buffer_print_rx);
	spi_loopback_compare_bufs(buffer2_tx, buffer2_rx, BUF2_SIZE,
				  buffer_print_tx2, buffer_print_rx2);
}

ZTEST(spi_loopback, test_spi_complete_loop)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, BUF_SIZE,
				  buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_null_tx_buf)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	static const uint8_t expected_nop_return_buf[BUF_SIZE] = { 0 };
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      NULL, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	(void)memset(buffer_rx, 0x77, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	spi_loopback_compare_bufs(expected_nop_return_buf, buffer_rx, BUF_SIZE,
				  buffer_print_rx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_half_start)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, 8);

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, 8,
				  buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_half_end)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		TC_PRINT("Skipped spi_rx_hald_end");
		return;
	}

	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 2,
							      NULL, 8,
							      buffer_rx, 8);

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	spi_loopback_compare_bufs(buffer_tx+8, buffer_rx, 8,
				  buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_every_4)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA) || IS_ENABLED(CONFIG_DSPI_MCUX_EDMA)) {
		TC_PRINT("Skipped spi_rx_every_4");
		return;
	};

	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 4,
							      NULL, 4,
							      buffer_rx, 4,
							      NULL, 4,
							      buffer_rx+4, 4);

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	spi_loopback_compare_bufs(buffer_tx+4, buffer_rx, 4,
				  buffer_print_tx, buffer_print_rx);
	spi_loopback_compare_bufs(buffer_tx+12, buffer_rx+4, 4,
				  buffer_print_tx, buffer_print_rx);
}

ZTEST(spi_loopback, test_spi_rx_bigger_than_tx)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA) || IS_ENABLED(CONFIG_DSPI_MCUX_EDMA)) {
		TC_PRINT("Skipped spi_rx_bigger_than_tx");
		return;
	}

	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const uint32_t tx_buf_size = 8;

	BUILD_ASSERT(tx_buf_size < BUF_SIZE,
		"Transmit buffer is expected to be smaller than the receive buffer");

	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, tx_buf_size);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	(void)memset(buffer_rx, 0xff, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, tx_buf_size,
				  buffer_print_tx, buffer_print_rx);

	static const uint8_t all_zeroes_buf[BUF_SIZE] = {0};

	spi_loopback_compare_bufs(all_zeroes_buf, buffer_rx + tx_buf_size, BUF_SIZE - tx_buf_size,
				  buffer_print_tx, buffer_print_rx);
}

/* test transferring different buffers on the same dma channels */
ZTEST(spi_loopback, test_spi_complete_large_transfers)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      large_buffer_tx, BUF3_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      large_buffer_rx, BUF3_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	zassert_false(memcmp(large_buffer_tx, large_buffer_rx, BUF3_SIZE),
			"Large Buffer contents are different");
}

ZTEST(spi_loopback, test_nop_nil_bufs)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1, NULL, 0);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1, NULL, 0);

	spi_loopback_transceive(spec, &tx, &rx);

	/* nothing really to check here, check is done in spi_loopback_transceive */
}

#if (CONFIG_SPI_ASYNC)
static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);
static K_SEM_DEFINE(caller, 0, 1);
static K_SEM_DEFINE(start_async, 0, 1);
static int result = 1;

static void spi_async_call_cb(void *p1,
			      void *p2,
			      void *p3)
{
	ARG_UNUSED(p3);

	struct k_poll_event *evt = p1;
	struct k_sem *caller_sem = p2;

	TC_PRINT("Polling...");

	while (1) {
		k_sem_take(&start_async, K_FOREVER);

		zassert_false(k_poll(evt, 1, K_MSEC(2000)), "one or more events are not ready");

		result = evt->signal->result;
		k_sem_give(caller_sem);

		/* Reinitializing for next call */
		evt->signal->signaled = 0U;
		evt->state = K_POLL_STATE_NOT_READY;
	}
}

ZTEST(spi_loopback, test_spi_async_call)
{
	struct spi_dt_spec *spec = loopback_specs[spec_idx];
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 3,
							      buffer_tx, BUF_SIZE,
							      buffer2_tx, BUF2_SIZE,
							      large_buffer_tx, BUF3_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 3,
							      buffer_rx, BUF_SIZE,
							      buffer2_rx, BUF2_SIZE,
							      large_buffer_rx, BUF3_SIZE);

	memset(buffer_rx, 0, sizeof(buffer_rx));
	memset(buffer2_rx, 0, sizeof(buffer2_rx));
	memset(large_buffer_rx, 0, sizeof(large_buffer_rx));

	k_sem_give(&start_async);

	int ret = spi_transceive_signal(spec->bus, &spec->config, &tx, &rx, &async_sig);

	if (ret == -ENOTSUP) {
		TC_PRINT("Skipping ASYNC test");
		return;
	}

	zassert_false(ret, "SPI transceive failed, code %d", ret);

	k_sem_take(&caller, K_FOREVER);

	zassert_false(result, "SPI async transceive failed, result %d", result);

	spi_loopback_compare_bufs(buffer_tx, buffer_rx, BUF_SIZE,
				  buffer_print_tx, buffer_print_rx);
	spi_loopback_compare_bufs(buffer2_tx, buffer2_rx, BUF2_SIZE,
				  buffer_print_tx2, buffer_print_rx2);

	zassert_false(memcmp(large_buffer_tx, large_buffer_rx, BUF3_SIZE),
			"Large Buffer contents are different");
}
#endif

ZTEST(spi_extra_api_features, test_spi_lock_release)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      NULL, BUF_SIZE);
	struct spi_dt_spec *lock_spec = &spi_slow;
	struct spi_dt_spec *try_spec = &spi_fast;

	lock_spec->config.operation |= SPI_LOCK_ON;

	spi_loopback_transceive(lock_spec, &tx, &rx);

	zassert_false(spi_release_dt(lock_spec), "SPI release failed");

	spi_loopback_transceive(try_spec, &tx, &rx);

	lock_spec->config.operation &= ~SPI_LOCK_ON;
}

/*
 *************************
 * Test suite definition *
 *************************
 */

static void *spi_loopback_common_setup(void)
{
	memset(buffer_tx, 0, sizeof(buffer_tx));
	memcpy(buffer_tx, tx_data, sizeof(tx_data));
	memset(buffer2_tx, 0, sizeof(buffer2_tx));
	memcpy(buffer2_tx, tx2_data, sizeof(tx2_data));
	memset(large_buffer_tx, 0, sizeof(large_buffer_tx));
	memcpy(large_buffer_tx, large_tx_data, sizeof(large_tx_data));
	return NULL;
}

static void *spi_loopback_setup(void)
{
	printf("Testing loopback spec: %s\n", spec_names[spec_idx]);
	spi_loopback_common_setup();
	return NULL;
}

static void run_after_suite(void *unused)
{
	spec_idx++;
}

static void run_after_lock(void *unused)
{
	spi_release_dt(&spi_fast);
	spi_release_dt(&spi_slow);
	spi_slow.config.operation &= ~SPI_LOCK_ON;
	spi_fast.config.operation &= ~SPI_LOCK_ON;
}

ZTEST_SUITE(spi_loopback, NULL, spi_loopback_setup, NULL, NULL, run_after_suite);
ZTEST_SUITE(spi_extra_api_features, NULL, spi_loopback_common_setup, NULL, NULL, run_after_lock);

struct k_thread async_thread;
k_tid_t async_thread_id;
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(spi_async_stack, STACK_SIZE);

void test_main(void)
{
	printf("SPI test on buffers TX/RX %p/%p, frame size = %d"
#ifdef CONFIG_DMA
		", DMA enabled"
#ifndef CONFIG_NOCACHE_MEMORY
		" (without CONFIG_NOCACHE_MEMORY)"
#endif
#endif /* CONFIG_DMA */
			"\n",
			buffer_tx,
			buffer_rx,
			FRAME_SIZE);

#if (CONFIG_SPI_ASYNC)
	async_thread_id = k_thread_create(&async_thread,
					  spi_async_stack, STACK_SIZE,
					  spi_async_call_cb,
					  &async_evt, &caller, NULL,
					  K_PRIO_COOP(7), 0, K_NO_WAIT);
#endif

	ztest_run_all(NULL, false, ARRAY_SIZE(loopback_specs), 1);

#if (CONFIG_SPI_ASYNC)
	k_thread_abort(async_thread_id);
#endif

	ztest_verify_all_test_suites_ran();
}
