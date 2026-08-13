/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * RTIO based driver for the nRF I2S peripheral, written to exercise the I2S RTIO
 * API rather than to be complete.
 *
 * The driver takes ownership of a submission when it hands its buffers to the
 * peripheral, and completes every submission it owns exactly once:
 *
 *   i2s_rtio_submit()   makes a submission current when no stream is running,
 *                       the stream is started with that submission
 *   i2s_rtio_continue() takes the submission following the one the peripheral is
 *                       transferring, called only when the peripheral needs the
 *                       buffers of the next block
 *   i2s_rtio_complete() completes the oldest owned submission, called once for
 *                       every block the peripheral releases
 *
 * The stream ends as soon as there is no submission to stream back to back with
 * the one being transferred, and starts again once one is submitted.
 *
 * Submissions and peripheral events are assumed to be serialized, the driver
 * state is not protected against a submission racing with an event.
 */

#if defined(CONFIG_I2S_NRFX_RTIO)

#define DT_DRV_COMPAT nordic_nrf_i2s

#include <zephyr/drivers/i2s.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <nrfx_i2s.h>
#include <hal/nrf_clock.h>

#include "i2s_rtio.h"

LOG_MODULE_REGISTER(i2s_nrfx_rtio, CONFIG_I2S_LOG_LEVEL);

/* Submissions owned at once: one being transferred, one queued up in hardware,
 * and one taken from the queue in the event which releases the oldest one.
 */
#define OWNED_MAX 3

enum driver_state {
	/* No stream, the peripheral is uninitialized */
	STATE_IDLE,
	/* The peripheral is initialized, waiting for its clock to start */
	STATE_CLOCK,
	/* Blocks are streaming */
	STATE_STREAM,
	/* The last block of the stream is transferring, waiting for the stop */
	STATE_DRAIN,
};

struct block_bufs {
	void *tx_buf;
	void *rx_buf;
	size_t size;
	enum i2s_dir dir;
};

struct i2s_nrfx_rtio_data {
	const struct device *dev;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;
	struct i2s_config cfg;
	nrfx_i2s_config_t nrfx_cfg;
	nrfx_i2s_t i2s;
	nrfx_i2s_buffers_t start_bufs;
	int owned_status[OWNED_MAX];
	uint8_t owned_count;
	enum driver_state state;
	enum i2s_dir stream_dir;
	bool request_clock;
	bool stop_issued;
	bool configured;
};

struct i2s_nrfx_rtio_config {
	struct i2s_rtio *rtio;
	nrfx_i2s_data_handler_t data_handler;
	nrfx_i2s_config_t nrfx_def_cfg;
	const struct pinctrl_dev_config *pcfg;
	/* Words transmitted and received by the block which ends a stream */
	uint32_t *stop_buf;
	enum clock_source {
		PCLK32M,
		PCLK32M_HFXO,
		ACLK
	} clk_src;
};

static void stream_start(const struct device *dev);
static void stream_end(const struct device *dev);

static void owned_push(struct i2s_nrfx_rtio_data *drv_data)
{
	__ASSERT_NO_MSG(drv_data->owned_count < OWNED_MAX);

	drv_data->owned_status[drv_data->owned_count] = 0;
	drv_data->owned_count++;
}

static void owned_status_set_last(struct i2s_nrfx_rtio_data *drv_data, int status)
{
	__ASSERT_NO_MSG(drv_data->owned_count > 0);

	drv_data->owned_status[drv_data->owned_count - 1] = status;
}

static void owned_status_set_all(struct i2s_nrfx_rtio_data *drv_data, int status)
{
	for (uint8_t i = 0; i < drv_data->owned_count; i++) {
		drv_data->owned_status[i] = status;
	}
}

/*
 * Completes the oldest owned submission. Submissions are completed in the order
 * they were handed to the peripheral, which is the order the I2S RTIO context
 * completes them in.
 */
static bool owned_complete_first(const struct device *dev)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;
	int status;

	drv_cfg = dev->config;
	drv_data = dev->data;

	__ASSERT_NO_MSG(drv_data->owned_count > 0);

	status = drv_data->owned_status[0];
	drv_data->owned_count--;

	for (uint8_t i = 0; i < drv_data->owned_count; i++) {
		drv_data->owned_status[i] = drv_data->owned_status[i + 1];
	}

	return i2s_rtio_complete(drv_cfg->rtio, status);
}

