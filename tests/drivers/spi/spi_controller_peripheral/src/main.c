/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/ztest.h>

#if CONFIG_TESTED_SPI_MODE == 0
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_LSB)
#elif CONFIG_TESTED_SPI_MODE == 1
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB | SPI_MODE_CPHA)
#elif CONFIG_TESTED_SPI_MODE == 2
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_LSB | SPI_MODE_CPOL)
#elif CONFIG_TESTED_SPI_MODE == 3
#define SPI_MODE (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB | SPI_MODE_CPHA \
				| SPI_MODE_CPOL)
#endif

#define SPIM_OP	 (SPI_OP_MODE_MASTER | SPI_MODE)
#define SPIS_OP	 (SPI_OP_MODE_SLAVE | SPI_MODE)

static struct spi_dt_spec spim = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPIM_OP, 0);
static const struct device *spis_dev = DEVICE_DT_GET(DT_NODELABEL(dut_spis));
static const struct spi_config spis_config = {
	.operation = SPIS_OP
};

static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &async_sig);

static struct k_poll_signal async_sig_spim = K_POLL_SIGNAL_INITIALIZER(async_sig_spim);
static struct k_poll_event async_evt_spim =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &async_sig_spim);

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t spim_buffer[32] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));
static uint8_t spis_buffer[32] MEMORY_SECTION(DT_NODELABEL(dut_spis));

struct test_data {
	struct k_work_delayable test_work;
	struct k_sem sem;
	int spim_alloc_idx;
	int spis_alloc_idx;
	struct spi_buf_set sets[4];
	struct spi_buf_set *mtx_set;
	struct spi_buf_set *mrx_set;
	struct spi_buf_set *stx_set;
	struct spi_buf_set *srx_set;
	struct spi_buf bufs[8];
	bool async;
};

static struct test_data tdata;

/* Allocate buffer from spim or spis space. */
static uint8_t *buf_alloc(size_t len, bool spim)
{
	int *idx = spim ? &tdata.spim_alloc_idx : &tdata.spis_alloc_idx;
	uint8_t *buf = spim ? spim_buffer : spis_buffer;
	size_t total = spim ? sizeof(spim_buffer) : sizeof(spis_buffer);
	uint8_t *rv;

	if (*idx + len > total) {
		zassert_false(true);

		return NULL;
	}

	rv = &buf[*idx];
	*idx += len;

	return rv;
}

static void work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct test_data *td = CONTAINER_OF(dwork, struct test_data, test_work);
	int rv;

	if (!td->async) {
		rv = spi_transceive_dt(&spim, td->mtx_set, td->mrx_set);
		if (rv == 0) {
			k_sem_give(&td->sem);
		}
	} else {
		rv = spi_transceive_signal(spim.bus, &spim.config, td->mtx_set, td->mrx_set,
				&async_sig_spim);
		zassert_equal(rv, 0);

		rv = k_poll(&async_evt_spim, 1, K_MSEC(200));
		zassert_false(rv, "one or more events are not ready");

		rv = async_evt_spim.signal->result;
		zassert_equal(rv, 0);

		/* Reinitializing for next call */
		async_evt_spim.signal->signaled = 0U;
		async_evt_spim.state = K_POLL_STATE_NOT_READY;

		k_sem_give(&td->sem);
	}
}

/** Copies data from buffers in the set to a single buffer which makes it easier
 * to compare transmitted and received data.
 *
 * @param buf Output buffer.
 * @param len Buffer length.
 * @param set Set of buffers.
 *
 * @return Number of bytes copied.
 */
static int cpy_data(uint8_t *buf, size_t len, struct spi_buf_set *set)
{
	int idx = 0;

	for (size_t i = 0; i < set->count; i++) {
		size_t l = set->buffers[i].len;

		if (len - idx >= l) {
			memcpy(&buf[idx], set->buffers[i].buf, l);
			idx += l;
		} else {
			return -1;
		}
	}

	return idx;
}

/** Compare two sets.
 *
 * @param tx_set TX set.
 * @param rx_set RX set.
 * @param same_size True if it is expected to have the same amount of data in both sets.
 *
 * @return 0 if data is the same and other value indicate that check failed.
 */
static int check_buffers(struct spi_buf_set *tx_set, struct spi_buf_set *rx_set, bool same_size)
{
	static uint8_t tx_data[256];
	static uint8_t rx_data[256];
	int rx_len;
	int tx_len;

	if (!tx_set || !rx_set) {
		return 0;
	}

	rx_len = cpy_data(rx_data, sizeof(rx_data), rx_set);
	tx_len = cpy_data(tx_data, sizeof(tx_data), tx_set);
	if (same_size && (rx_len != tx_len)) {
		return -1;
	}

	return memcmp(tx_data, rx_data, rx_len);
}

/** Calculate expected number of received bytes by the SPI peripheral.
 *
 * It is used to check if SPI API call for peripheral SPI device returns correct value.
 * @param tx_set TX set.
 * @param rx_set RX set.
 *
 * @return Expected amount of received bytes.
 */
