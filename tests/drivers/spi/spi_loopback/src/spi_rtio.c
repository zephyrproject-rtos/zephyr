/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_rtio_loopback);

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/ztest.h>

#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/spi.h>
#include "spi_rtio.h"

#define SPI_FAST_DEV	DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback_fast)
#define SPI_SLOW_DEV	DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback_slow)

#if CONFIG_SPI_LOOPBACK_MODE_LOOP
#define MODE_LOOP SPI_MODE_LOOP
#else
#define MODE_LOOP 0
#endif

#define SPI_OP SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL | MODE_LOOP | \
	       SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE

static SPI_DT_IODEV_DEFINE(spi_fast, SPI_FAST_DEV, SPI_OP);
static SPI_DT_IODEV_DEFINE(spi_slow, SPI_FAST_DEV, SPI_OP);

RTIO_DEFINE(r, 8, 8);

/* to run this test, connect the SDO pin to the SDI of the SPI */

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define BUF_SIZE 17
#define BUF2_SIZE 36

#if CONFIG_NOCACHE_MEMORY
static const char tx_data[BUF_SIZE] = "0123456789abcdef\0";
static __aligned(32) char buffer_tx[BUF_SIZE] __used __attribute__((__section__(".nocache")));
static __aligned(32) char buffer_rx[BUF_SIZE] __used __attribute__((__section__(".nocache")));
static const char tx2_data[BUF2_SIZE] = "Thequickbrownfoxjumpsoverthelazydog\0";
static __aligned(32) char buffer2_tx[BUF2_SIZE] __used __attribute__((__section__(".nocache")));
static __aligned(32) char buffer2_rx[BUF2_SIZE] __used __attribute__((__section__(".nocache")));
#else
/* this src memory shall be in RAM to support using as a DMA source pointer.*/
static uint8_t buffer_tx[] = "0123456789abcdef\0";
static uint8_t buffer_rx[BUF_SIZE] = {};

static uint8_t buffer2_tx[] = "Thequickbrownfoxjumpsoverthelazydog\0";
static uint8_t buffer2_rx[BUF2_SIZE] = {};
#endif

/*
 * We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
static uint8_t buffer_print_tx[BUF_SIZE * 5 + 1];
static uint8_t buffer_print_rx[BUF_SIZE * 5 + 1];

static uint8_t buffer_print_tx2[BUF2_SIZE * 5 + 1];
static uint8_t buffer_print_rx2[BUF2_SIZE * 5 + 1];



static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

/* test transferring different buffers on the same dma channels */
static int spi_complete_multiple(struct rtio_iodev *spi_iodev)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_transceive(sqe, spi_iodev, RTIO_PRIO_NORM,
				buffer_tx, buffer_rx, BUF_SIZE, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;
	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_transceive(sqe, spi_iodev, RTIO_PRIO_NORM,
				 buffer2_tx, buffer2_rx, BUF2_SIZE, NULL);

	LOG_INF("Start complete multiple");
	rtio_submit(&r, 1);
	cqe = rtio_cqe_consume(&r);
	ret = cqe->result;
	rtio_cqe_release(&r, cqe);

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return ret;
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

	LOG_INF("Passed");

	return 0;
}

static int spi_complete_loop(struct rtio_iodev *spi_iodev)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_transceive(sqe, spi_iodev, RTIO_PRIO_NORM,
				 buffer_tx,  buffer_rx, BUF_SIZE, NULL);

	LOG_INF("Start complete loop");

	rtio_submit(&r, 1);
	cqe = rtio_cqe_consume(&r);
	ret = cqe->result;
	rtio_cqe_release(&r, cqe);

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return ret;
	}

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

static int spi_null_tx_buf(struct rtio_iodev *spi_iodev)
{
	static const uint8_t EXPECTED_NOP_RETURN_BUF[BUF_SIZE] = { 0 };

	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	(void)memset(buffer_rx, 0x77, BUF_SIZE);

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_read(sqe, spi_iodev, RTIO_PRIO_NORM,
				 buffer_rx, BUF_SIZE,
				 NULL);

	LOG_INF("Start null tx");

	rtio_submit(&r, 1);
	cqe = rtio_cqe_consume(&r);
	ret = cqe->result;
	rtio_cqe_release(&r, cqe);

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return ret;
	}


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

