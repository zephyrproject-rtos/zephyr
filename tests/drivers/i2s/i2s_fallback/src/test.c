/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2s/fake.h>
#include <zephyr/drivers/i2s/fallback.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_BUF_SIZE_MIN 64
#define TEST_BUF_SIZE_MID 128
#define TEST_BUF_SIZE_MAX 256
#define TEST_BUF_COUNT 6
#define TEST_SQE_COUNT TEST_BUF_COUNT
#define TEST_CQE_COUNT TEST_BUF_COUNT
#define TEST_WORD_SIZE 16
#define TEST_CHANNELS 2
#define TEST_FORMAT I2S_FMT_DATA_FORMAT_I2S
#define TEST_OPTIONS (I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER)
#define TEST_FRAME_CLK_FREQ 44100
#define TEST_PRELOAD 3
#define TEST_TIMEOUT K_MSEC(CONFIG_I2S_FALLBACK_TIMEOUT_MS)

DEFINE_FFF_GLOBALS;

static const struct device *test_dev = DEVICE_DT_GET(DT_NODELABEL(i2s));

static uint8_t test_rx_bufs[TEST_BUF_COUNT][TEST_BUF_SIZE_MAX];
static uint8_t test_tx_bufs[TEST_BUF_COUNT][TEST_BUF_SIZE_MAX];

static const size_t test_buf_sizes[TEST_BUF_COUNT] = {
	TEST_BUF_SIZE_MIN,
	TEST_BUF_SIZE_MID,
	TEST_BUF_SIZE_MAX,
	TEST_BUF_SIZE_MIN,
	TEST_BUF_SIZE_MID,
	TEST_BUF_SIZE_MAX,
};

static uint8_t test_front_buf_it;
static uint8_t test_front_buf_val;

static void push_front_bufs(uint8_t **rx_buf, uint8_t **tx_buf, size_t *buf_len)
{
	if (rx_buf != NULL) {
		*rx_buf = test_rx_bufs[test_front_buf_it];
	}

	if (tx_buf != NULL) {
		*tx_buf = test_tx_bufs[test_front_buf_it];
	}

	*buf_len = test_buf_sizes[test_front_buf_it];

	/* Fill TX buffer with pseudo random sequence */
	for (size_t i = 0; i < *buf_len; i++) {
		(*tx_buf)[i] = test_front_buf_val;
		test_front_buf_val++;
	}

	/* Add offset to pseudo random sequence */
	test_front_buf_val += test_front_buf_it;

	test_front_buf_it++;
	test_front_buf_it = test_front_buf_it == TEST_BUF_COUNT ? 0 : test_front_buf_it;
}

static uint8_t test_back_buf_it;
static uint8_t test_back_buf_val;

static void pop_back_bufs(const uint8_t *rx_buf, const uint8_t *tx_buf, size_t buf_len)
{
	zassert_equal(buf_len, test_buf_sizes[test_back_buf_it]);

	test_back_buf_val += test_back_buf_it;

	test_back_buf_it++;
	test_back_buf_it = test_back_buf_it == TEST_BUF_COUNT ? 0 : test_back_buf_it;
}

I2S_DT_FALLBACK_IODEV_DEFINE(
	test_iodev,
	DT_NODELABEL(i2s),
	TEST_WORD_SIZE,
	TEST_CHANNELS,
	TEST_FORMAT,
	TEST_OPTIONS,
	TEST_FRAME_CLK_FREQ,
	TEST_PRELOAD,
	TEST_BUF_SIZE_MAX,
	4
);

RTIO_DEFINE(test_rtio, TEST_SQE_COUNT, TEST_CQE_COUNT);

static struct i2s_config test_rx_cfg;
static struct i2s_config test_tx_cfg;

static void submit_tx_submission(void)
{
	struct rtio_sqe *sqe;
	uint8_t *tx_buf;
	size_t buf_len;

	sqe = rtio_sqe_acquire(&test_rtio);
	zassert_not_null(sqe);

	push_front_bufs(NULL, &tx_buf, &buf_len);

	rtio_sqe_prep_write(sqe,
			    &test_iodev,
			    RTIO_PRIO_NORM,
			    tx_buf,
			    buf_len,
			    NULL);

	rtio_submit(&test_rtio, 0);
}

static void consume_tx_completion(void)
{
	struct rtio_cqe *cqe;

	cqe = rtio_cqe_consume_block(&test_rtio);
	zassert_not_null(cqe);
	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio, cqe);
}