static int peripheral_rx_len(struct spi_buf_set *tx_set, struct spi_buf_set *rx_set)
{
	size_t tx_len = 0;
	size_t rx_len = 0;

	if (!tx_set || !rx_set) {
		return 0;
	}

	for (size_t i = 0; i < tx_set->count; i++) {
		tx_len += tx_set->buffers[i].len;
	}

	for (size_t i = 0; i < rx_set->count; i++) {
		rx_len += rx_set->buffers[i].len;
	}

	return MIN(rx_len, tx_len);
}

/** Generic function which runs the test with sets prepared in the test data structure. */
static void run_test(bool m_same_size, bool s_same_size, bool async)
{
	int rv;
	int periph_rv;
	int srx_len;

	tdata.async = async;
	rv = k_work_schedule(&tdata.test_work, K_MSEC(10));
	zassert_equal(rv, 1);

	if (!async) {
		periph_rv = spi_transceive(spis_dev, &spis_config, tdata.stx_set, tdata.srx_set);
		if (periph_rv == -ENOTSUP) {
			ztest_test_skip();
		}
	} else {
		rv = spi_transceive_signal(spis_dev, &spis_config, tdata.stx_set, tdata.srx_set,
					   &async_sig);
		if (rv == -ENOTSUP) {
			ztest_test_skip();
		}
		zassert_equal(rv, 0);

		/* Transfer not finished yet */
		rv = k_sem_take(&tdata.sem, K_NO_WAIT);
		zassert_equal(rv, -EBUSY);

		rv = k_poll(&async_evt, 1, K_MSEC(200));
		zassert_false(rv, "one or more events are not ready");

		periph_rv = async_evt.signal->result;

		/* Reinitializing for next call */
		async_evt.signal->signaled = 0U;
		async_evt.state = K_POLL_STATE_NOT_READY;
	}

	rv = k_sem_take(&tdata.sem, K_MSEC(100));
	zassert_equal(rv, 0);

	srx_len = peripheral_rx_len(tdata.mtx_set, tdata.srx_set);

	zassert_equal(periph_rv, srx_len, "Got: %d but expected:%d", periph_rv, srx_len);

	rv = check_buffers(tdata.mtx_set, tdata.srx_set, m_same_size);
	zassert_equal(rv, 0);

	rv = check_buffers(tdata.stx_set, tdata.mrx_set, s_same_size);
	zassert_equal(rv, 0);
}

/** Basic test where SPI controller and SPI peripheral have RX and TX sets which contains only one
 *  same size buffer.
 */
static void test_basic(bool async)
{
	size_t len = 16;

	for (int i = 0; i < 4; i++) {
		tdata.bufs[i].buf = buf_alloc(len, i < 2);
		tdata.bufs[i].len = len;
		tdata.sets[i].buffers = &tdata.bufs[i];
		tdata.sets[i].count = 1;
	}

	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = &tdata.sets[1];
	tdata.stx_set = &tdata.sets[2];
	tdata.srx_set = &tdata.sets[3];

	run_test(true, true, async);
}

ZTEST(spi_controller_peripheral, test_basic)
{
	test_basic(false);
}

ZTEST(spi_controller_peripheral, test_basic_async)
{
	test_basic(true);
}

/** Basic test with zero length buffers.
 */
void test_basic_zero_len(bool async)
{
	size_t len = 8;

	/* SPIM */
	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.bufs[1].buf = buf_alloc(len, true);
	/* Intentionally len was set to 0 - second buffer "is empty". */
	tdata.bufs[1].len = 0;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 2;
	tdata.mtx_set = &tdata.sets[0];

	tdata.bufs[2].buf = buf_alloc(len, true);
	tdata.bufs[2].len = len;
	tdata.bufs[3].buf = buf_alloc(len, true);
	/* Intentionally len was set to 0 - second buffer "is empty". */
	tdata.bufs[3].len = 0;
	tdata.sets[1].buffers = &tdata.bufs[2];
	tdata.sets[1].count = 2;
	tdata.mrx_set = &tdata.sets[1];

	/* SPIS */
	tdata.bufs[4].buf = buf_alloc(len, false);
	tdata.bufs[4].len = len;
	tdata.sets[2].buffers = &tdata.bufs[4];
	tdata.sets[2].count = 1;
	tdata.stx_set = &tdata.sets[2];

	tdata.bufs[6].buf = buf_alloc(len, false);
	tdata.bufs[6].len = len;
	tdata.sets[3].buffers = &tdata.bufs[6];
	tdata.sets[3].count = 1;
	tdata.srx_set = &tdata.sets[3];

	run_test(true, true, async);
}

ZTEST(spi_controller_peripheral, test_basic_zero_len)
{
	test_basic_zero_len(false);
}

ZTEST(spi_controller_peripheral, test_basic_zero_len_async)
{
	test_basic_zero_len(true);
}

