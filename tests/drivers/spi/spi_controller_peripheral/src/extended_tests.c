/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define SPI_MODE      (SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_LSB)
#define SPIM_OP       (SPI_OP_MODE_MASTER | SPI_MODE)
#define SPIS_OP       (SPI_OP_MODE_SLAVE | SPI_MODE)
#define TEST_BUF_SIZE 4096

static struct spi_dt_spec spim = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPIM_OP);
static const struct device *spis_dev = DEVICE_DT_GET(DT_NODELABEL(dut_spis));
static const struct spi_config spis_config = {
	.operation = SPIS_OP,
};

static struct k_poll_signal async_sig_ext = K_POLL_SIGNAL_INITIALIZER(async_sig_ext);
static struct k_poll_event async_evt_ext =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &async_sig_ext);

static struct k_poll_signal async_sig_spim = K_POLL_SIGNAL_INITIALIZER(async_sig_spim);
static struct k_poll_event async_evt_spim =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &async_sig_spim);

static uint8_t spim_buffer[TEST_BUF_SIZE];
static uint8_t spis_buffer[TEST_BUF_SIZE];

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

static uint8_t *buf_alloc(size_t len, bool spim_space)
{
	int *idx = spim_space ? &tdata.spim_alloc_idx : &tdata.spis_alloc_idx;
	uint8_t *buf = spim_space ? spim_buffer : spis_buffer;

	zassert_true(*idx + len <= TEST_BUF_SIZE, "test buffer too small");

	uint8_t *rv = &buf[*idx];

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
		zassert_equal(rv, 0);
		k_sem_give(&td->sem);
		return;
	}

	rv = spi_transceive_signal(spim.bus, &spim.config, td->mtx_set, td->mrx_set,
				   &async_sig_spim);
	zassert_equal(rv, 0);

	rv = k_poll(&async_evt_spim, 1, K_MSEC(500));
	zassert_equal(rv, 0);
	zassert_equal(async_evt_spim.signal->result, 0);

	async_evt_spim.signal->signaled = 0U;
	async_evt_spim.state = K_POLL_STATE_NOT_READY;
	k_sem_give(&td->sem);
}

static size_t copy_data(uint8_t *buf, size_t len, struct spi_buf_set *set)
{
	size_t idx = 0;

	for (size_t i = 0; i < set->count; i++) {
		size_t l = set->buffers[i].len;

		zassert_true(len - idx >= l, "copy buffer too small");
		memcpy(&buf[idx], set->buffers[i].buf, l);
		idx += l;
	}

	return idx;
}

static void check_buffers(struct spi_buf_set *tx_set, struct spi_buf_set *rx_set)
{
	static uint8_t tx_data[TEST_BUF_SIZE];
	static uint8_t rx_data[TEST_BUF_SIZE];
	size_t tx_len;
	size_t rx_len;

	tx_len = copy_data(tx_data, sizeof(tx_data), tx_set);
	rx_len = copy_data(rx_data, sizeof(rx_data), rx_set);

	zassert_mem_equal(tx_data, rx_data, MIN(tx_len, rx_len));
}

static size_t total_len(struct spi_buf_set *set)
{
	size_t len = 0;

	for (size_t i = 0; i < set->count; i++) {
		len += set->buffers[i].len;
	}

	return len;
}

static void run_test_ext(bool async)
{
	int rv;
	int periph_rv;
	size_t expected_periph_rv;

	async_evt_ext.signal->result = -1;
	async_evt_ext.signal->signaled = 0U;
	async_evt_ext.state = K_POLL_STATE_NOT_READY;

	tdata.async = async;
	rv = k_work_schedule(&tdata.test_work, K_MSEC(10));
	zassert_equal(rv, 1);

	if (!async) {
		periph_rv = spi_transceive(spis_dev, &spis_config, tdata.stx_set, tdata.srx_set);
	} else {
		rv = spi_transceive_signal(spis_dev, &spis_config, tdata.stx_set, tdata.srx_set,
					   &async_sig_ext);
		zassert_equal(rv, 0);

		rv = k_poll(&async_evt_ext, 1, K_MSEC(500));
		zassert_equal(rv, 0);
		periph_rv = async_evt_ext.signal->result;
	}

	rv = k_sem_take(&tdata.sem, K_MSEC(500));
	zassert_equal(rv, 0);

	expected_periph_rv = MIN(total_len(tdata.mtx_set), total_len(tdata.srx_set));
	zassert_equal(periph_rv, expected_periph_rv);

	check_buffers(tdata.mtx_set, tdata.srx_set);
	check_buffers(tdata.stx_set, tdata.mrx_set);
}

