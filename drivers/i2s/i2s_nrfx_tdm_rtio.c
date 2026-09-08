/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <soc.h>
#include <dmm.h>
#include <nrfx_tdm.h>

#include "i2s_rtio.h"

#define DT_DRV_COMPAT nordic_nrf_tdm

struct driver_data {
	nrfx_tdm_t tdm;
	bool configured;
	struct nrf_clock_spec aclk_spec;
};

struct driver_config {
	struct i2s_rtio *ctx;
	void (*irq_connect)(void);
	void (*data_handler)(nrfx_tdm_buffers_t const *released, uint32_t status);
	const struct pinctrl_dev_config *pcfg;
	const struct device *aclk;
};

static void init_tdm_buffers(const struct device *dev, nrfx_tdm_buffers_t *buffers)
{
	const struct driver_config *dev_config = dev->config;
	struct i2s_rtio *ctx = dev_config->ctx;
	struct rtio_iodev_sqe *iodev_sqe = ctx->curr;
	struct rtio_sqe *sqe = &iodev_sqe->sqe;

	switch (sqe->op) {
	case RTIO_OP_RX:
		buffers->p_rx_buffer = (uint32_t *)sqe->rx.buf;
		buffers->rx_buffer_size = sqe->rx.buf_len / sizeof(uint32_t);
		buffers->p_tx_buffer = NULL;
		buffers->tx_buffer_size = 0;
		break;

	case RTIO_OP_TX:
		buffers->p_rx_buffer = NULL;
		buffers->rx_buffer_size = 0;
		buffers->p_tx_buffer = (const uint32_t *)sqe->tx.buf;
		buffers->tx_buffer_size = sqe->tx.buf_len / sizeof(uint32_t);
		break;

	case RTIO_OP_TXRX:
		buffers->p_rx_buffer = (uint32_t *)sqe->txrx.rx_buf;
		buffers->rx_buffer_size = sqe->txrx.buf_len / sizeof(uint32_t);
		buffers->p_tx_buffer = (const uint32_t *)sqe->txrx.tx_buf;
		buffers->tx_buffer_size = buffers->rx_buffer_size;
		break;

	default:
		break;
	}
}

