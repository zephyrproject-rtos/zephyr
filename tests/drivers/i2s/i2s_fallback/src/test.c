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

#define TEST_MAX_BUF_SIZE 512
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

static size_t test_buf_size;

struct test_buf_pool {
	uint8_t *bufs;
	uint16_t buf_count;
	uint16_t buf_size;
	uint8_t head_it;
	uint8_t head_val;
	uint8_t tail_it;
	uint8_t tail_val;
};

#define TEST_BUF_POOL_DEFINE(_name, _buf_count, _buf_size)					\
	static uint8_t CONCAT(_name, _bufs)[(_buf_count) * (_buf_size)];			\
	static struct test_buf_pool _name = {							\
		.bufs = CONCAT(_name, _bufs),							\
		.buf_count = _buf_count,							\
		.buf_size = _buf_size,								\
	}

TEST_BUF_POOL_DEFINE(test_rx_buf_pool, TEST_BUF_COUNT, TEST_MAX_BUF_SIZE);
TEST_BUF_POOL_DEFINE(test_tx_buf_pool, TEST_BUF_COUNT, TEST_MAX_BUF_SIZE);

static void test_buf_pool_reset(struct test_buf_pool *buf_pool)
{
	memset(buf_pool->bufs, 0, buf_pool->buf_count * buf_pool->buf_size);
	buf_pool->head_it = 0;
	buf_pool->head_val = 0;
	buf_pool->tail_it = 0;
	buf_pool->tail_val = 0;
}

static void test_buf_pool_push_tx(struct test_buf_pool *buf_pool,
				  uint8_t **buf,
				  size_t *buf_len)
{
	*buf = &buf_pool->bufs[buf_pool->head_it * buf_pool->buf_size];
	*buf_len = test_buf_size;

	for (size_t i = 0; i < test_buf_size; i++) {
		(*buf)[i] = buf_pool->head_val;
		buf_pool->head_val++;
	}

	buf_pool->head_val += buf_pool->head_it;

	buf_pool->head_it++;
	buf_pool->head_it = buf_pool->head_it == buf_pool->buf_count ? 0 : buf_pool->head_it;
}

static void test_buf_pool_pop_tx(struct test_buf_pool *buf_pool,
				 const uint8_t *buf,
				 size_t buf_len)
{
	zassert_equal(buf_len, test_buf_size);

	for (size_t i = 0; i < buf_len; i++) {
		zassert_equal(buf[i], buf_pool->tail_val);
		buf_pool->tail_val++;
	}

	buf_pool->tail_val += buf_pool->tail_it;

	buf_pool->tail_it++;
	buf_pool->tail_it = buf_pool->tail_it == buf_pool->buf_count ? 0 : buf_pool->tail_it;
}

static void test_buf_pool_push_rx(struct test_buf_pool *buf_pool,
				  uint8_t **buf,
				  size_t *buf_len)
{
	*buf = &buf_pool->bufs[buf_pool->head_it * buf_pool->buf_size];
	*buf_len = test_buf_size;

	buf_pool->head_it++;
	buf_pool->head_it = buf_pool->head_it == buf_pool->buf_count ? 0 : buf_pool->head_it;
}

static void test_buf_pool_fill_rx(struct test_buf_pool *buf_pool,
				  uint8_t *buf,
				  size_t buf_len)
{
	uint8_t val;

	zassert_equal(buf_len, test_buf_size);

	val = buf_pool->tail_val;

	for (size_t i = 0; i < test_buf_size; i++) {
		buf[i] = val;
		val++;
	}
}

static void test_buf_pool_pop_rx(struct test_buf_pool *buf_pool)
{
	const uint8_t *buf;

	buf = &buf_pool->bufs[buf_pool->tail_it * buf_pool->buf_size];

	for (size_t i = 0; i < test_buf_size; i++) {
		zassert_equal(buf[i], buf_pool->tail_val);
		buf_pool->tail_val++;
	}

	buf_pool->tail_val += buf_pool->tail_it;

	buf_pool->tail_it++;
	buf_pool->tail_it = buf_pool->tail_it == buf_pool->buf_count ? 0 : buf_pool->tail_it;
}