static int spi_rx_half_start(struct rtio_iodev *spi_iodev)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_transceive(sqe, spi_iodev, RTIO_PRIO_NORM,
				 buffer_tx, buffer_rx, 8, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;
	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_write(sqe, spi_iodev, RTIO_PRIO_NORM,
				 &buffer_tx[8], BUF_SIZE-8, NULL);


	LOG_INF("Start half start");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	rtio_submit(&r, 1);
	cqe = rtio_cqe_consume(&r);
	ret = cqe->result;
	rtio_cqe_release(&r, cqe);

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

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

static int spi_rx_half_end(struct rtio_iodev *spi_iodev)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		LOG_INF("Skip half end");
		return 0;
	}

	if (IS_ENABLED(CONFIG_SPI_MAX32_DMA)) {
		LOG_INF("Skip half end");
		return 0;
	}

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_write(sqe, spi_iodev, RTIO_PRIO_NORM,
				 buffer_tx, 8, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_transceive(sqe, spi_iodev, RTIO_PRIO_NORM,
				 &buffer_tx[8],
				 buffer_rx, 8,
				 NULL);

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_write(sqe, spi_iodev, RTIO_PRIO_NORM,
		&buffer_tx[16], BUF_SIZE-16, NULL);

	LOG_INF("Start half end");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	rtio_submit(&r, 1);
	cqe = rtio_cqe_consume(&r);
	ret = cqe->result;
	rtio_cqe_release(&r, cqe);

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

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

static int spi_rx_every_4(struct rtio_iodev *spi_iodev)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		LOG_INF("Skip every 4");
		return 0;
	}

	if (IS_ENABLED(CONFIG_DSPI_MCUX_EDMA)) {
		LOG_INF("Skip every 4");
		return 0;
	}

	if (IS_ENABLED(CONFIG_SPI_MAX32_DMA)) {
		LOG_INF("Skip every 4");
		return 0;
	}

	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_write(sqe, spi_iodev, RTIO_PRIO_NORM,
				 buffer_tx, 4,
				 NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;
	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_transceive(sqe, spi_iodev, RTIO_PRIO_NORM,
				 &buffer_tx[4], buffer_rx, 4, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;
	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_write(sqe, spi_iodev, RTIO_PRIO_NORM,
				 &buffer_tx[8], (BUF_SIZE - 8),
				 NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;
	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_transceive(sqe, spi_iodev, RTIO_PRIO_NORM,
				 &buffer_tx[12], &buffer_rx[4], 4, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;
	sqe = rtio_sqe_acquire(&r);
	rtio_sqe_prep_write(sqe, spi_iodev, RTIO_PRIO_NORM,
				 &buffer_tx[16], BUF_SIZE-16, NULL);

	LOG_INF("Start every 4");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	rtio_submit(&r, 1);
	cqe = rtio_cqe_consume(&r);
	ret = cqe->result;
	rtio_cqe_release(&r, cqe);

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

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

/* Helper to submit, consume, and release all CQEs for a transaction safely */
static int spi_rtio_wait_and_drain(struct rtio *r_ctx, int count)
{
	struct rtio_cqe *cqe;
	int first_err = 0;

	int flush_err;

	rtio_submit(r_ctx, count);
	while (count > 0) {
		cqe = rtio_cqe_consume(r_ctx);
		count--;
		if (cqe != NULL) {
			if (cqe->result < 0 && first_err == 0) {
				first_err = cqe->result;
			}
			rtio_cqe_release(r_ctx, cqe);
		}
	}
	flush_err = rtio_flush_completion_queue(r_ctx);
	if (first_err == 0) {
		first_err = flush_err;
	}
	return first_err;
}

/* Test asymmetric SPI transceive with NULL (discard) buffers under RTIO */
static int spi_asymmetric_null_buffers(struct rtio_iodev *spi_iodev)
{
	struct rtio_sqe *last_sqe = NULL;
	int ret;
	int err;

	LOG_INF("Start asymmetric null buffers test");

	/* Test 1: Subsystem spi_rtio_copy decomposition of asymmetric buffers with small discard */
	struct spi_buf tx_buf_arr1[] = {
		{ .buf = buffer_tx, .len = 4 },
		{ .buf = buffer_tx + 4, .len = 4 },
	};
	struct spi_buf rx_buf_arr1[] = {
		{ .buf = NULL, .len = 4 },
		{ .buf = buffer_rx, .len = 4 },
	};
	struct spi_buf_set tx_set1 = { .buffers = tx_buf_arr1, .count = ARRAY_SIZE(tx_buf_arr1) };
	struct spi_buf_set rx_set1 = { .buffers = rx_buf_arr1, .count = ARRAY_SIZE(rx_buf_arr1) };

	(void)memset(buffer_rx, 0xaa, BUF_SIZE);

	ret = spi_rtio_copy(&r, spi_iodev, &tx_set1, &rx_set1, false, &last_sqe);
	zassert_true(ret > 0, "spi_rtio_copy failed with code %d", ret);

	err = spi_rtio_wait_and_drain(&r, ret);
	zassert_ok(err, "spi_rtio_copy Test 1 failed with error %d", err);
	zassert_mem_equal(buffer_rx, buffer_tx + 4, 4,
			  "Discard failed: buffer_rx did not match expected data");

	/* Test 2: Subsystem spi_rtio_copy multi-chunk scratch buffer discard */
	static const uint8_t expected_nop[4] = { 0 };
	struct spi_buf tx_buf_arr2[] = {
		{ .buf = buffer_tx, .len = 3 },
	};
	struct spi_buf rx_buf_arr2[] = {
		{ .buf = NULL, .len = 64 },
		{ .buf = buffer_rx, .len = 4 },
	};
	struct spi_buf_set tx_set2 = { .buffers = tx_buf_arr2, .count = ARRAY_SIZE(tx_buf_arr2) };
	struct spi_buf_set rx_set2 = { .buffers = rx_buf_arr2, .count = ARRAY_SIZE(rx_buf_arr2) };

	(void)memset(buffer_rx, 0xaa, BUF_SIZE);

	ret = spi_rtio_copy(&r, spi_iodev, &tx_set2, &rx_set2, false, &last_sqe);
	zassert_true(ret > 0, "spi_rtio_copy multi-chunk failed with code %d", ret);

	err = spi_rtio_wait_and_drain(&r, ret);
	zassert_ok(err, "spi_rtio_copy multi-chunk failed with error %d", err);
	zassert_mem_equal(buffer_rx, expected_nop, 4,
			  "Multi-chunk discard failed: expected NOP received data");
	zassert_equal((uint8_t)buffer_rx[4], 0xaa,
		      "Multi-chunk discard overflowed receive buffer");

	/* Test 3: Multi-chunk scratch buffer discard with payload data verification */
	struct spi_buf tx_buf_arr3[] = {
		{ .buf = NULL, .len = 64 },
		{ .buf = buffer_tx, .len = 4 },
	};
	struct spi_buf rx_buf_arr3[] = {
		{ .buf = NULL, .len = 64 },
		{ .buf = buffer_rx, .len = 4 },
	};
	struct spi_buf_set tx_set3 = { .buffers = tx_buf_arr3, .count = ARRAY_SIZE(tx_buf_arr3) };
	struct spi_buf_set rx_set3 = { .buffers = rx_buf_arr3, .count = ARRAY_SIZE(rx_buf_arr3) };

	(void)memset(buffer_rx, 0xaa, BUF_SIZE);

	ret = spi_rtio_copy(&r, spi_iodev, &tx_set3, &rx_set3, false, &last_sqe);
	zassert_true(ret > 0, "spi_rtio_copy Test 3 failed with code %d", ret);

	err = spi_rtio_wait_and_drain(&r, ret);
	zassert_ok(err, "spi_rtio_copy Test 3 failed with error %d", err);
	zassert_mem_equal(buffer_rx, buffer_tx, 4,
			  "Multi-chunk discard failed: payload mismatch after 64 discard bytes");

	/* Test 4: Standalone discard read with NO TX buffer (tx_bufs == NULL) */
	struct spi_buf rx_buf_arr4[] = {
		{ .buf = NULL, .len = 64 },
	};
	struct spi_buf_set rx_set4 = { .buffers = rx_buf_arr4, .count = ARRAY_SIZE(rx_buf_arr4) };

	ret = spi_rtio_copy(&r, spi_iodev, NULL, &rx_set4, false, &last_sqe);
	zassert_true(ret > 0, "spi_rtio_copy read-only discard failed with code %d", ret);

	err = spi_rtio_wait_and_drain(&r, ret);
	zassert_ok(err, "spi_rtio_copy read-only discard failed with error %d", err);
	LOG_INF("Passed asymmetric null buffers test");
	return 0;
}

ZTEST(spi_loopback_rtio, test_spi_loopback_rtio)
{

	LOG_INF("SPI test on buffers TX/RX %p/%p", buffer_tx, buffer_rx);

	zassert_true(spi_is_ready_iodev(&spi_slow), "Slow spi lookback device is not ready");

	LOG_INF("SPI test slow config");

	if (spi_complete_multiple(&spi_slow) ||
	    spi_complete_loop(&spi_slow) ||
	    spi_null_tx_buf(&spi_slow) ||
	    spi_rx_half_start(&spi_slow) ||
	    spi_rx_half_end(&spi_slow) ||
	    spi_rx_every_4(&spi_slow) ||
	    spi_asymmetric_null_buffers(&spi_slow)
	    ) {
		goto end;
	}

	zassert_true(spi_is_ready_iodev(&spi_fast), "Fast spi lookback device is not ready");

	LOG_INF("SPI test fast config");

	if (spi_complete_multiple(&spi_fast) ||
	    spi_complete_loop(&spi_fast) ||
	    spi_null_tx_buf(&spi_fast) ||
	    spi_rx_half_start(&spi_fast) ||
	    spi_rx_half_end(&spi_fast) ||
	    spi_rx_every_4(&spi_fast) ||
	    spi_asymmetric_null_buffers(&spi_fast)
	    ) {
		goto end;
	}

	LOG_INF("All tx/rx passed");
end:
}

static void *spi_loopback_setup(void)
{
#if CONFIG_NOCACHE_MEMORY
	memset(buffer_tx, 0, sizeof(buffer_tx));
	memcpy(buffer_tx, tx_data, sizeof(tx_data));
	memset(buffer2_tx, 0, sizeof(buffer2_tx));
	memcpy(buffer2_tx, tx2_data, sizeof(tx2_data));
#endif
	return NULL;
}

static void spi_loopback_rtio_before(void *fixture)
{
	(void)rtio_flush_completion_queue(&r);
}

ZTEST_SUITE(spi_loopback_rtio, NULL, spi_loopback_setup, spi_loopback_rtio_before, NULL, NULL);