static int driver_configure(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	struct i2s_rtio *ctx = dev_config->ctx;
	struct rtio_iodev_sqe *iodev_sqe = ctx->curr;
	struct rtio_sqe *sqe = &iodev_sqe->sqe;
	const struct rtio_iodev *iodev = sqe->iodev;
	struct i2s_iodev_data *iodev_data = iodev->data;
	struct i2s_iodev_config *iodev_config = &iodev_data->config;
	nrfx_tdm_t *tdm = &dev_data->tdm;
	nrfx_tdm_config_t tdm_cfg;
	uint8_t extra_channels;
	uint8_t max_num_of_channels;
	uint8_t channel_count;
	i2s_fmt_t data_format;
	nrf_tdm_fsync_duration_t fsync_duration;
	uint32_t sck_freq;
	nrfx_tdm_clk_params_t clk_params;
	int ret;

	if (iodev_config->frame_clk_freq == 0) {
		return -EINVAL;
	}

	data_format = iodev_config->format & I2S_FMT_DATA_FORMAT_MASK;
#if NRF_TDM_HAS_CONFIG_FSYNC_DURATION_ENUM
	fsync_duration = (data_format == I2S_FMT_DATA_FORMAT_PCM_SHORT ||
			  data_format == I2S_FMT_DATA_FORMAT_PCM_LONG) ?
			 NRF_TDM_FSYNC_DURATION_SCK : NRF_TDM_FSYNC_DURATION_CHANNEL;
#else
	fsync_duration = (data_format == I2S_FMT_DATA_FORMAT_PCM_SHORT ||
			    data_format == I2S_FMT_DATA_FORMAT_PCM_LONG) ?
			   1 : iodev_config->word_size;
#endif

	switch (iodev_config->word_size) {
	case 8:
		tdm_cfg.sample_width = NRF_TDM_SWIDTH_8BIT;
		break;
	case 16:
		tdm_cfg.sample_width = NRF_TDM_SWIDTH_16BIT;
		break;
	case 24:
		tdm_cfg.sample_width = NRF_TDM_SWIDTH_24BIT;
		break;
	case 32:
		tdm_cfg.sample_width = NRF_TDM_SWIDTH_32BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (data_format) {
	case I2S_FMT_DATA_FORMAT_I2S:
		tdm_cfg.alignment = NRF_TDM_ALIGN_LEFT;
		tdm_cfg.fsync_polarity = NRF_TDM_POLARITY_NEGEDGE;
		tdm_cfg.sck_polarity = NRF_TDM_POLARITY_POSEDGE;
		tdm_cfg.fsync_duration = fsync_duration;
		tdm_cfg.channel_delay = NRF_TDM_CHANNEL_DELAY_1CK;
		max_num_of_channels = 2;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		tdm_cfg.alignment = NRF_TDM_ALIGN_LEFT;
		tdm_cfg.fsync_polarity = NRF_TDM_POLARITY_POSEDGE;
		tdm_cfg.sck_polarity = NRF_TDM_POLARITY_POSEDGE;
		tdm_cfg.fsync_duration = fsync_duration;
		tdm_cfg.channel_delay = NRF_TDM_CHANNEL_DELAY_NONE;
		max_num_of_channels = 2;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		tdm_cfg.alignment = NRF_TDM_ALIGN_RIGHT;
		tdm_cfg.fsync_polarity = NRF_TDM_POLARITY_POSEDGE;
		tdm_cfg.sck_polarity = NRF_TDM_POLARITY_POSEDGE;
		tdm_cfg.fsync_duration = fsync_duration;
		tdm_cfg.channel_delay = NRF_TDM_CHANNEL_DELAY_NONE;
		max_num_of_channels = 2;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		tdm_cfg.alignment = NRF_TDM_ALIGN_LEFT;
		tdm_cfg.fsync_polarity = NRF_TDM_POLARITY_NEGEDGE;
		tdm_cfg.sck_polarity = NRF_TDM_POLARITY_NEGEDGE;
		tdm_cfg.fsync_duration = fsync_duration;
		tdm_cfg.channel_delay = NRF_TDM_CHANNEL_DELAY_NONE;
		max_num_of_channels = NRFX_TDM_NUM_OF_CHANNELS;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		tdm_cfg.alignment = NRF_TDM_ALIGN_LEFT;
		tdm_cfg.fsync_polarity = NRF_TDM_POLARITY_POSEDGE;
		tdm_cfg.sck_polarity = NRF_TDM_POLARITY_NEGEDGE;
		tdm_cfg.fsync_duration = fsync_duration;
		tdm_cfg.channel_delay = NRF_TDM_CHANNEL_DELAY_NONE;
		max_num_of_channels = NRFX_TDM_NUM_OF_CHANNELS;
		break;
	default:
		return -EINVAL;
	}

	if (iodev_config->format &
	    (I2S_FMT_DATA_ORDER_LSB | I2S_FMT_BIT_CLK_INV | I2S_FMT_FRAME_CLK_INV)) {
		return -EINVAL;
	}

	extra_channels = 0;
	if (iodev_config->channels == 1 &&
#if NRF_TDM_HAS_CONFIG_FSYNC_DURATION_ENUM
	    fsync_duration == NRF_TDM_FSYNC_DURATION_CHANNEL
#else
	    fsync_duration == iodev_config->word_size
#endif
	) {
		extra_channels = 1;
	} else if (iodev_config->channels > max_num_of_channels) {
		return -EINVAL;
	}

	channel_count = iodev_config->channels + extra_channels;
	tdm_cfg.channel_number = channel_count;
	tdm_cfg.rx_channel_mask = (nrf_tdm_channel_mask_t)(((1UL << iodev_config->channels) - 1) <<
		TDM_CONFIG_CHANNEL_MASK_Rx0Enable_Pos);
	tdm_cfg.tx_channel_mask = (nrf_tdm_channel_mask_t)(((1UL << iodev_config->channels) - 1) <<
		TDM_CONFIG_CHANNEL_MASK_Tx0Enable_Pos);

	/* TODO: why though */

	switch (sqe->op) {
	case RTIO_OP_TX:
		tdm_cfg.rx_channel_mask = 0;
		break;
	case RTIO_OP_RX:
		tdm_cfg.tx_channel_mask = 0;
		break;
	case RTIO_OP_TXRX:
		break;
	default:
		return -EINVAL;
	}

	if (iodev_config->options & (I2S_OPT_LOOPBACK | I2S_OPT_PINGPONG)) {
		return -EINVAL;
	}

	if ((iodev_config->options & I2S_OPT_BIT_CLK_TARGET) &&
	    (iodev_config->options & I2S_OPT_FRAME_CLK_TARGET)) {
		tdm_cfg.mode = NRF_TDM_MODE_SLAVE;
	} else if ((iodev_config->options & I2S_OPT_BIT_CLK_TARGET) == 0 &&
		   (iodev_config->options & I2S_OPT_FRAME_CLK_TARGET) == 0) {
		tdm_cfg.mode = NRF_TDM_MODE_MASTER;
	} else {
		return -EINVAL;
	}

	tdm_cfg.ors = 0;
	tdm_cfg.mck_src = NRF_TDM_SRC_PCLK32M;
	tdm_cfg.sck_src = NRF_TDM_SRC_PCLK32M;
	tdm_cfg.mck_bypass = false;
	tdm_cfg.sck_bypass = false;
	tdm_cfg.prescalers.mck_div = 0;
	tdm_cfg.prescalers.sck_div = 0;

	if (tdm_cfg.mode == NRF_TDM_MODE_MASTER) {
		sck_freq = iodev_config->word_size * iodev_config->frame_clk_freq * channel_count;
		clk_params.base_mck_freq = 0;
		clk_params.base_sck_freq = 32 * 1000 * 1000UL;
		clk_params.mck_freq = 0;
		clk_params.sck_freq = sck_freq;
		ret = nrfx_tdm_prescalers_calc(&clk_params, &tdm_cfg.prescalers);
		if (ret != 0) {
			return ret;
		}
	}

	tdm_cfg.skip_gpio_cfg = true;
	tdm_cfg.skip_psel_cfg = true;

	if (dev_data->configured) {
		ret = nrfx_tdm_reconfigure(tdm, &tdm_cfg);
	} else {
		ret = nrfx_tdm_init(tdm, &tdm_cfg, dev_config->data_handler);
	}

	if (ret) {
		return ret;
	}

	dev_data->configured = true;

	return ret;
}

static int driver_start_transfer(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	nrfx_tdm_buffers_t initial_buffers;
	int ret;

	ret = driver_configure(dev);
	if (ret) {
		return ret;
	}

	init_tdm_buffers(dev, &initial_buffers);
	return nrfx_tdm_start(&dev_data->tdm, &initial_buffers);
}

static int driver_continue_transfer(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	nrfx_tdm_buffers_t next_buffers;

	init_tdm_buffers(dev, &next_buffers);
	return nrfx_tdm_next_buffers_set(&dev_data->tdm, &next_buffers);
}

static void driver_continue(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	int ret;
	bool pending;

	while (true) {
		ret = driver_start_transfer(dev);
		if (ret == 0) {
			break;
		}

		pending = i2s_rtio_complete(dev_config->ctx, ret);

		if (!pending) {
			(void)pm_device_runtime_put(dev);
			break;
		}
	}
}

static void driver_start(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	int ret;
	bool pending;

	ret = pm_device_runtime_get(dev);
	if (ret) {
		/* Drain every submission */
		while (i2s_rtio_complete(dev_config->ctx, ret)) {
		}

		return;
	}

	while (true) {
		ret = driver_start_transfer(dev);
		if (ret == 0) {
			break;
		}

		pending = i2s_rtio_complete(dev_config->ctx, ret);

		if (!pending) {
			(void)pm_device_runtime_put(dev);
			break;
		}
	}
}

static void tdm_data_handler(const struct device *dev,
			     nrfx_tdm_buffers_t const *released,
			     uint32_t status)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	struct i2s_rtio *ctx = dev_config->ctx;
	nrfx_tdm_t *tdm = &dev_data->tdm;
	bool pending;
	bool stream;

	if (status & NRFX_TDM_STATUS_NEXT_BUFFERS_NEEDED) {
		/* TDM wants the next submission to continue transfer */
		stream = i2s_rtio_continue(ctx);

		if (stream) {
			/* Continue transfer with next submission */
			driver_continue_transfer(dev);
		} else {
			/*
			 * Next submission can not continue transfer. We will stop the transfer
			 * after the current submission is complete. We will get a callback once
			 * the current submission is complete, and restart the transfer from
			 * there.
			 */
			nrfx_tdm_stop(tdm, false);
		}
	}

	if (released == NULL) {
		return;
	}

	if (released->p_rx_buffer != NULL || released->p_tx_buffer != NULL) {
		/* Submission is complete */
		pending = i2s_rtio_complete(ctx, 0);

		if (pending) {
			driver_continue(dev);
		}

		return;
	}

}