static bool is_clk_controller(const struct i2s_config *i2s_cfg)
{
	return (i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET) == 0 &&
	       (i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET) == 0;
}

static int block_bufs_get(struct block_bufs *bufs, const struct rtio_iodev_sqe *iodev_sqe)
{
	const struct rtio_sqe *sqe;

	sqe = &iodev_sqe->sqe;
	bufs->tx_buf = NULL;
	bufs->rx_buf = NULL;

	switch (sqe->op) {
	case RTIO_OP_TXRX:
		bufs->dir = I2S_DIR_BOTH;
		bufs->tx_buf = (void *)sqe->txrx.tx_buf;
		bufs->rx_buf = sqe->txrx.rx_buf;
		bufs->size = sqe->txrx.buf_len;
		break;
	case RTIO_OP_TX:
		bufs->dir = I2S_DIR_TX;
		bufs->tx_buf = (void *)sqe->tx.buf;
		bufs->size = sqe->tx.buf_len;
		break;
	case RTIO_OP_RX:
		bufs->dir = I2S_DIR_RX;
		bufs->rx_buf = sqe->rx.buf;
		bufs->size = sqe->rx.buf_len;
		break;
	default:
		LOG_ERR("Unsupported operation: %u", sqe->op);
		return -ENOTSUP;
	}

	if (bufs->size < sizeof(uint32_t) || (bufs->size % sizeof(uint32_t)) != 0) {
		LOG_ERR("Blocks must hold whole 32-bit words: %zu", bufs->size);
		return -EINVAL;
	}

	if ((bufs->size / sizeof(uint32_t)) > UINT16_MAX) {
		LOG_ERR("Block is too large: %zu", bufs->size);
		return -EINVAL;
	}

	if (bufs->dir != I2S_DIR_RX && bufs->tx_buf == NULL) {
		return -EINVAL;
	}

	if (bufs->dir != I2S_DIR_TX && bufs->rx_buf == NULL) {
		return -EINVAL;
	}

	return 0;
}

static void block_bufs_to_nrfx(const struct block_bufs *bufs, nrfx_i2s_buffers_t *nrfx_bufs)
{
	nrfx_bufs->p_tx_buffer = bufs->tx_buf;
	nrfx_bufs->p_rx_buffer = bufs->rx_buf;
	nrfx_bufs->buffer_size = bufs->size / sizeof(uint32_t);
}

/*
 * The peripheral needs the buffers of the block which follows the last block of
 * a stream, so that the last block is transferred in full. Dummy words are
 * supplied for it, the peripheral is stopped once the last block is released.
 */
static void stop_bufs_get(const struct i2s_nrfx_rtio_config *drv_cfg, enum i2s_dir dir,
			  nrfx_i2s_buffers_t *nrfx_bufs)
{
	uint32_t *stop_buf;

	stop_buf = drv_cfg->stop_buf;
	nrfx_bufs->p_tx_buffer = (dir != I2S_DIR_RX) ? &stop_buf[0] : NULL;
	nrfx_bufs->p_rx_buffer = (dir != I2S_DIR_TX) ? &stop_buf[1] : NULL;
	nrfx_bufs->buffer_size = 1;
}

static bool is_stop_bufs(const struct i2s_nrfx_rtio_config *drv_cfg,
			 const nrfx_i2s_buffers_t *released)
{
	uint32_t *stop_buf;

	stop_buf = drv_cfg->stop_buf;

	return released->p_tx_buffer == &stop_buf[0] || released->p_rx_buffer == &stop_buf[1];
}

/* Finds the clock settings that give the frame clock frequency closest to
 * the one requested, taking into account the hardware limitations.
 */