I2S_DT_IODEV_DEFINE(
	test_iodev,
	DT_NODELABEL(i2s),
	TEST_WORD_SIZE,
	TEST_CHANNELS,
	TEST_FORMAT,
	TEST_OPTIONS,
	TEST_FRAME_CLK_FREQ,
	TEST_PRELOAD
);

I2S_FALLBACK_DEFINE(test_fallback, 4);

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

	test_buf_pool_push_tx(&test_tx_buf_pool, &tx_buf, &buf_len);

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

static void submit_rx_submission(void)
{
	struct rtio_sqe *sqe;
	uint8_t *rx_buf;
	size_t buf_len;

	sqe = rtio_sqe_acquire(&test_rtio);
	zassert_not_null(sqe);

	test_buf_pool_push_rx(&test_rx_buf_pool, &rx_buf, &buf_len);

	rtio_sqe_prep_read(sqe,
			   &test_iodev,
			   RTIO_PRIO_NORM,
			   rx_buf,
			   buf_len,
			   NULL);

	rtio_submit(&test_rtio, 0);
}

static void consume_rx_completion(void)
{
	struct rtio_cqe *cqe;

	cqe = rtio_cqe_consume_block(&test_rtio);
	zassert_not_null(cqe);
	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio, cqe);
}

static void submit_txrx_submission(void)
{
	struct rtio_sqe *sqe;
	uint8_t *tx_buf;
	size_t buf_len;
	uint8_t *rx_buf;

	sqe = rtio_sqe_acquire(&test_rtio);
	zassert_not_null(sqe);

	test_buf_pool_push_tx(&test_tx_buf_pool, &tx_buf, &buf_len);
	test_buf_pool_push_rx(&test_rx_buf_pool, &rx_buf, &buf_len);

	rtio_sqe_prep_transceive(sqe,
				 &test_iodev,
				 RTIO_PRIO_NORM,
				 tx_buf,
				 rx_buf,
				 buf_len,
				 NULL);

	rtio_submit(&test_rtio, 0);
}

static void consume_txrx_completion(void)
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
	ARGS_UNUSED(dev, dir);
	test_tx_cfg = *cfg;
	return 0;
}

K_SEM_DEFINE(test_trigger_sem, 0, K_SEM_MAX_LIMIT);

static int i2s_fake_trigger_custom_fake_tx(const struct device *dev,
					   enum i2s_dir dir,
					   enum i2s_trigger_cmd cmd)
{
	ARGS_UNUSED(dev, dir, cmd);
	k_sem_give(&test_trigger_sem);
	return 0;
}

K_SEM_DEFINE(test_write_sem, 0, K_SEM_MAX_LIMIT);

static int i2s_fake_write_custom_fake_tx(const struct device *dev,
					 void *mem_block,
					 size_t size)
{
	ARGS_UNUSED(dev, mem_block, size);
	k_sem_give(&test_write_sem);
	return 0;
}

static int i2s_fake_configure_custom_fake_rx(const struct device *dev,
					     enum i2s_dir dir,
					     const struct i2s_config *cfg)
{
	ARGS_UNUSED(dev, dir);
	test_rx_cfg = *cfg;
	return 0;
}

static int i2s_fake_trigger_custom_fake_rx(const struct device *dev,
					   enum i2s_dir dir,
					   enum i2s_trigger_cmd cmd)
{
	ARGS_UNUSED(dev, dir, cmd);
	k_sem_give(&test_trigger_sem);
	return 0;
}

K_SEM_DEFINE(test_read_sem, 0, K_SEM_MAX_LIMIT);