static void driver_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct driver_config *dev_config = dev->config;

	if (i2s_rtio_submit(dev_config->ctx, iodev_sqe)) {
		driver_start(dev);
	}
}

static DEVICE_API(i2s, driver_api) = {
	.iodev_submit = driver_iodev_submit,
};

static int driver_suspend(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	int ret;

	if (dev_data->configured) {
		nrfx_tdm_uninit(&dev_data->tdm);
		dev_data->configured = false;
	}

	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret) {
		return ret;
	}

	return 0;
}

static int driver_resume(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	return pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
}

int driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = driver_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = driver_resume(dev);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;

	dev_data->configured = false;
	i2s_rtio_init(dev_config->ctx);
	dev_config->irq_connect();

	return pm_device_driver_init(dev, driver_pm_action);
}

#define DRIVER_DEFINE(inst)									\
	I2S_RTIO_DEFINE(CONCAT(ctx, inst));							\
												\
	static struct driver_data CONCAT(data, inst) = {					\
		.tdm = NRFX_TDM_INSTANCE(DT_INST_REG_ADDR(inst)),				\
	};											\
												\
	NRF_DT_INST_IRQ_DIRECT_DEFINE(								\
		inst,										\
		nrfx_tdm_irq_handler,								\
		&CONCAT(data, inst)								\
	)											\
												\
	static void CONCAT(irq_connect, inst)(void)						\
	{											\
		NRF_DT_INST_IRQ_CONNECT(							\
			inst,									\
			nrfx_tdm_irq_handler,							\
			&CONCAT(data, inst)							\
		);										\
	}											\
												\
	static void CONCAT(data_handler, inst)(nrfx_tdm_buffers_t const *released,		\
					       uint32_t status)					\
	{											\
		tdm_data_handler(DEVICE_DT_INST_GET(inst), released, status);			\
	}											\
												\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, driver_pm_handler);					\
												\
	static const struct driver_config CONCAT(config, inst) = {				\
		.ctx = &CONCAT(ctx, inst),							\
		.irq_connect = CONCAT(irq_connect, inst),					\
		.data_handler = CONCAT(data_handler, inst),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		driver_init,									\
		PM_DEVICE_DT_INST_GET(inst),							\
		&CONCAT(data, inst),								\
		&CONCAT(config, inst),								\
		POST_KERNEL,									\
		CONFIG_I2S_INIT_PRIORITY,							\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_DEFINE)