static void find_suitable_clock(const struct i2s_nrfx_rtio_config *drv_cfg,
				nrfx_i2s_config_t *nrfx_cfg,
				const struct i2s_config *i2s_cfg)
{
	const nrfx_i2s_clk_params_t clk_params = {
		.base_clock_freq =
			(NRF_I2S_HAS_CLKCONFIG && drv_cfg->clk_src == ACLK)
			/* The I2S_NRFX_RTIO_DEVICE() macro contains build assertions
			 * that make sure that the ACLK clock source is only used when
			 * it is available and only with the "hfclkaudio-frequency"
			 * property defined, but the default value of 0 here needs to
			 * be used to prevent compilation errors when the property is
			 * not defined (this expression will be eventually optimized
			 * away then).
			 */
			? DT_PROP_OR(DT_NODELABEL(clock), hfclkaudio_frequency, 0)
			: 32 * 1000 * 1000UL,
		.transfer_rate = i2s_cfg->frame_clk_freq,
		.swidth = nrfx_cfg->sample_width,
		.allow_bypass = IS_ENABLED(CONFIG_I2S_NRFX_ALLOW_MCK_BYPASS),
	};

	if (nrfx_i2s_prescalers_calc(&clk_params, &nrfx_cfg->prescalers) != 0) {
		LOG_ERR("Failed to find suitable I2S clock configuration.");
	}
}

static int stream_configure(const struct device *dev, enum i2s_dir dir,
			    const struct i2s_config *i2s_cfg)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;
	nrfx_i2s_config_t nrfx_cfg;

	if (i2s_cfg->frame_clk_freq == 0) {
		return -EINVAL;
	}

	drv_cfg = dev->config;
	drv_data = dev->data;
	nrfx_cfg = drv_cfg->nrfx_def_cfg;

	switch (i2s_cfg->word_size) {
	case 8:
		nrfx_cfg.sample_width = NRF_I2S_SWIDTH_8BIT;
		break;
	case 16:
		nrfx_cfg.sample_width = NRF_I2S_SWIDTH_16BIT;
		break;
	case 24:
		nrfx_cfg.sample_width = NRF_I2S_SWIDTH_24BIT;
		break;
#if defined(I2S_CONFIG_SWIDTH_SWIDTH_32Bit)
	case 32:
		nrfx_cfg.sample_width = NRF_I2S_SWIDTH_32BIT;
		break;
#endif
	default:
		LOG_ERR("Unsupported word size: %u", i2s_cfg->word_size);
		return -EINVAL;
	}

	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		nrfx_cfg.alignment = NRF_I2S_ALIGN_LEFT;
		nrfx_cfg.format = NRF_I2S_FORMAT_I2S;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		nrfx_cfg.alignment = NRF_I2S_ALIGN_LEFT;
		nrfx_cfg.format = NRF_I2S_FORMAT_ALIGNED;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		nrfx_cfg.alignment = NRF_I2S_ALIGN_RIGHT;
		nrfx_cfg.format = NRF_I2S_FORMAT_ALIGNED;
		break;
	default:
		LOG_ERR("Unsupported data format: 0x%02x", i2s_cfg->format);
		return -EINVAL;
	}

	if ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) ||
	    (i2s_cfg->format & I2S_FMT_BIT_CLK_INV) ||
	    (i2s_cfg->format & I2S_FMT_FRAME_CLK_INV)) {
		LOG_ERR("Unsupported stream format: 0x%02x", i2s_cfg->format);
		return -EINVAL;
	}

	if (i2s_cfg->channels == 2) {
		nrfx_cfg.channels = NRF_I2S_CHANNELS_STEREO;
	} else if (i2s_cfg->channels == 1) {
		nrfx_cfg.channels = NRF_I2S_CHANNELS_LEFT;
	} else {
		LOG_ERR("Unsupported number of channels: %u", i2s_cfg->channels);
		return -EINVAL;
	}

	if (i2s_cfg->options & (I2S_OPT_LOOPBACK | I2S_OPT_PINGPONG)) {
		LOG_ERR("Unsupported options: 0x%02x", i2s_cfg->options);
		return -EINVAL;
	}

	if ((i2s_cfg->options & I2S_OPT_BIT_CLK_TARGET) &&
	    (i2s_cfg->options & I2S_OPT_FRAME_CLK_TARGET)) {
		nrfx_cfg.mode = NRF_I2S_MODE_SLAVE;
	} else if (is_clk_controller(i2s_cfg)) {
		nrfx_cfg.mode = NRF_I2S_MODE_MASTER;
	} else {
		LOG_ERR("Unsupported operation mode: 0x%02x", i2s_cfg->options);
		return -EINVAL;
	}

	/* If the master clock generator is needed (i.e. in Master mode or when
	 * the MCK output is used), find a suitable clock configuration for it.
	 */
	if (nrfx_cfg.mode == NRF_I2S_MODE_MASTER ||
	    nrf_i2s_mck_pin_connected_check(drv_data->i2s.p_reg)) {
		find_suitable_clock(drv_cfg, &nrfx_cfg, i2s_cfg);
		/* Unless the PCLK32M source is used with the HFINT oscillator
		 * (which is always available without any additional actions),
		 * it is required to request the proper clock to be running
		 * before starting the transfer itself.
		 */
		drv_data->request_clock = (drv_cfg->clk_src != PCLK32M);
	} else {
		nrfx_cfg.prescalers.mck_setup = NRF_I2S_MCK_DISABLED;
		drv_data->request_clock = false;
	}

	drv_data->nrfx_cfg = nrfx_cfg;
	drv_data->cfg = *i2s_cfg;
	drv_data->stream_dir = dir;
	drv_data->configured = true;

	return 0;
}