static int i2s_fake_read_custom_fake_rx(const struct device *dev,
					void **mem_block,
					size_t *size)
{
	int ret;
	size_t buf_len;
	uint8_t *rx_buf;

	ARG_UNUSED(dev);

	buf_len = test_rx_cfg.block_size;

	ret = k_mem_slab_alloc(test_rx_cfg.mem_slab, mem_block, K_NO_WAIT);
	zassert_ok(ret);

	rx_buf = *mem_block;

	test_buf_pool_fill_rx(&test_rx_buf_pool, rx_buf, buf_len);
	k_sem_give(&test_read_sem);
	return 0;
}

static int i2s_fake_configure_custom_fake_txrx(const struct device *dev,
					       enum i2s_dir dir,
					       const struct i2s_config *cfg)
{
	ARG_UNUSED(dev);

	if (dir == I2S_DIR_RX) {
		test_rx_cfg = *cfg;
	} else if (dir == I2S_DIR_TX) {
		test_tx_cfg = *cfg;
	}

	return 0;
}

static void i2s_fake_iodev_submit_custom_fake(const struct device *dev,
					      struct rtio_iodev_sqe *iodev_sqe)
{
	ARG_UNUSED(dev);

	i2s_fallback_iodev_submit(&test_fallback, iodev_sqe);
}

static void i2s_fake_iodev_cancel_custom_fake(const struct device *dev)
{
	ARG_UNUSED(dev);

	i2s_fallback_iodev_cancel(&test_fallback);
}

static void *test_setup(void)
{
	i2s_fallback_init(&test_fallback, DEVICE_DT_GET(DT_NODELABEL(i2s)));
	zassert_true(i2s_is_ready_iodev(&test_iodev));
	return NULL;
}

static void test_before(void *f)
{
	ARG_UNUSED(f);
	test_buf_size = TEST_MAX_BUF_SIZE;
	test_buf_pool_reset(&test_rx_buf_pool);
	test_buf_pool_reset(&test_tx_buf_pool);
	memset(&test_rx_cfg, 0, sizeof(test_rx_cfg));
	memset(&test_tx_cfg, 0, sizeof(test_tx_cfg));
	k_sem_reset(&test_trigger_sem);
	k_sem_reset(&test_write_sem);
	k_sem_reset(&test_read_sem);
	i2s_fake_iodev_submit_fake.custom_fake = i2s_fake_iodev_submit_custom_fake;
	i2s_fake_iodev_cancel_fake.custom_fake = i2s_fake_iodev_cancel_custom_fake;
}

ZTEST_SUITE(i2s_fallback, NULL, test_setup, test_before, NULL, NULL);

