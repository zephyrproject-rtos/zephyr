/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SPIM, driven through the SPI RTIO API, is connected to a SPIS which is
 * driven through the standard (asynchronous) SPI API. Every transfer is a
 * single full-duplex transceive, verified on both sides: the data received
 * by each side must match the data transmitted by the other and TX buffers
 * must not be modified.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/random/random.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#include <string.h>

#define SPIM_NODE DT_NODELABEL(dut_spim)
#define SPIS_NODE DT_NODELABEL(dut_spis)

#if !DT_NODE_EXISTS(SPIM_NODE)
#error "dut_spim node not defined"
#endif

#if !DT_NODE_EXISTS(SPIS_NODE)
#error "dut_spis node not defined"
#endif

#if CONFIG_TEST_SPI_MODE == 0
#define TEST_SPI_MODE (0)
#elif CONFIG_TEST_SPI_MODE == 1
#define TEST_SPI_MODE (SPI_MODE_CPHA)
#elif CONFIG_TEST_SPI_MODE == 2
#define TEST_SPI_MODE (SPI_MODE_CPOL)
#elif CONFIG_TEST_SPI_MODE == 3
#define TEST_SPI_MODE (SPI_MODE_CPOL | SPI_MODE_CPHA)
#endif

#define TEST_SPI_OP (SPI_WORD_SET(8) | SPI_TRANSFER_MSB | TEST_SPI_MODE)
#define SPIM_OP     (SPI_OP_MODE_CONTROLLER | TEST_SPI_OP)
#define SPIS_OP     (SPI_OP_MODE_PERIPHERAL | TEST_SPI_OP)

#define MAX_TEST_BUF_SIZE 2048
#define TEST_TIMEOUT      K_MSEC(500)

/*
 * The SPIS acquires its buffers asynchronously after being armed,
 * give it some time before the SPIM starts clocking.
 */
#define TEST_SPIS_ARM_DELAY K_MSEC(10)

/* One SQE per transfer, generating one CQE */
RTIO_DEFINE(spim_rtio, 1, 1);

SPI_DT_IODEV_DEFINE(spim_iodev, SPIM_NODE, SPIM_OP);

static const struct spi_dt_spec spis_spec = SPI_DT_SPEC_GET(SPIS_NODE, SPIS_OP);

static uint8_t spim_tx_buffer[MAX_TEST_BUF_SIZE];
static uint8_t spim_rx_buffer[MAX_TEST_BUF_SIZE];
static uint8_t spis_tx_buffer[MAX_TEST_BUF_SIZE];
static uint8_t spis_rx_buffer[MAX_TEST_BUF_SIZE];

/* Reference TX data, also the expected content of the RX buffer on the other side */
static uint8_t spim_tx_reference_pattern[MAX_TEST_BUF_SIZE];
static uint8_t spis_tx_reference_pattern[MAX_TEST_BUF_SIZE];

static K_SEM_DEFINE(spis_done, 0, 1);
static int spis_result;
static bool spis_armed;

static void fill_buffer(uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = sys_rand8_get();
	}
}

static void prepare_buffers(size_t len)
{
	/* Prepare reference patterns (random) */
	fill_buffer(spim_tx_reference_pattern, len);
	fill_buffer(spis_tx_reference_pattern, len);

	/* Copy reference patterns to the TX buffers */
	memcpy(spim_tx_buffer, spim_tx_reference_pattern, len);
	memcpy(spis_tx_buffer, spis_tx_reference_pattern, len);

	memset(spim_rx_buffer, 0xFF, len);
	memset(spis_rx_buffer, 0xFF, len);
}

static void check_buffer(const char *buffer_name, const uint8_t *actual_buffer,
			 const uint8_t *expected_buffer, size_t len)
{
	zassert_mem_equal(actual_buffer, expected_buffer, len, "%s content mismatch", buffer_name);
}

static void validate_buffers(size_t len)
{
	check_buffer("spim_tx_buffer", spim_tx_buffer, spim_tx_reference_pattern, len);
	check_buffer("spis_tx_buffer", spis_tx_buffer, spis_tx_reference_pattern, len);
	check_buffer("spim_rx_buffer", spim_rx_buffer, spis_tx_reference_pattern, len);
	check_buffer("spis_rx_buffer", spis_rx_buffer, spim_tx_reference_pattern, len);
}