static void stream_stop(struct i2s_nrfx_rtio_data *drv_data)
{
	if (drv_data->stop_issued) {
		return;
	}

	drv_data->stop_issued = true;
	nrfx_i2s_stop(&drv_data->i2s);
}

static void stream_end(const struct device *dev)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;
	nrfx_i2s_buffers_t stop_bufs;
	int ret;

	drv_cfg = dev->config;
	drv_data = dev->data;
	drv_data->state = STATE_DRAIN;

	stop_bufs_get(drv_cfg, drv_data->stream_dir, &stop_bufs);

	ret = nrfx_i2s_next_buffers_set(&drv_data->i2s, &stop_bufs);
	if (ret != 0) {
		stream_stop(drv_data);
	}
}

static void stream_abort(const struct device *dev, int status)
{
	struct i2s_nrfx_rtio_data *drv_data;

	drv_data = dev->data;

	/* Every block still owned by the driver is cut short by the abort */
	owned_status_set_all(drv_data, status);
	drv_data->state = STATE_DRAIN;

	stream_stop(drv_data);
}

/*
 * Hands the buffers of the submission following the one being transferred to the
 * peripheral, or ends the stream if there is none to stream back to back.
 */
static void stream_supply_next(const struct device *dev)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;
	struct i2s_rtio *rtio;
	struct block_bufs bufs;
	nrfx_i2s_buffers_t nrfx_bufs;
	int ret;

	drv_data = dev->data;

	if (drv_data->state != STATE_STREAM) {
		/* The block which ends the stream is transferring, so the
		 * peripheral is done with the last block of the stream.
		 */
		stream_stop(drv_data);
		return;
	}

	drv_cfg = dev->config;
	rtio = drv_cfg->rtio;

	__ASSERT_NO_MSG(rtio->curr != NULL);

	if (!i2s_rtio_continue(rtio)) {
		stream_end(dev);
		return;
	}

	owned_push(drv_data);

	ret = block_bufs_get(&bufs, rtio->curr);
	if (ret == 0 && bufs.dir != drv_data->stream_dir) {
		LOG_ERR("Submission does not match the direction of the stream");
		ret = -EINVAL;
	}

	if (ret == 0) {
		block_bufs_to_nrfx(&bufs, &nrfx_bufs);
		ret = nrfx_i2s_next_buffers_set(&drv_data->i2s, &nrfx_bufs);
	}

	if (ret != 0) {
		owned_status_set_last(drv_data, ret);
		stream_end(dev);
	}
}

static void block_release(const struct device *dev, const nrfx_i2s_buffers_t *released)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;

	if (released->p_tx_buffer == NULL && released->p_rx_buffer == NULL) {
		/* Nothing was transferred yet, the stream has just started */
		return;
	}

	drv_cfg = dev->config;

	if (is_stop_bufs(drv_cfg, released)) {
		/* The block which ends a stream does not belong to a submission */
		return;
	}

	drv_data = dev->data;

	if (drv_data->owned_count == 0) {
		return;
	}

	if (owned_complete_first(dev) && drv_data->state == STATE_STREAM) {
		/* The completed submission was never queued up in hardware, so
		 * the submission which follows it needs a stream of its own.
		 */
		stream_end(dev);
	}
}

