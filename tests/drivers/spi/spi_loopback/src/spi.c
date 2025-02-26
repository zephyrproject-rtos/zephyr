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
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_loopback);

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

/*
 **************
 * Test cases *
 **************
 */

/* test transferring different buffers on the same dma channels */
static int spi_complete_multiple(struct spi_dt_spec *spec)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 2,
							      buffer_tx, BUF_SIZE,
							      buffer2_tx, BUF2_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 2,
							      buffer_rx, BUF_SIZE,
							      buffer2_rx, BUF2_SIZE);

	LOG_INF("Start complete multiple");

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		to_display_format(buffer_tx, BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	if (memcmp(buffer2_tx, buffer2_rx, BUF2_SIZE)) {
		to_display_format(buffer2_tx, BUF2_SIZE, buffer_print_tx2);
		to_display_format(buffer2_rx, BUF2_SIZE, buffer_print_rx2);
		LOG_ERR("Buffer 2 contents are different: %s", buffer_print_tx2);
		LOG_ERR("                             vs: %s", buffer_print_rx2);
		zassert_false(1, "Buffer 2 contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_complete_loop(struct spi_dt_spec *spec)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	LOG_INF("Start complete loop");

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		to_display_format(buffer_tx, BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_null_tx_buf(struct spi_dt_spec *spec)
{
	static const uint8_t EXPECTED_NOP_RETURN_BUF[BUF_SIZE] = { 0 };
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      NULL, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	(void)memset(buffer_rx, 0x77, BUF_SIZE);

	LOG_INF("Start null tx");

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(buffer_rx, EXPECTED_NOP_RETURN_BUF, BUF_SIZE)) {
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		LOG_ERR("Rx Buffer should contain NOP frames but got: %s",
			buffer_print_rx);
		zassert_false(1, "Buffer not as expected");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_rx_half_start(struct spi_dt_spec *spec)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, 8);

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(buffer_tx, buffer_rx, 8)) {
		to_display_format(buffer_tx, 8, buffer_print_tx);
		to_display_format(buffer_rx, 8, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_rx_half_end(struct spi_dt_spec *spec)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		LOG_INF("Skip half end");
		return 0;
	}

	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 2,
							      NULL, 8,
							      buffer_rx, 8);

	LOG_INF("Start half end");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(buffer_tx + 8, buffer_rx, 8)) {
		to_display_format(buffer_tx + 8, 8, buffer_print_tx);
		to_display_format(buffer_rx, 8, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_rx_every_4(struct spi_dt_spec *spec)
{
	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		LOG_INF("Skip every 4");
		return 0;
	}

	if (IS_ENABLED(CONFIG_DSPI_MCUX_EDMA)) {
		LOG_INF("Skip every 4");
		return 0;
	}

	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, BUF_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 4,
							      NULL, 4,
							      buffer_rx, 4,
							      NULL, 4,
							      buffer_rx+4, 4);

	LOG_INF("Start every 4");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(buffer_tx + 4, buffer_rx, 4)) {
		to_display_format(buffer_tx + 4, 4, buffer_print_tx);
		to_display_format(buffer_rx, 4, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	} else if (memcmp(buffer_tx + 12, buffer_rx + 4, 4)) {
		to_display_format(buffer_tx + 12, 4, buffer_print_tx);
		to_display_format(buffer_rx + 4, 4, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_rx_bigger_than_tx(struct spi_dt_spec *spec)
{
	const uint32_t tx_buf_size = 8;

	BUILD_ASSERT(tx_buf_size < BUF_SIZE,
		"Transmit buffer is expected to be smaller than the receive buffer");

	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		LOG_INF("Skip rx bigger than tx");
		return 0;
	}

	if (IS_ENABLED(CONFIG_DSPI_MCUX_EDMA)) {
		LOG_INF("Skip rx bigger than tx");
		return 0;
	}

	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      buffer_tx, tx_buf_size);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      buffer_rx, BUF_SIZE);

	LOG_INF("Start rx bigger than tx");

	(void)memset(buffer_rx, 0xff, BUF_SIZE);

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(buffer_tx, buffer_rx, tx_buf_size)) {
		to_display_format(buffer_tx, tx_buf_size, buffer_print_tx);
		to_display_format(buffer_rx, tx_buf_size, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	const uint8_t all_zeroes_buf[BUF_SIZE] = {0};

	if (memcmp(all_zeroes_buf, buffer_rx + tx_buf_size, BUF_SIZE - tx_buf_size)) {
		to_display_format(
			buffer_rx + tx_buf_size,  BUF_SIZE - tx_buf_size, buffer_print_tx);

		to_display_format(
			all_zeroes_buf, BUF_SIZE - tx_buf_size, buffer_print_rx);

		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

/* test transferring different buffers on the same dma channels */
static int spi_complete_large_transfers(struct spi_dt_spec *spec)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 1,
							      large_buffer_tx, BUF3_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 1,
							      large_buffer_rx, BUF3_SIZE);

	LOG_INF("Start complete large transfers");

	spi_loopback_transceive(spec, &tx, &rx);

	if (memcmp(large_buffer_tx, large_buffer_rx, BUF3_SIZE)) {
		zassert_false(1, "Large Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}
#if (CONFIG_SPI_ASYNC)
static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);
static K_SEM_DEFINE(caller, 0, 1);
static int result = 1;

static void spi_async_call_cb(void *p1,
			      void *p2,
			      void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct k_poll_event *evt = p1;
	struct k_sem *caller_sem = p2;
	int ret;

	LOG_DBG("Polling...");

	while (1) {
		ret = k_poll(evt, 1, K_MSEC(2000));
		zassert_false(ret, "one or more events are not ready");

		result = evt->signal->result;
		k_sem_give(caller_sem);

		/* Reinitializing for next call */
		evt->signal->signaled = 0U;
		evt->state = K_POLL_STATE_NOT_READY;
	}
}

static int spi_async_call(struct spi_dt_spec *spec)
{
	const struct spi_buf_set tx = spi_loopback_setup_xfer(tx_bufs_pool, 3,
							      buffer_tx, BUF_SIZE,
							      buffer2_tx, BUF2_SIZE,
							      large_buffer_tx, BUF3_SIZE);
	const struct spi_buf_set rx = spi_loopback_setup_xfer(rx_bufs_pool, 3,
							      buffer_rx, BUF_SIZE,
							      buffer2_rx, BUF2_SIZE,
							      large_buffer_rx, BUF3_SIZE);

	LOG_INF("Start async call");
	memset(buffer_rx, 0, sizeof(buffer_rx));
	memset(buffer2_rx, 0, sizeof(buffer2_rx));
	memset(large_buffer_rx, 0, sizeof(large_buffer_rx));

	int ret = spi_transceive_signal(spec->bus, &spec->config, &tx, &rx, &async_sig);

	if (ret == -ENOTSUP) {
		LOG_DBG("Not supported");
		return 0;
	}

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

	k_sem_take(&caller, K_FOREVER);

	if (result) {
		LOG_ERR("Call code %d", ret);
		zassert_false(result, "SPI transceive failed");
		return -1;
	}

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		to_display_format(buffer_tx, BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s", buffer_print_tx);
		LOG_ERR("                           vs: %s", buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	if (memcmp(buffer2_tx, buffer2_rx, BUF2_SIZE)) {
		to_display_format(buffer2_tx, BUF2_SIZE, buffer_print_tx2);
		to_display_format(buffer2_rx, BUF2_SIZE, buffer_print_rx2);
		LOG_ERR("Buffer 2 contents are different: %s", buffer_print_tx2);
		LOG_ERR("                             vs: %s", buffer_print_rx2);
		zassert_false(1, "Buffer 2 contents are different");
		return -1;
	}

	if (memcmp(large_buffer_tx, large_buffer_rx, BUF3_SIZE)) {
		zassert_false(1, "Buffer 3 contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}
#endif

static int spi_resource_lock_test(struct spi_dt_spec *lock_spec,
				  struct spi_dt_spec *try_spec)
{
	lock_spec->config.operation |= SPI_LOCK_ON;

	if (spi_complete_loop(lock_spec)) {
		return -1;
	}

	if (spi_release_dt(lock_spec)) {
		LOG_ERR("Deadlock now?");
		zassert_false(1, "SPI release failed");
		return -1;
	}

	if (spi_complete_loop(try_spec)) {
		return -1;
	}

	return 0;
}

/*
 *************************
 * Test suite definition *
 *************************
 */

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(spi_async_stack, STACK_SIZE);

#if defined(CONFIG_DMA) && defined(CONFIG_NOCACHE_MEMORY)
#define DMA_ENABLED_STR ", DMA enabled"
#elif defined(CONFIG_DMA)
#define DMA_ENABLED_STR ", DMA enabled (without CONFIG_NOCACHE_MEMORY)"
#else
#define DMA_ENABLED_STR
#endif /* CONFIG_DMA */

#ifdef CONFIG_SPI_LOOPBACK_16BITS_FRAMES
#define FRAME_SIZE_STR ", frame size = 16"
#else
#define FRAME_SIZE_STR ", frame size = 8"
#endif /* CONFIG_SPI_LOOPBACK_16BITS_FRAMES */

ZTEST(spi_loopback, test_spi_loopback)
{
#if (CONFIG_SPI_ASYNC)
	struct k_thread async_thread;
	k_tid_t async_thread_id;
#endif

	LOG_INF("SPI test on buffers TX/RX %p/%p" FRAME_SIZE_STR DMA_ENABLED_STR,
			buffer_tx,
			buffer_rx);

#if (CONFIG_SPI_ASYNC)
	async_thread_id = k_thread_create(&async_thread,
					  spi_async_stack, STACK_SIZE,
					  spi_async_call_cb,
					  &async_evt, &caller, NULL,
					  K_PRIO_COOP(7), 0, K_NO_WAIT);
#endif
	zassert_true(spi_is_ready_dt(&spi_slow), "Slow spi lookback device is not ready");

	LOG_INF("SPI test slow config");

	if (spi_complete_multiple(&spi_slow) ||
	    spi_complete_loop(&spi_slow) ||
	    spi_null_tx_buf(&spi_slow) ||
	    spi_rx_half_start(&spi_slow) ||
	    spi_rx_half_end(&spi_slow) ||
	    spi_rx_every_4(&spi_slow) ||
	    spi_rx_bigger_than_tx(&spi_slow) ||
	    spi_complete_large_transfers(&spi_slow)
#if (CONFIG_SPI_ASYNC)
	    || spi_async_call(&spi_slow)
#endif
	    ) {
		goto end;
	}

	zassert_true(spi_is_ready_dt(&spi_fast), "Fast spi lookback device is not ready");

	LOG_INF("SPI test fast config");

	if (spi_complete_multiple(&spi_fast) ||
	    spi_complete_loop(&spi_fast) ||
	    spi_null_tx_buf(&spi_fast) ||
	    spi_rx_half_start(&spi_fast) ||
	    spi_rx_half_end(&spi_fast) ||
	    spi_rx_every_4(&spi_fast) ||
	    spi_rx_bigger_than_tx(&spi_fast) ||
	    spi_complete_large_transfers(&spi_fast)
#if (CONFIG_SPI_ASYNC)
	    || spi_async_call(&spi_fast)
#endif
	    ) {
		goto end;
	}

	if (spi_resource_lock_test(&spi_slow, &spi_fast)) {
		goto end;
	}

	LOG_INF("All tx/rx passed");
end:
#if (CONFIG_SPI_ASYNC)
	k_thread_abort(async_thread_id);
#else
	;
#endif
}

static void *spi_loopback_setup(void)
{
	memset(buffer_tx, 0, sizeof(buffer_tx));
	memcpy(buffer_tx, tx_data, sizeof(tx_data));
	memset(buffer2_tx, 0, sizeof(buffer2_tx));
	memcpy(buffer2_tx, tx2_data, sizeof(tx2_data));
	memset(large_buffer_tx, 0, sizeof(large_buffer_tx));
	memcpy(large_buffer_tx, large_tx_data, sizeof(large_tx_data));
	return NULL;
}

ZTEST_SUITE(spi_loopback, NULL, spi_loopback_setup, NULL, NULL, NULL);