ZTEST(i2s_fallback, test_tx)
{
	int ret;
	uint8_t *tx_buf;
	size_t tx_buf_len;

	i2s_fake_configure_fake.custom_fake = i2s_fake_configure_custom_fake_tx;
	i2s_fake_trigger_fake.custom_fake = i2s_fake_trigger_custom_fake_tx;
	i2s_fake_write_fake.custom_fake = i2s_fake_write_custom_fake_tx;

	for (size_t i = 0; i < 2; i++) {
		submit_tx_submission();
	}

	/* Start should not occur before 3 submissions are preloaded */
	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_not_ok(ret);

	submit_tx_submission();

	for (size_t i = 0; i < 3; i++) {
		ret = k_sem_take(&test_write_sem, TEST_TIMEOUT);
		zassert_ok(ret);
	}

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
	zassert_equal(test_tx_cfg.block_size, test_buf_size);

	zassert_equal(i2s_fake_trigger_fake.call_count, 1);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[0], I2S_DIR_TX);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[0], I2S_TRIGGER_START);

	for (size_t i = 0; i < 6; i++) {
		if (i < 4) {
			zassert_equal(i2s_fake_write_fake.call_count, i + 3);
		} else {
			zassert_equal(i2s_fake_write_fake.call_count, 6);
		}

		tx_buf = i2s_fake_write_fake.arg1_history[i];
		tx_buf_len = i2s_fake_write_fake.arg2_history[i];
		test_buf_pool_pop_tx(&test_tx_buf_pool, tx_buf, tx_buf_len);

		if (i < 3) {
			submit_tx_submission();
		}

		/* I2S driver completes the write submission by freeing the block */
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

ZTEST(i2s_fallback, test_rx)
{
	int ret;

	i2s_fake_configure_fake.custom_fake = i2s_fake_configure_custom_fake_rx;
	i2s_fake_trigger_fake.custom_fake = i2s_fake_trigger_custom_fake_rx;
	i2s_fake_read_fake.custom_fake = i2s_fake_read_custom_fake_rx;

	for (size_t i = 0; i < 2; i++) {
		submit_rx_submission();
	}

	/* Start should not occur before 3 submissions are preloaded */
	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_not_ok(ret);

	submit_rx_submission();

	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_ok(ret);

	zassert_equal(i2s_fake_configure_fake.call_count, 1);
	zassert_equal(i2s_fake_configure_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_configure_fake.arg1_history[0], I2S_DIR_RX);

	zassert_equal(test_rx_cfg.word_size, TEST_WORD_SIZE);
	zassert_equal(test_rx_cfg.channels, TEST_CHANNELS);
	zassert_equal(test_rx_cfg.format, TEST_FORMAT);
	zassert_equal(test_rx_cfg.options, TEST_OPTIONS);
	zassert_equal(test_rx_cfg.frame_clk_freq, TEST_FRAME_CLK_FREQ);
	zassert_equal(test_rx_cfg.block_size, test_buf_size);

	zassert_equal(i2s_fake_trigger_fake.call_count, 1);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[0], I2S_DIR_RX);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[0], I2S_TRIGGER_START);

	for (size_t i = 0; i < 6; i++) {
		ret = k_sem_take(&test_read_sem, TEST_TIMEOUT);
		zassert_ok(ret);

		if (i < 5) {
			zassert_equal(i2s_fake_read_fake.call_count, i + 1);
		} else {
			zassert_equal(i2s_fake_read_fake.call_count, 6);
		}

		if (i < 3) {
			submit_rx_submission();
		}

		if (i == 4) {
			ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
			zassert_ok(ret);

			zassert_equal(i2s_fake_trigger_fake.call_count, 2);
			zassert_equal(i2s_fake_trigger_fake.arg0_history[1], test_dev);
			zassert_equal(i2s_fake_trigger_fake.arg1_history[1], I2S_DIR_RX);
			zassert_equal(i2s_fake_trigger_fake.arg2_history[1], I2S_TRIGGER_STOP);
		}

		consume_rx_completion();
		test_buf_pool_pop_rx(&test_rx_buf_pool);
	}

	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_ok(ret);

	zassert_equal(i2s_fake_trigger_fake.call_count, 3);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[2], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[2], I2S_DIR_RX);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[2], I2S_TRIGGER_DROP);
}