static void stream_finalize(const struct device *dev)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;
	struct i2s_rtio *rtio;

	drv_data = dev->data;

	nrfx_i2s_uninit(&drv_data->i2s);

	if (drv_data->request_clock) {
		(void)onoff_release(drv_data->clk_mgr);
	}

	drv_data->state = STATE_IDLE;
	drv_data->stop_issued = false;

	/* Submissions the peripheral never released, because the stream ended
	 * before their block was transferred.
	 */
	while (drv_data->owned_count > 0) {
		(void)owned_complete_first(dev);
	}

	drv_cfg = dev->config;
	rtio = drv_cfg->rtio;

	if (rtio->curr != NULL) {
		stream_start(dev);
	}
}

static void data_handler(const struct device *dev, const nrfx_i2s_buffers_t *released,
			 uint32_t status)
{
	if (status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) {
		if (released == NULL) {
			/* The peripheral reused the buffers of the block it is
			 * transferring because the next ones were not supplied
			 * on time, so that block is corrupted.
			 */
			LOG_ERR("Next buffers not supplied on time");
			stream_abort(dev, -EIO);
		} else {
			stream_supply_next(dev);
		}
	}

	if (released != NULL) {
		block_release(dev, released);
	}

	if (status & NRFX_I2S_STATUS_TRANSFER_STOPPED) {
		stream_finalize(dev);
	}
}

static int stream_hw_start(const struct device *dev)
{
	struct i2s_nrfx_rtio_data *drv_data;
	int ret;

	drv_data = dev->data;

	ret = nrfx_i2s_start(&drv_data->i2s, &drv_data->start_bufs, 0);
	if (ret != 0) {
		LOG_ERR("Failed to start I2S transfer: %d", ret);
		nrfx_i2s_uninit(&drv_data->i2s);

		if (drv_data->request_clock) {
			(void)onoff_release(drv_data->clk_mgr);
		}

		drv_data->state = STATE_IDLE;

		return ret;
	}

	drv_data->state = STATE_STREAM;

	return 0;
}

static void clock_started_callback(struct onoff_manager *mgr, struct onoff_client *cli,
				   uint32_t state, int res)
{
	struct i2s_nrfx_rtio_data *drv_data;
	const struct device *dev;
	int ret;

	ARG_UNUSED(mgr);
	ARG_UNUSED(state);
	ARG_UNUSED(res);

	drv_data = CONTAINER_OF(cli, struct i2s_nrfx_rtio_data, clk_cli);
	dev = drv_data->dev;

	__ASSERT_NO_MSG(drv_data->state == STATE_CLOCK);

	ret = stream_hw_start(dev);
	if (ret != 0) {
		owned_status_set_last(drv_data, ret);

		if (owned_complete_first(dev)) {
			stream_start(dev);
		}
	}
}

/*
 * Takes ownership of the current submission and starts a stream with it. The
 * blocks start flowing once the peripheral clock is running.
 */
static int stream_try_start(const struct device *dev)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;
	struct i2s_rtio *rtio;
	struct rtio_iodev_sqe *iodev_sqe;
	const struct rtio_iodev *iodev;
	const struct i2s_iodev_data *iodev_data;
	struct block_bufs bufs;
	struct i2s_config i2s_cfg;
	int ret;

	drv_cfg = dev->config;
	drv_data = dev->data;
	rtio = drv_cfg->rtio;
	iodev_sqe = rtio->curr;

	__ASSERT_NO_MSG(drv_data->state == STATE_IDLE);
	__ASSERT_NO_MSG(iodev_sqe != NULL);

	owned_push(drv_data);

	ret = block_bufs_get(&bufs, iodev_sqe);
	if (ret != 0) {
		return ret;
	}

	iodev = iodev_sqe->sqe.iodev;
	iodev_data = iodev->data;
	i2s_cfg = iodev_data->config;
	i2s_cfg.block_size = bufs.size;

	ret = stream_configure(dev, bufs.dir, &i2s_cfg);
	if (ret != 0) {
		return ret;
	}

	block_bufs_to_nrfx(&bufs, &drv_data->start_bufs);

	ret = nrfx_i2s_init(&drv_data->i2s, &drv_data->nrfx_cfg, drv_cfg->data_handler);
	if (ret != 0) {
		LOG_ERR("Failed to initialize I2S: %d", ret);
		return ret;
	}