/** Setup a transfer where RX buffer on SPI controller and SPI peripheral are
 *  shorter than TX buffers. RX buffers shall contain beginning of TX data
 *  and last TX bytes that did not fit in the RX buffers shall be lost.
 */
static void test_short_rx(bool async)
{
	size_t len = 16;

	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.bufs[1].buf = buf_alloc(len, true);
	tdata.bufs[1].len = len - 3; /* RX buffer */
	tdata.bufs[2].buf = buf_alloc(len, false);
	tdata.bufs[2].len = len;
	tdata.bufs[3].buf = buf_alloc(len, false);
	tdata.bufs[3].len = len - 4; /* RX buffer */

	for (int i = 0; i < 4; i++) {
		tdata.sets[i].buffers = &tdata.bufs[i];
		tdata.sets[i].count = 1;
	}

	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = &tdata.sets[1];
	tdata.stx_set = &tdata.sets[2];
	tdata.srx_set = &tdata.sets[3];

	run_test(false, false, async);
}

ZTEST(spi_controller_peripheral, test_short_rx)
{
	test_short_rx(false);
}

ZTEST(spi_controller_peripheral, test_short_rx_async)
{
	test_short_rx(true);
}

/** Test where only master transmits. */
static void test_only_tx(bool async)
{
	size_t len = 16;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len, false);
	tdata.bufs[1].len = len;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 1;
	tdata.srx_set = &tdata.sets[1];
	tdata.stx_set = NULL;

	run_test(true, true, async);
}

ZTEST(spi_controller_peripheral, test_only_tx)
{
	test_only_tx(false);
}

ZTEST(spi_controller_peripheral, test_only_tx_async)
{
	test_only_tx(true);
}

/** Test where only SPI controller transmits and SPI peripheral receives in chunks. */
static void test_only_tx_in_chunks(bool async)
{
	size_t len1 = 7;
	size_t len2 = 8;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len1 + len2, true);
	tdata.bufs[0].len = len1 + len2;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len1, false);
	tdata.bufs[1].len = len1;
	tdata.bufs[2].buf = buf_alloc(len2, false);
	tdata.bufs[2].len = len2;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 2;
	tdata.srx_set = &tdata.sets[1];
	tdata.stx_set = NULL;

	run_test(true, true, async);
}

ZTEST(spi_controller_peripheral, test_only_tx_in_chunks)
{
	test_only_tx_in_chunks(false);
}

ZTEST(spi_controller_peripheral, test_only_tx_in_chunks_async)
{
	test_only_tx_in_chunks(true);
}

/** Test where only SPI peripheral transmits. */
static void test_only_rx(bool async)
{
	size_t len = 16;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len, true);
	tdata.bufs[0].len = len;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mrx_set = &tdata.sets[0];
	tdata.mtx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len, false);
	tdata.bufs[1].len = len;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 1;
	tdata.stx_set = &tdata.sets[1];
	tdata.srx_set = NULL;

	run_test(true, true, async);
}

ZTEST(spi_controller_peripheral, test_only_rx)
{
	test_only_rx(false);
}

ZTEST(spi_controller_peripheral, test_only_rx_async)
{
	test_only_rx(true);
}

/** Test where only SPI peripheral transmits in chunks. */
static void test_only_rx_in_chunks(bool async)
{
	size_t len1 = 7;
	size_t len2 = 9;

	/* MTX buffer */
	tdata.bufs[0].buf = buf_alloc(len1 + len2, true);
	tdata.bufs[0].len = len1 + len2;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mrx_set = &tdata.sets[0];
	tdata.mtx_set = NULL;

	/* STX buffer */
	tdata.bufs[1].buf = buf_alloc(len1, false);
	tdata.bufs[1].len = len1;
	tdata.bufs[2].buf = buf_alloc(len2, false);
	tdata.bufs[2].len = len2;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 2;
	tdata.stx_set = &tdata.sets[1];
	tdata.srx_set = NULL;

	run_test(true, true, async);
}

ZTEST(spi_controller_peripheral, test_only_rx_in_chunks)
{
	test_only_rx_in_chunks(false);
}

ZTEST(spi_controller_peripheral, test_only_rx_in_chunks_async)
{
	test_only_rx_in_chunks(true);
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);

	memset(&tdata, 0, sizeof(tdata));
	for (size_t i = 0; i < sizeof(spim_buffer); i++) {
		spim_buffer[i] = (uint8_t)i;
	}
	for (size_t i = 0; i < sizeof(spis_buffer); i++) {
		spis_buffer[i] = (uint8_t)(i + 0x80);
	}

	k_work_init_delayable(&tdata.test_work, work_handler);
	k_sem_init(&tdata.sem, 0, 1);
}

static void after(void *not_used)
{
	ARG_UNUSED(not_used);

	k_work_cancel_delayable(&tdata.test_work);
}

static void *suite_setup(void)
{
	return NULL;
}

ZTEST_SUITE(spi_controller_peripheral, NULL, suite_setup, before, after, NULL);