ZTEST(i2s_fallback, test_txrx)
{
	int ret;
	uint8_t *tx_buf;
	size_t tx_buf_len;

	i2s_fake_configure_fake.custom_fake = i2s_fake_configure_custom_fake_txrx;
	i2s_fake_trigger_fake.custom_fake = i2s_fake_trigger_custom_fake_tx;
	i2s_fake_write_fake.custom_fake = i2s_fake_write_custom_fake_tx;
	i2s_fake_read_fake.custom_fake = i2s_fake_read_custom_fake_rx;

	for (size_t i = 0; i < 2; i++) {
		submit_txrx_submission();
	}

	/* Start should not occur before 3 submissions are preloaded */
	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_not_ok(ret);

	submit_txrx_submission();

	for (size_t i = 0; i < 3; i++) {
		ret = k_sem_take(&test_write_sem, TEST_TIMEOUT);
		zassert_ok(ret);
	}

	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_ok(ret);

	zassert_equal(i2s_fake_configure_fake.call_count, 2);
	zassert_equal(i2s_fake_configure_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_configure_fake.arg1_history[0], I2S_DIR_RX);
	zassert_equal(i2s_fake_configure_fake.arg0_history[1], test_dev);
	zassert_equal(i2s_fake_configure_fake.arg1_history[1], I2S_DIR_TX);

	zassert_equal(test_rx_cfg.word_size, TEST_WORD_SIZE);
	zassert_equal(test_rx_cfg.channels, TEST_CHANNELS);
	zassert_equal(test_rx_cfg.format, TEST_FORMAT);
	zassert_equal(test_rx_cfg.options, TEST_OPTIONS);
	zassert_equal(test_rx_cfg.frame_clk_freq, TEST_FRAME_CLK_FREQ);
	zassert_equal(test_rx_cfg.block_size, test_buf_size);

	zassert_equal(test_tx_cfg.word_size, TEST_WORD_SIZE);
	zassert_equal(test_tx_cfg.channels, TEST_CHANNELS);
	zassert_equal(test_tx_cfg.format, TEST_FORMAT);
	zassert_equal(test_tx_cfg.options, TEST_OPTIONS);
	zassert_equal(test_tx_cfg.frame_clk_freq, TEST_FRAME_CLK_FREQ);
	zassert_equal(test_tx_cfg.block_size, test_buf_size);

	zassert_equal(i2s_fake_trigger_fake.call_count, 1);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[0], I2S_DIR_BOTH);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[0], I2S_TRIGGER_START);

	for (size_t i = 0; i < 6; i++) {
		ret = k_sem_take(&test_read_sem, TEST_TIMEOUT);
		zassert_ok(ret);

		zassert_equal(i2s_fake_read_fake.call_count, i + 1);

		if (i < 4) {
			zassert_equal(i2s_fake_write_fake.call_count, i + 3);
		} else {
			zassert_equal(i2s_fake_write_fake.call_count, 6);
		}

		tx_buf = i2s_fake_write_fake.arg1_history[i];
		tx_buf_len = i2s_fake_write_fake.arg2_history[i];
		test_buf_pool_pop_tx(&test_tx_buf_pool, tx_buf, tx_buf_len);

		if (i < 3) {
			submit_txrx_submission();
		}

		/* I2S driver completes the write submission by freeing the block */
		k_mem_slab_free(test_tx_cfg.mem_slab, tx_buf);

		if (i == 3) {
			ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
			zassert_ok(ret);

			zassert_equal(i2s_fake_trigger_fake.call_count, 2);
			zassert_equal(i2s_fake_trigger_fake.arg0_history[1], test_dev);
			zassert_equal(i2s_fake_trigger_fake.arg1_history[1], I2S_DIR_BOTH);
			zassert_equal(i2s_fake_trigger_fake.arg2_history[1], I2S_TRIGGER_DRAIN);
		}

		if (i < 3) {
			ret = k_sem_take(&test_write_sem, TEST_TIMEOUT);
			zassert_ok(ret);
		}

		consume_txrx_completion();
		test_buf_pool_pop_rx(&test_rx_buf_pool);
	}

	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_ok(ret);

	zassert_equal(i2s_fake_trigger_fake.call_count, 3);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[2], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[2], I2S_DIR_BOTH);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[2], I2S_TRIGGER_DROP);
}