#if NRF_I2S_HAS_CLKCONFIG
	nrf_i2s_clk_configure(drv_data->i2s.p_reg,
			      drv_cfg->clk_src == ACLK ? NRF_I2S_CLKSRC_ACLK
						       : NRF_I2S_CLKSRC_PCLK32M,
			      drv_data->nrfx_cfg.prescalers.enable_bypass);
#endif

	if (!drv_data->request_clock) {
		return stream_hw_start(dev);
	}

	sys_notify_init_callback(&drv_data->clk_cli.notify, clock_started_callback);

	ret = onoff_request(drv_data->clk_mgr, &drv_data->clk_cli);
	if (ret < 0) {
		LOG_ERR("Failed to request clock: %d", ret);
		nrfx_i2s_uninit(&drv_data->i2s);
		return ret;
	}

	drv_data->state = STATE_CLOCK;

	return 0;
}

static void stream_start(const struct device *dev)
{
	struct i2s_nrfx_rtio_data *drv_data;
	int ret;

	drv_data = dev->data;

	do {
		ret = stream_try_start(dev);
		if (ret == 0) {
			return;
		}

		/* The submission can not be streamed, complete it and try the
		 * submission which follows it, if any.
		 */
		owned_status_set_last(drv_data, ret);
	} while (owned_complete_first(dev));
}

static void i2s_nrfx_rtio_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct i2s_nrfx_rtio_config *drv_cfg;
	struct i2s_nrfx_rtio_data *drv_data;

	drv_cfg = dev->config;

	if (!i2s_rtio_submit(drv_cfg->rtio, iodev_sqe)) {
		/* A stream is running, the submission is queued up in hardware
		 * once the peripheral needs it.
		 */
		return;
	}

	drv_data = dev->data;

	if (drv_data->state != STATE_IDLE) {
		/* The previous stream is still stopping, the submission starts a
		 * stream once it has.
		 */
		return;
	}

	stream_start(dev);
}

static int i2s_nrfx_rtio_configure(const struct device *dev, enum i2s_dir dir,
				   const struct i2s_config *i2s_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dir);
	ARG_UNUSED(i2s_cfg);

	return -ENOTSUP;
}

static const struct i2s_config *i2s_nrfx_rtio_config_get(const struct device *dev,
							 enum i2s_dir dir)
{
	struct i2s_nrfx_rtio_data *drv_data;

	drv_data = dev->data;

	if (!drv_data->configured) {
		return NULL;
	}

	if (dir != drv_data->stream_dir && drv_data->stream_dir != I2S_DIR_BOTH) {
		return NULL;
	}

	return &drv_data->cfg;
}

static int i2s_nrfx_rtio_read(const struct device *dev, void **mem_block, size_t *size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

static int i2s_nrfx_rtio_write(const struct device *dev, void *mem_block, size_t size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

static int i2s_nrfx_rtio_trigger(const struct device *dev, enum i2s_dir dir,
				 enum i2s_trigger_cmd cmd)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dir);
	ARG_UNUSED(cmd);

	return -ENOTSUP;
}

static void init_clock_manager(const struct device *dev)
{
	struct i2s_nrfx_rtio_data *drv_data;
	clock_control_subsys_t subsys;
#if NRF_CLOCK_HAS_HFCLKAUDIO
	const struct i2s_nrfx_rtio_config *drv_cfg;
#endif

	drv_data = dev->data;
	subsys = CLOCK_CONTROL_NRF_SUBSYS_HF;

#if NRF_CLOCK_HAS_HFCLKAUDIO
	drv_cfg = dev->config;

	if (drv_cfg->clk_src == ACLK) {
		subsys = CLOCK_CONTROL_NRF_SUBSYS_HFAUDIO;
	}
#endif

	drv_data->clk_mgr = z_nrf_clock_control_get_onoff(subsys);
	__ASSERT_NO_MSG(drv_data->clk_mgr != NULL);
}