static int i2s_fake_configure_custom_fake_tx(const struct device *dev,
					     enum i2s_dir dir,
					     const struct i2s_config *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dir);
	test_tx_cfg = *cfg;
	return 0;
}

K_SEM_DEFINE(test_trigger_sem, 0, 1);

static int i2s_fake_trigger_custom_fake_tx(const struct device *dev,
					   enum i2s_dir dir,
					   enum i2s_trigger_cmd cmd)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dir);
	ARG_UNUSED(cmd);
	k_sem_give(&test_trigger_sem);
	return 0;
}

K_SEM_DEFINE(test_write_sem, 0, 1);

static int i2s_fake_write_custom_fake_tx(const struct device *dev,
					 void *mem_block,
					 size_t size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);
	k_sem_give(&test_write_sem);
	return 0;
}

static void *test_setup(void)
{
	i2s_fallback_iodev_init(&test_iodev);
	zassert_true(i2s_fallback_iodev_is_ready(&test_iodev));
	return NULL;
}

static void test_before(void *f)
{
	ARG_UNUSED(f);
	memset(test_rx_bufs, 0, sizeof(test_rx_bufs));
	memset(test_tx_bufs, 0, sizeof(test_tx_bufs));
	test_front_buf_it = 0;
	test_front_buf_val = 0;
	test_back_buf_it = 0;
	test_back_buf_val = 0;
	memset(&test_rx_cfg, 0, sizeof(test_rx_cfg));
	memset(&test_tx_cfg, 0, sizeof(test_tx_cfg));

}

ZTEST_SUITE(i2s_fallback, NULL, test_setup, test_before, NULL, NULL);

ZTEST(i2s_fallback, test_tx)
{
	int ret;
	uint8_t *tx_buf;
	size_t tx_buf_len;

	i2s_fake_configure_fake.custom_fake = i2s_fake_configure_custom_fake_tx;
	i2s_fake_trigger_fake.custom_fake = i2s_fake_trigger_custom_fake_tx;

	for (size_t i = 0; i < 2; i++) {
		submit_tx_submission();
	}

	/* Start should not occur before 3 submissions are preloaded */
	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_not_ok(ret);

	submit_tx_submission();

	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_ok(ret);

	zassert_equal(i2s_fake_configure_fake.call_count, 1);
	zassert_equal(i2s_fake_configure_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_configure_fake.arg1_history[0], I2S_DIR_TX);

	zassert_equal(test_tx_cfg.word_size, TEST_WORD_SIZE);
	zassert_equal(test_tx_cfg.channels, TEST_CHANNELS);
	zassert_equal(test_tx_cfg.format, TEST_FORMAT);
	zassert_equal(test_tx_cfg.options, TEST_OPTIONS);
	zassert_equal(test_tx_cfg.frame_clk_freq, TEST_FRAME_CLK_FREQ);
	zassert_equal(test_tx_cfg.block_size, TEST_BUF_SIZE_MAX);

	zassert_equal(i2s_fake_trigger_fake.call_count, 1);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[0], I2S_DIR_TX);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[0], I2S_TRIGGER_START);

	i2s_fake_write_fake.custom_fake = i2s_fake_write_custom_fake_tx;

	for (size_t i = 0; i < 6; i++) {
		if (i < 4) {
			zassert_equal(i2s_fake_write_fake.call_count, i + 3);
		} else {
			zassert_equal(i2s_fake_write_fake.call_count, 6);
		}

		tx_buf = i2s_fake_write_fake.arg1_history[i];
		tx_buf_len = i2s_fake_write_fake.arg2_history[i];
		pop_back_bufs(NULL, tx_buf, tx_buf_len);

		if (i < 3) {
			submit_tx_submission();
		}

		k_mem_slab_free(test_tx_cfg.mem_slab, tx_buf);

		if (i == 3) {
			ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
			zassert_ok(ret);

			zassert_equal(i2s_fake_trigger_fake.call_count, 2);
			zassert_equal(i2s_fake_trigger_fake.arg0_history[1], test_dev);
			zassert_equal(i2s_fake_trigger_fake.arg1_history[1], I2S_DIR_TX);
			zassert_equal(i2s_fake_trigger_fake.arg2_history[1], I2S_TRIGGER_DRAIN);
		}

		if (i < 3) {
			ret = k_sem_take(&test_write_sem, TEST_TIMEOUT);
			zassert_ok(ret);
		}

		consume_tx_completion();
	}

	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_ok(ret);

	zassert_equal(i2s_fake_trigger_fake.call_count, 3);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[2], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[2], I2S_DIR_TX);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[2], I2S_TRIGGER_DROP);
}