static void setup_slave_unequal_single_buffers(size_t master_len, size_t slave_tx_len,
					       size_t slave_rx_len)
{
	tdata.bufs[0].buf = buf_alloc(master_len, true);
	tdata.bufs[0].len = master_len;
	tdata.sets[0].buffers = &tdata.bufs[0];
	tdata.sets[0].count = 1;
	tdata.mtx_set = &tdata.sets[0];

	tdata.bufs[1].buf = buf_alloc(master_len, true);
	tdata.bufs[1].len = master_len;
	tdata.sets[1].buffers = &tdata.bufs[1];
	tdata.sets[1].count = 1;
	tdata.mrx_set = &tdata.sets[1];

	tdata.bufs[2].buf = buf_alloc(slave_tx_len, false);
	tdata.bufs[2].len = slave_tx_len;
	tdata.sets[2].buffers = &tdata.bufs[2];
	tdata.sets[2].count = 1;
	tdata.stx_set = &tdata.sets[2];

	tdata.bufs[3].buf = buf_alloc(slave_rx_len, false);
	tdata.bufs[3].len = slave_rx_len;
	tdata.sets[3].buffers = &tdata.bufs[3];
	tdata.sets[3].count = 1;
	tdata.srx_set = &tdata.sets[3];
}

static void setup_slave_tx_shorter_than_rx_chunked(void)
{
	setup_slave_unequal_single_buffers(16, 8, 16);

	tdata.bufs[2].buf = &spis_buffer[0];
	tdata.bufs[2].len = 5;
	tdata.bufs[3].buf = &spis_buffer[5];
	tdata.bufs[3].len = 3;
	tdata.sets[2].buffers = &tdata.bufs[2];
	tdata.sets[2].count = 2;

	tdata.bufs[4].buf = &spis_buffer[8];
	tdata.bufs[4].len = 16;
	tdata.sets[3].buffers = &tdata.bufs[4];
	tdata.sets[3].count = 1;
	tdata.srx_set = &tdata.sets[3];
}

static void setup_slave_tx_shorter_than_rx_asymmetric_chunks(void)
{
	setup_slave_unequal_single_buffers(16, 12, 16);

	tdata.bufs[2].buf = &spis_buffer[0];
	tdata.bufs[2].len = 5;
	tdata.bufs[3].buf = &spis_buffer[5];
	tdata.bufs[3].len = 7;
	tdata.sets[2].buffers = &tdata.bufs[2];
	tdata.sets[2].count = 2;

	tdata.bufs[4].buf = &spis_buffer[12];
	tdata.bufs[4].len = 9;
	tdata.bufs[5].buf = &spis_buffer[21];
	tdata.bufs[5].len = 7;
	tdata.sets[3].buffers = &tdata.bufs[4];
	tdata.sets[3].count = 2;
	tdata.srx_set = &tdata.sets[3];
}

static void prepare(void *not_used)
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

static void test_slave_tx_shorter_than_rx(bool async)
{
	setup_slave_unequal_single_buffers(16, 8, 16);
	run_test_ext(async);
}

ZTEST(spi_controller_peripheral_ext, test_slave_tx_shorter_than_rx)
{
	test_slave_tx_shorter_than_rx(false);
}

ZTEST(spi_controller_peripheral_ext, test_slave_tx_shorter_than_rx_async)
{
	test_slave_tx_shorter_than_rx(true);
}

static void test_slave_rx_shorter_than_tx(bool async)
{
	setup_slave_unequal_single_buffers(16, 16, 8);
	run_test_ext(async);
}

ZTEST(spi_controller_peripheral_ext, test_slave_rx_shorter_than_tx)
{
	test_slave_rx_shorter_than_tx(false);
}

ZTEST(spi_controller_peripheral_ext, test_slave_rx_shorter_than_tx_async)
{
	test_slave_rx_shorter_than_tx(true);
}

static void test_slave_tx_shorter_than_rx_chunked(bool async)
{
	setup_slave_tx_shorter_than_rx_chunked();
	run_test_ext(async);
}

ZTEST(spi_controller_peripheral_ext, test_slave_tx_shorter_than_rx_chunked)
{
	test_slave_tx_shorter_than_rx_chunked(false);
}

ZTEST(spi_controller_peripheral_ext, test_slave_tx_shorter_than_rx_chunked_async)
{
	test_slave_tx_shorter_than_rx_chunked(true);
}

static void test_slave_tx_shorter_than_rx_asymmetric_chunks(bool async)
{
	setup_slave_tx_shorter_than_rx_asymmetric_chunks();
	run_test_ext(async);
}

ZTEST(spi_controller_peripheral_ext, test_slave_tx_shorter_than_rx_asymmetric_chunks)
{
	test_slave_tx_shorter_than_rx_asymmetric_chunks(false);
}

ZTEST(spi_controller_peripheral_ext, test_slave_tx_shorter_than_rx_asymmetric_chunks_async)
{
	test_slave_tx_shorter_than_rx_asymmetric_chunks(true);
}

ZTEST_SUITE(spi_controller_peripheral_ext, NULL, NULL, prepare, NULL, NULL);