static DEVICE_API(i2s, i2s_nrfx_rtio_api) = {
	.configure = i2s_nrfx_rtio_configure,
	.config_get = i2s_nrfx_rtio_config_get,
	.read = i2s_nrfx_rtio_read,
	.write = i2s_nrfx_rtio_write,
	.trigger = i2s_nrfx_rtio_trigger,
	.iodev_submit = i2s_nrfx_rtio_submit,
};

#define I2S_CLK_SRC(inst) DT_STRING_TOKEN(DT_DRV_INST(inst), clock_source)

#define I2S_NRFX_RTIO_DEVICE(inst)								\
	I2S_RTIO_DEFINE(i2s_rtio##inst);							\
	static uint32_t stop_buf##inst[2];							\
	static void data_handler##inst(nrfx_i2s_buffers_t const *p_released, uint32_t status)	\
	{											\
		data_handler(DEVICE_DT_GET(DT_DRV_INST(inst)), p_released, status);		\
	}											\
	PINCTRL_DT_DEFINE(DT_DRV_INST(inst));							\
	static const struct i2s_nrfx_rtio_config i2s_nrfx_rtio_cfg##inst = {			\
		.rtio = &i2s_rtio##inst,							\
		.data_handler = data_handler##inst,						\
		.nrfx_def_cfg = NRFX_I2S_DEFAULT_CONFIG(					\
			NRF_I2S_PIN_NOT_CONNECTED, NRF_I2S_PIN_NOT_CONNECTED,			\
			NRF_I2S_PIN_NOT_CONNECTED, NRF_I2S_PIN_NOT_CONNECTED,			\
			NRF_I2S_PIN_NOT_CONNECTED),						\
		.nrfx_def_cfg.skip_gpio_cfg = true,						\
		.nrfx_def_cfg.skip_psel_cfg = true,						\
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(inst)),				\
		.stop_buf = stop_buf##inst,							\
		.clk_src = I2S_CLK_SRC(inst),							\
	};											\
	static struct i2s_nrfx_rtio_data i2s_nrfx_rtio_data##inst = {				\
		.i2s = NRFX_I2S_INSTANCE(DT_INST_REG_ADDR(inst)),				\
		.state = STATE_IDLE,								\
	};											\
	static int i2s_nrfx_rtio_init##inst(const struct device *dev)				\
	{											\
		const struct i2s_nrfx_rtio_config *drv_cfg = dev->config;			\
		int ret;									\
												\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),			\
			    nrfx_i2s_irq_handler, &i2s_nrfx_rtio_data##inst.i2s, 0);		\
		ret = pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_DEFAULT);		\
		if (ret < 0) {									\
			return ret;								\
		}										\
		i2s_nrfx_rtio_data##inst.dev = dev;						\
		i2s_rtio_init(&i2s_rtio##inst, dev);						\
		init_clock_manager(dev);							\
		return 0;									\
	}											\
	BUILD_ASSERT(I2S_CLK_SRC(inst) != ACLK ||						\
			     (NRF_I2S_HAS_CLKCONFIG && NRF_CLOCK_HAS_HFCLKAUDIO),		\
		     "Clock source ACLK is not available.");					\
	BUILD_ASSERT(I2S_CLK_SRC(inst) != ACLK ||						\
			     DT_NODE_HAS_PROP(DT_NODELABEL(clock), hfclkaudio_frequency),	\
		     "Clock source ACLK requires the hfclkaudio-frequency "			\
		     "property to be defined in the nordic,nrf-clock node.");			\
	DEVICE_DT_INST_DEFINE(inst, i2s_nrfx_rtio_init##inst, NULL, &i2s_nrfx_rtio_data##inst,	\
			      &i2s_nrfx_rtio_cfg##inst, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,	\
			      &i2s_nrfx_rtio_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_NRFX_RTIO_DEVICE)

#endif /* CONFIG_I2S_NRFX_RTIO */