static void spis_callback(const struct device *dev, int result, void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	spis_result = result;
	k_sem_give(&spis_done);
}

static void setup_spis_transfer(size_t len)
{
	static struct spi_buf tx_buf = {.buf = spis_tx_buffer};
	static struct spi_buf rx_buf = {.buf = spis_rx_buffer};
	static const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	static const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	tx_buf.len = len;
	rx_buf.len = len;

	ret = spi_transceive_cb(spis_spec.bus, &spis_spec.config, &tx_set, &rx_set, spis_callback,
				NULL);
	zassert_ok(ret, "SPIS transceive failed (%d)", ret);
	spis_armed = true;

	k_sleep(TEST_SPIS_ARM_DELAY);

	zassert_equal(k_sem_take(&spis_done, K_NO_WAIT), -EBUSY,
		      "SPIS transfer completed before SPIM started");
}

/*
 * Wait for the SPIS transfer to complete,
 * returns the number of bytes received
 */
static int spis_wait_for_transfer_completion(void)
{
	zassert_ok(k_sem_take(&spis_done, TEST_TIMEOUT), "SPIS transfer timed out");
	spis_armed = false;

	return spis_result;
}

/*
 * Submit the prepared SQE
 * and wait for its completion
 */
static void spim_submit_and_check(void)
{
	struct rtio_cqe cqe;
	int ret;

	ret = rtio_submit(&spim_rtio, 0);
	zassert_ok(ret, "RTIO submit failed (%d)", ret);

	ret = rtio_cqe_copy_out(&spim_rtio, &cqe, 1, TEST_TIMEOUT);
	zassert_equal(ret, 1, "SPIM transfer timed out");
	zassert_ok(cqe.result, "SPIM transfer failed (%d)", cqe.result);
}

/*
 * SPIM writes and reads len bytes
 * in a single RTIO transceive
 */
static void spim_rtio_write_read(size_t len)
{
	struct rtio_sqe *sqe;
	int ret;

	if (len > MAX_TEST_BUF_SIZE) {
		TC_PRINT("Skipping %zu byte transfer, limit is %d bytes\n", len, MAX_TEST_BUF_SIZE);
		return;
	}

	prepare_buffers(len);
	setup_spis_transfer(len);

	sqe = rtio_sqe_acquire(&spim_rtio);
	zassert_not_null(sqe, "SQE pool exhausted");
	rtio_sqe_prep_transceive(sqe, &spim_iodev, RTIO_PRIO_NORM, spim_tx_buffer, spim_rx_buffer,
				 len, NULL);
	spim_submit_and_check();

	ret = spis_wait_for_transfer_completion();
	zassert_equal(ret, len, "SPIS received %d bytes, expected %zu", ret, len);
	validate_buffers(len);
}

ZTEST(spim_rtio_loopback, test_spim_rtio_write_read)
{
	spim_rtio_write_read(1);
	spim_rtio_write_read(16);
	spim_rtio_write_read(64);
	spim_rtio_write_read(256);
	spim_rtio_write_read(512);
	spim_rtio_write_read(1024);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&spis_done);
	spis_armed = false;
}

static void after(void *fixture)
{
	struct rtio_cqe cqe;
	struct rtio_sqe *sqe;

	ARG_UNUSED(fixture);

	rtio_sqe_drop_all(&spim_rtio);

	/* Complete a SPIS transfer (left pending by a failed test) */
	if (spis_armed) {
		sqe = rtio_sqe_acquire(&spim_rtio);
		if (sqe != NULL) {
			rtio_sqe_prep_transceive(sqe, &spim_iodev, RTIO_PRIO_NORM, spim_tx_buffer,
						 spim_rx_buffer, MAX_TEST_BUF_SIZE, NULL);
			(void)rtio_submit(&spim_rtio, 0);
		}
		(void)k_sem_take(&spis_done, TEST_TIMEOUT);
		spis_armed = false;
	}

	/* Drop any late completions */
	while (rtio_cqe_copy_out(&spim_rtio, &cqe, 1, K_MSEC(10)) > 0) {
	}
}

ZTEST_SUITE(spim_rtio_loopback, NULL, NULL, before, after, NULL);