ZTEST(i2s_fallback, test_cancel)
{
	int ret;
	uint8_t *tx_buf;
	size_t tx_buf_len;
	struct rtio_sqe *sqe;
	rtio_sqe_handle_t handles[5];
	struct rtio_cqe *cqe;

	i2s_fake_configure_fake.custom_fake = i2s_fake_configure_custom_fake_tx;
	i2s_fake_trigger_fake.custom_fake = i2s_fake_trigger_custom_fake_tx;
	i2s_fake_write_fake.custom_fake = i2s_fake_write_custom_fake_tx;

	/*
	 * Start stream with 2 submission pending in IO queue, 3 submissions queued up in hardware.
	 * Point the submission userdata to the handle pointer to link the completions to the
	 * submissions.
	 */
	ARRAY_FOR_EACH(handles, i) {
		sqe = rtio_sqe_acquire(&test_rtio);
		zassert_not_null(sqe);

		test_buf_pool_push_tx(&test_tx_buf_pool, &tx_buf, &tx_buf_len);

		rtio_sqe_prep_write(sqe,
				    &test_iodev,
				    RTIO_PRIO_NORM,
				    tx_buf,
				    tx_buf_len,
				    &handles[i]);

		handles[i] = rtio_sqe_handle(&test_rtio, sqe);

		rtio_submit(&test_rtio, 0);
	}

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
	zassert_equal(test_tx_cfg.block_size, test_buf_size);

	zassert_equal(i2s_fake_write_fake.call_count, 3);

	zassert_equal(i2s_fake_trigger_fake.call_count, 1);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[0], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[0], I2S_DIR_TX);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[0], I2S_TRIGGER_START);

	/* Cancel one of the submissions in the IO queue */
	rtio_sqe_cancel(&test_rtio, handles[3]);
	cqe = rtio_cqe_consume_block(&test_rtio);
	zassert_not_null(cqe);
	zassert_equal(cqe->result, -ECANCELED);
	zassert_equal(cqe->userdata, &handles[3]);
	rtio_cqe_release(&test_rtio, cqe);

	/* Validate the stream was not affected since we cancelled a submission in the IO queue */
	zassert_equal(i2s_fake_write_fake.call_count, 3);
	zassert_equal(i2s_fake_trigger_fake.call_count, 1);
	cqe = rtio_cqe_consume(&test_rtio);
	zassert_is_null(cqe);

	/*
	 * Cancel one of the submissions which have been queued up in hardware. This should stop
	 * the stream immediately, dropping and cancelling the queued up submissions.
	 */
	rtio_sqe_cancel(&test_rtio, handles[1]);

	/* Drop should be triggered */
	ret = k_sem_take(&test_trigger_sem, TEST_TIMEOUT);
	zassert_ok(ret);
	zassert_equal(i2s_fake_trigger_fake.call_count, 2);
	zassert_equal(i2s_fake_trigger_fake.arg0_history[1], test_dev);
	zassert_equal(i2s_fake_trigger_fake.arg1_history[1], I2S_DIR_TX);
	zassert_equal(i2s_fake_trigger_fake.arg2_history[1], I2S_TRIGGER_DROP);

	/* Drop will release all the blocks written to it */
	for (size_t i = 0; i < 3; i++) {
		tx_buf = i2s_fake_write_fake.arg1_history[i];
		k_mem_slab_free(test_tx_cfg.mem_slab, tx_buf);
	}

	for (size_t i = 0; i < 3; i++) {
		cqe = rtio_cqe_consume_block(&test_rtio);
		zassert_not_null(cqe);
		zassert_equal(cqe->result, -ECANCELED);
		zassert_equal(cqe->userdata, &handles[i]);
		rtio_cqe_release(&test_rtio, cqe);
	}

	/* The last submission should still be pending in the IO queue */
	cqe = rtio_cqe_consume(&test_rtio);
	zassert_is_null(cqe);

	/* Cancel the last submission */
	rtio_sqe_cancel(&test_rtio, handles[4]);
	cqe = rtio_cqe_consume_block(&test_rtio);
	zassert_not_null(cqe);
	zassert_equal(cqe->result, -ECANCELED);
	zassert_equal(cqe->userdata, &handles[4]);
	rtio_cqe_release(&test_rtio, cqe);
}
