/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_i2s_ssie

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <r_ssi.h>
#include <r_dtc.h>

#include <soc.h>

#define I2S_OPT_BIT_CLK_FRAME_CLK_MASK 0x6
#define SSI_CLOCK_DIV_INVALID          (-1)

LOG_MODULE_REGISTER(renesas_ra_i2s_ssie, CONFIG_I2S_LOG_LEVEL);

struct renesas_ra_ssie_config {
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	const struct device *audio_clock_dev;
};

struct renesas_ra_ssie_stream {
	void *mem_block;
	size_t mem_block_len;
};

struct renesas_ra_ssie_data {
	ssi_instance_ctrl_t fsp_ctrl;
	i2s_cfg_t fsp_cfg;
	ssi_extended_cfg_t fsp_ext_cfg;
	struct k_spinlock lock;
	volatile enum i2s_state state;
	enum i2s_dir active_dir;
	struct k_msgq rx_queue;
	struct k_msgq tx_queue;
	struct i2s_config tx_cfg;
	struct i2s_config rx_cfg;
	struct renesas_ra_ssie_stream tx_stream;
	struct renesas_ra_ssie_stream rx_stream;
#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	struct renesas_ra_ssie_stream rx_stream_next;
#endif
	struct renesas_ra_ssie_stream tx_msgs[CONFIG_I2S_RENESAS_RA_SSIE_TX_BLOCK_COUNT];
	struct renesas_ra_ssie_stream rx_msgs[CONFIG_I2S_RENESAS_RA_SSIE_RX_BLOCK_COUNT];
	bool tx_configured;
	bool rx_configured;
	bool stop_with_draining;
	bool full_duplex;
	bool trigger_drop;

#if defined(CONFIG_I2S_RENESAS_RA_SSIE_DTC)
	struct st_transfer_instance rx_transfer;
	struct st_transfer_cfg rx_transfer_cfg;
	struct st_dtc_instance_ctrl rx_transfer_ctrl;
	struct st_dtc_extended_cfg rx_transfer_cfg_extend;
	struct st_transfer_info DTC_TRANSFER_INFO_ALIGNMENT rx_transfer_info;

	struct st_transfer_instance tx_transfer;
	struct st_transfer_cfg tx_transfer_cfg;
	struct st_dtc_instance_ctrl tx_transfer_ctrl;
	struct st_dtc_extended_cfg tx_transfer_cfg_extend;
	struct st_transfer_info DTC_TRANSFER_INFO_ALIGNMENT tx_transfer_info;
#endif /* CONFIG_I2S_RENESAS_RA_SSIE_DTC */
};

/* FSP interruption handlers. */
void ssi_txi_isr(void);
void ssi_rxi_isr(void);
void ssi_int_isr(void);

__maybe_unused static void ssi_rt_isr(void *p_args)
{
	const struct device *dev = (struct device *)p_args;
	struct renesas_ra_ssie_data *dev_data = (struct renesas_ra_ssie_data *)dev->data;

	if (dev_data->active_dir == I2S_DIR_TX) {
		ssi_txi_isr();
	}

	if (dev_data->active_dir == I2S_DIR_RX) {
		ssi_rxi_isr();
	}
}

static int audio_clock_enable(const struct device *dev)
{
	const struct renesas_ra_ssie_config *config = dev->config;
	const struct device *audio_clk_dev = config->audio_clock_dev;
	uint32_t rate;
	int ret;

	if (!device_is_ready(audio_clk_dev)) {
		LOG_ERR("Invalid audio_clock device");
		return -ENODEV;
	}

	ret = clock_control_on(audio_clk_dev, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to enable Audio clock, error %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(config->audio_clock_dev, NULL, &rate);
	if (ret < 0) {
		LOG_ERR("Failed to get audio clock rate, error: (%d)", ret);
		return ret;
	}

	return 0;
}

static ssi_clock_div_t get_ssi_clock_div_enum(uint32_t divisor)
{
	switch (divisor) {
	case 1:
		return SSI_CLOCK_DIV_1;
	case 2:
		return SSI_CLOCK_DIV_2;
	case 4:
		return SSI_CLOCK_DIV_4;
	case 6:
		return SSI_CLOCK_DIV_6;
	case 8:
		return SSI_CLOCK_DIV_8;
	case 12:
		return SSI_CLOCK_DIV_12;
	case 16:
		return SSI_CLOCK_DIV_16;
	case 24:
		return SSI_CLOCK_DIV_24;
	case 32:
		return SSI_CLOCK_DIV_32;
	case 48:
		return SSI_CLOCK_DIV_48;
	case 64:
		return SSI_CLOCK_DIV_64;
	case 96:
		return SSI_CLOCK_DIV_96;
	case 128:
		return SSI_CLOCK_DIV_128;
	default:
		return SSI_CLOCK_DIV_INVALID;
	}
}

static int renesas_ra_ssie_set_clock_divider(const struct device *dev,
					     const struct i2s_config *i2s_cfg)
{
	const uint32_t valid_divisors[] = {1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128};
	const struct renesas_ra_ssie_config *config = dev->config;
	struct renesas_ra_ssie_data *dev_data = dev->data;
	ssi_extended_cfg_t *fsp_ext_cfg = &dev_data->fsp_ext_cfg;
	const uint64_t target_bclk =
		i2s_cfg->word_size * i2s_cfg->channels * i2s_cfg->frame_clk_freq;
	uint32_t selected_div = 0;
	int error_min = INT_MAX;
	ssi_clock_div_t bit_clock_div = SSI_CLOCK_DIV_INVALID;
	uint32_t rate;
	int ret;

	ret = clock_control_get_rate(config->audio_clock_dev, NULL, &rate);
	if (ret < 0) {
		LOG_ERR("Failed to get audio clock rate, error: (%d)", ret);
		return ret;
	}

	if (rate <= 0) {
		return -EIO;
	}

	for (size_t i = 0; i < ARRAY_SIZE(valid_divisors); i++) {
		const uint64_t actual_bclk = DIV_ROUND_CLOSEST(rate, valid_divisors[i]);
		int err = INT_MAX;

		if (actual_bclk >= target_bclk) {
			err = DIV_ROUND_UP(1000 * (actual_bclk - target_bclk), target_bclk);
		} else {
			err = DIV_ROUND_UP(1000 * (target_bclk - actual_bclk), target_bclk);
		}

		if (selected_div == 0 || err < error_min) {
			selected_div = valid_divisors[i];
			error_min = err;

			if (error_min == 0) {
				break;
			}
		}
	}

	if (selected_div == 0 || error_min > 100) {
		LOG_ERR("Cannot find suitable clock config for target bit clock: %llu Hz",
			target_bclk);
		return -EIO;
	}

	bit_clock_div = get_ssi_clock_div_enum(selected_div);
	if (bit_clock_div == SSI_CLOCK_DIV_INVALID) {
		LOG_ERR("Invalid clock divisor selected");
		return -EINVAL;
	}

	LOG_DBG("SPS error: %d 1/1000", error_min);

	fsp_ext_cfg->bit_clock_div = bit_clock_div;

	return 0;
}

static void i2s_renesas_ra_release_stream(struct renesas_ra_ssie_stream *stream)
{
	__ASSERT((stream != NULL), "Invalid parameter");

	stream->mem_block = NULL;
	stream->mem_block_len = 0;
}

static int i2s_renesas_ra_alloc_stream(struct i2s_config *cfg,
				       struct renesas_ra_ssie_stream *stream)
{
	int ret = 0;

	__ASSERT((cfg != NULL && stream != NULL), "Invalid parameter");

	if (k_mem_slab_num_free_get(cfg->mem_slab) > 0) {
		ret = k_mem_slab_alloc(cfg->mem_slab, &stream->mem_block, K_NO_WAIT);
		if (ret == 0) {
			stream->mem_block_len = cfg->block_size;
		}
	} else {
		ret = -ENOMEM;
	}

	if (ret < 0) {
		stream->mem_block = NULL;
		stream->mem_block_len = 0;
	}

	return ret;
}

static void i2s_renesas_ra_free_stream(struct i2s_config *cfg,
				       struct renesas_ra_ssie_stream *stream)
{
	__ASSERT((cfg != NULL && stream != NULL), "Invalid parameter");

	if (stream->mem_block != NULL) {
		k_mem_slab_free(cfg->mem_slab, stream->mem_block);
	}

	stream->mem_block = NULL;
	stream->mem_block_len = 0;
}

static int i2s_renesas_ra_put_stream(struct k_msgq *msgq, struct renesas_ra_ssie_stream *stream)
{
	__ASSERT((msgq != NULL && stream != NULL), "Invalid parameter");

	return k_msgq_put(msgq, stream, K_NO_WAIT);
}

static int i2s_renesas_ra_get_stream(struct k_msgq *msgq, struct renesas_ra_ssie_stream *stream,
				     k_timeout_t timeout)
{
	__ASSERT((msgq != NULL && stream != NULL), "Invalid parameter");

	return k_msgq_get(msgq, stream, timeout);
}

static void drop_queue(const struct device *dev, enum i2s_dir dir)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;
	struct i2s_config *cfg = dir == I2S_DIR_TX ? &dev_data->tx_cfg : &dev_data->rx_cfg;
	struct k_msgq *msgq = dir == I2S_DIR_TX ? &dev_data->tx_queue : &dev_data->rx_queue;
	struct renesas_ra_ssie_stream stream;
	int ret;

	while ((ret = k_msgq_get(msgq, &stream, K_NO_WAIT)) != -ENOMSG) {
		if (ret == 0) {
			k_mem_slab_free(cfg->mem_slab, stream.mem_block);
		}
	}
}

static int renesas_ra_ssie_rx_start_transfer(const struct device *dev)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	struct renesas_ra_ssie_stream *stream = &dev_data->rx_stream;
	int ret;
	fsp_err_t fsp_err;

	ret = i2s_renesas_ra_alloc_stream(&dev_data->rx_cfg, stream);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	ret = i2s_renesas_ra_alloc_stream(&dev_data->rx_cfg, &dev_data->rx_stream_next);
	if (ret < 0) {
		LOG_DBG("Failed to allocate secondary RX buffer");
	}
#endif

	fsp_err = R_SSI_Read(&dev_data->fsp_ctrl, stream->mem_block, stream->mem_block_len);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to start read data");
		dev_data->state = I2S_STATE_ERROR;
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream);
#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream_next);
#endif
		return -EIO;
	}

	dev_data->state = I2S_STATE_RUNNING;

	return 0;
}

static int renesas_ra_ssie_tx_start_transfer(const struct device *dev)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	struct renesas_ra_ssie_stream *tx_stream = &dev_data->tx_stream;
	fsp_err_t fsp_err;
	int ret;

	ret = i2s_renesas_ra_get_stream(&dev_data->tx_queue, tx_stream, K_NO_WAIT);
	if (ret < 0) {
		dev_data->state = I2S_STATE_ERROR;
		return -ENOMEM;
	}

	fsp_err = R_SSI_Write(&dev_data->fsp_ctrl, tx_stream->mem_block, tx_stream->mem_block_len);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to start write data");
		dev_data->state = I2S_STATE_ERROR;
		i2s_renesas_ra_free_stream(&dev_data->tx_cfg, tx_stream);
		return -EIO;
	}

	dev_data->state = I2S_STATE_RUNNING;

	return 0;
}

static int renesas_ra_ssie_tx_rx_start_transfer(const struct device *dev)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	struct renesas_ra_ssie_stream *stream_tx = &dev_data->tx_stream;
	struct renesas_ra_ssie_stream *stream_rx = &dev_data->rx_stream;
	fsp_err_t fsp_err;
	int ret;

	ret = i2s_renesas_ra_get_stream(&dev_data->tx_queue, stream_tx, K_NO_WAIT);
	if (ret < 0) {
		dev_data->state = I2S_STATE_ERROR;
		return -ENOMEM;
	}

#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	if (dev_data->state == I2S_STATE_RUNNING || dev_data->state == I2S_STATE_STOPPING) {
		if (dev_data->rx_stream_next.mem_block == NULL) {
			i2s_renesas_ra_free_stream(&dev_data->tx_cfg, stream_tx);
			dev_data->state = I2S_STATE_ERROR;
			return -ENOMEM;
		}

		*stream_rx = dev_data->rx_stream_next;
	} else {
#endif
		ret = i2s_renesas_ra_alloc_stream(&dev_data->rx_cfg, stream_rx);
		if (ret < 0) {
			i2s_renesas_ra_free_stream(&dev_data->tx_cfg, stream_tx);
			dev_data->state = I2S_STATE_ERROR;
			return ret;
		}
#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	}
#endif

	fsp_err = R_SSI_WriteRead(&dev_data->fsp_ctrl, stream_tx->mem_block, stream_rx->mem_block,
				  MAX(stream_rx->mem_block_len, stream_tx->mem_block_len));
	if (fsp_err != FSP_SUCCESS) {
		dev_data->state = I2S_STATE_ERROR;
		i2s_renesas_ra_free_stream(&dev_data->tx_cfg, stream_tx);
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, stream_rx);
		LOG_ERR("Failed to start write and read data");
		return -EIO;
	}

#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	ret = i2s_renesas_ra_alloc_stream(&dev_data->rx_cfg, &dev_data->rx_stream_next);
	if (ret < 0) {
		LOG_DBG("Failed to allocate secondary RX buffer");
	}
#endif

	/* In case I2S stream is draining, don't update the state */
	if (dev_data->state != I2S_STATE_STOPPING) {
		dev_data->state = I2S_STATE_RUNNING;
	}

	return 0;
}

static void renesas_ra_ssie_idle_dir_both_handle(const struct device *dev)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	struct renesas_ra_ssie_stream *stream_tx = &dev_data->tx_stream;
	bool restart = true;

	if (dev_data->state == I2S_STATE_STOPPING) {
		/*
		 * In case device is stopping, restart the transfer only if there is still data in
		 * the TX queue and the trigger command is I2S_TRIGGER_DRAIN.
		 */
		if (k_msgq_num_used_get(&dev_data->tx_queue) == 0 ||
		    dev_data->stop_with_draining == false) {
			dev_data->state = I2S_STATE_READY;
			restart = false;
		}
	}

	i2s_renesas_ra_free_stream(&dev_data->tx_cfg, stream_tx);

	if (restart) {
		renesas_ra_ssie_tx_rx_start_transfer(dev);
	}
}

static void renesas_ra_ssie_idle_tx_handle(const struct device *dev)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;

	if (dev_data->state == I2S_STATE_STOPPING) {
		i2s_renesas_ra_free_stream(&dev_data->tx_cfg, &dev_data->tx_stream);
		dev_data->state = I2S_STATE_READY;
	}

	if (dev_data->state == I2S_STATE_RUNNING) {
		renesas_ra_ssie_tx_start_transfer(dev);
	}
}

static void renesas_ra_ssie_idle_rx_handle(const struct device *dev)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;

	if (dev_data->state == I2S_STATE_STOPPING) {
		dev_data->state = I2S_STATE_READY;
	}
}

static void renesas_ra_ssie_rx_callback(const struct device *dev)
{
#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	bool free_stream = false;
	bool free_next_stream = false;
	fsp_err_t fsp_err;
	int ret;

	if (dev_data->rx_stream.mem_block == NULL) {
		return;
	}

	if (dev_data->rx_stream_next.mem_block != NULL) {
		fsp_err = R_SSI_Read(&dev_data->fsp_ctrl, dev_data->rx_stream_next.mem_block,
				     dev_data->rx_stream_next.mem_block_len);
		if (fsp_err != FSP_SUCCESS) {
			dev_data->state = I2S_STATE_ERROR;
			LOG_ERR("Failed to restart RX transfer");
			free_next_stream = true;
		}
	} else {
		free_next_stream = true;
	}

	if (dev_data->trigger_drop) {
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream);
		return;
	}

	ret = i2s_renesas_ra_put_stream(&dev_data->rx_queue, &dev_data->rx_stream);
	if (ret < 0) {
		dev_data->state = I2S_STATE_ERROR;
		free_stream = true;
		free_next_stream = true;
		goto free;
	}

	if (dev_data->active_dir == I2S_DIR_RX) {
		if (dev_data->state == I2S_STATE_STOPPING) {
			goto stop;
		}

		if (free_next_stream) {
			dev_data->state = I2S_STATE_ERROR;
			goto free;
		}

		dev_data->rx_stream = dev_data->rx_stream_next;
		ret = i2s_renesas_ra_alloc_stream(&dev_data->rx_cfg, &dev_data->rx_stream_next);
		if (ret < 0) {
			LOG_DBG("Failed to allocate RX buffer");
		}
	}

	return;
free:
	if (free_stream) {
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream);
	}

	if (free_next_stream) {
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream_next);
	}
stop:
	i2s_renesas_ra_release_stream(&dev_data->rx_stream);
	R_SSI_Stop(&dev_data->fsp_ctrl);
#else
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	fsp_err_t fsp_err;
	int ret;

	if (dev_data->rx_stream.mem_block == NULL) {
		return;
	}

	if (dev_data->trigger_drop) {
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream);
		return;
	}

	ret = i2s_renesas_ra_put_stream(&dev_data->rx_queue, &dev_data->rx_stream);
	if (ret < 0) {
		dev_data->state = I2S_STATE_ERROR;
		goto free;
	}

	if (dev_data->active_dir == I2S_DIR_RX) {
		if (dev_data->state == I2S_STATE_STOPPING) {
			goto stop;
		}

		ret = i2s_renesas_ra_alloc_stream(&dev_data->rx_cfg, &dev_data->rx_stream);
		if (ret < 0) {
			dev_data->state = I2S_STATE_ERROR;
			goto stop;
		}

		fsp_err = R_SSI_Read(&dev_data->fsp_ctrl, dev_data->rx_stream.mem_block,
				     dev_data->rx_stream.mem_block_len);
		if (fsp_err != FSP_SUCCESS) {
			dev_data->state = I2S_STATE_ERROR;
			LOG_ERR("Failed to restart RX transfer");
			goto free;
		}
	}

	return;
free:
	i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream);
stop:
	i2s_renesas_ra_release_stream(&dev_data->rx_stream);
	R_SSI_Stop(&dev_data->fsp_ctrl);
#endif
}

static void renesas_ra_ssie_tx_callback(const struct device *dev)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	struct renesas_ra_ssie_stream *tx_stream = &dev_data->tx_stream;
	fsp_err_t fsp_err;
	int ret;

	if (dev_data->trigger_drop) {
		goto tx_disable;
	}

	if (dev_data->active_dir == I2S_DIR_TX) {
		if (dev_data->state == I2S_STATE_STOPPING) {
			/* If there is no msg in tx queue, set decive state to ready*/
			if (k_msgq_num_used_get(&dev_data->tx_queue) == 0) {
				goto tx_disable;
			}

			if (dev_data->stop_with_draining == false) {
				goto tx_disable;
			}
		}

		i2s_renesas_ra_free_stream(&dev_data->tx_cfg, tx_stream);

		ret = i2s_renesas_ra_get_stream(&dev_data->tx_queue, tx_stream, K_NO_WAIT);
		if (ret < 0) {
			goto tx_disable;
		}

		fsp_err = R_SSI_Write(&dev_data->fsp_ctrl, tx_stream->mem_block,
				      tx_stream->mem_block_len);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to restart write data");
		}
	}

	return;
tx_disable:
	i2s_renesas_ra_free_stream(&dev_data->tx_cfg, tx_stream);
}

static void renesas_ra_ssie_idle_callback(const struct device *dev)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;

	if (dev_data->trigger_drop) {
		dev_data->state = I2S_STATE_READY;
		return;
	}

	switch (dev_data->active_dir) {
	case I2S_DIR_BOTH:
		renesas_ra_ssie_idle_dir_both_handle(dev);
		break;
	case I2S_DIR_TX:
		renesas_ra_ssie_idle_tx_handle(dev);
		break;
	case I2S_DIR_RX:
		renesas_ra_ssie_idle_rx_handle(dev);
		break;
	default:
		LOG_ERR("Invalid direction: %d", dev_data->active_dir);
	}
}

static void renesas_ra_ssie_callback(i2s_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;

	switch (p_args->event) {
	case I2S_EVENT_IDLE:
		renesas_ra_ssie_idle_callback(dev);
		break;

	case I2S_EVENT_TX_EMPTY:
		renesas_ra_ssie_tx_callback(dev);
		break;

	case I2S_EVENT_RX_FULL:
		renesas_ra_ssie_rx_callback(dev);
		break;

	default:
		LOG_ERR("Invalid triger envent: %d", p_args->event);
	}
}

static int renesas_ra_ssie_start_transfer(const struct device *dev, enum i2s_dir dir)
{
	struct renesas_ra_ssie_data *const dev_data = dev->data;
	int ret;

	if (dev_data->state != I2S_STATE_READY) {
		return -EIO;
	}

	/* Start transfer following the current state */
	switch (dir) {
	case I2S_DIR_BOTH:
		ret = renesas_ra_ssie_tx_rx_start_transfer(dev);
		break;
	case I2S_DIR_TX:
		ret = renesas_ra_ssie_tx_start_transfer(dev);
		break;
	case I2S_DIR_RX:
		ret = renesas_ra_ssie_rx_start_transfer(dev);
		break;
	default:
		LOG_ERR("Invalid direction: %d", dir);
		ret = -EIO;
	}

	if (ret < 0) {
		return ret;
	}

	dev_data->active_dir = dir;
	dev_data->stop_with_draining = false;
	dev_data->trigger_drop = false;

	return 0;
}

static int renesas_ra_ssie_stop(const struct device *dev)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;

	if (dev_data->state != I2S_STATE_RUNNING) {
		return -EIO;
	}

#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream_next);
#endif

	dev_data->stop_with_draining = false;
	dev_data->trigger_drop = false;
	dev_data->state = I2S_STATE_STOPPING;

	return 0;
}

static int renesas_ra_trigger_drain(const struct device *dev, enum i2s_dir dir)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;

	if (dev_data->state != I2S_STATE_RUNNING) {
		return -EIO;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		dev_data->stop_with_draining =
			k_msgq_num_used_get(&dev_data->tx_queue) > 0 ? true : false;
	} else {
		/* I2S_DIR_RX */
		dev_data->stop_with_draining = false;
	}

	dev_data->state = I2S_STATE_STOPPING;
	dev_data->trigger_drop = false;

	return 0;
}

static int renesas_ra_trigger_drop(const struct device *dev, enum i2s_dir dir)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;
	fsp_err_t fsp_err;

	if (dev_data->state == I2S_STATE_NOT_READY) {
		return -EIO;
	}

	if (dev_data->state != I2S_STATE_READY) {
		fsp_err = R_SSI_Stop(&dev_data->fsp_ctrl);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to stop SSI, error: (%d)", fsp_err);
			return -EIO;
		}
	}

#ifdef CONFIG_I2S_RENESAS_RA_SSIE_RX_SECONDARY_BUFFER
	if (dir == I2S_DIR_BOTH || dir == I2S_DIR_RX) {
		i2s_renesas_ra_free_stream(&dev_data->rx_cfg, &dev_data->rx_stream_next);
	}
#endif

	if (dir == I2S_DIR_BOTH || dir == I2S_DIR_TX) {
		drop_queue(dev, I2S_DIR_TX);
	}

	if (dir == I2S_DIR_BOTH || dir == I2S_DIR_RX) {
		drop_queue(dev, I2S_DIR_RX);
	}

	dev_data->trigger_drop = true;

	return 0;
}

static int renesas_ra_trigger_prepare(const struct device *dev, enum i2s_dir dir)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;

	if (dev_data->state != I2S_STATE_ERROR) {
		return -EIO;
	}

	dev_data->state = I2S_STATE_READY;

	return 0;
}

#define I2S_SUPPORTED_OPTIONS                                                                      \
	(I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_CONTROLLER |    \
	 I2S_OPT_BIT_CLK_TARGET | I2S_OPT_BIT_CLK_CONT)

#define I2S_UNSUPPORTED_FORMATS (I2S_FMT_DATA_FORMAT_PCM_SHORT | I2S_FMT_DATA_FORMAT_PCM_LONG)

static int i2s_renesas_ra_ssie_configure(const struct device *dev, enum i2s_dir dir,
					 const struct i2s_config *i2s_cfg)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;
	i2s_cfg_t *fsp_cfg = &dev_data->fsp_cfg;
	uint32_t frame_size_bytes;
	i2s_pcm_width_t pcm_width;
	i2s_word_length_t word_length;
	i2s_mode_t operating_mode;
	fsp_err_t fsp_err;
	int ret;

	/* Supports only the I2S format */
	if ((i2s_cfg->format & I2S_UNSUPPORTED_FORMATS) != 0) {
		LOG_ERR("Unsupported format: 0x%02x", i2s_cfg->format);
		return -ENOTSUP;
	}

	/* I2S format using only MSB order */
	if (i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) {
		return -EINVAL;
	}

	if (i2s_cfg->format & I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED &&
	    i2s_cfg->format & I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED) {
		return -EINVAL;
	}

	if ((i2s_cfg->options & ~I2S_SUPPORTED_OPTIONS)) {
		LOG_ERR("Unsupported options: 0x%02x", i2s_cfg->options);
		return -ENOTSUP;
	}

	/* If the state is not in ready or not ready state, return ERROR */
	if (!(dev_data->state == I2S_STATE_READY || dev_data->state == I2S_STATE_NOT_READY)) {
		LOG_ERR("Cannot configure in state: %d", dev_data->state);
		return -EINVAL;
	}

	/* If the node do not only full duplex, return ERROR when configure I2S_DIR_BOTH */
	if (dev_data->full_duplex == false && dir == I2S_DIR_BOTH) {
		LOG_ERR("Cannot configure I2S_DIR_BOTH direction for half-duplex device");
		return -ENOSYS;
	}

	/* Module always generate bit clock although there is no data transferring */
	if (i2s_cfg->options & I2S_OPT_BIT_CLK_GATED) {
		LOG_ERR("Unsupported option: I2S_OPT_BIT_CLK_GATED");
		return -ENOTSUP;
	}

	/* If frame_clk_freq = 0, set the state to not ready
	 * Then drop all data in tx and rx queue
	 */
	if (i2s_cfg->frame_clk_freq == 0) {
		if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
			drop_queue(dev, I2S_DIR_TX);
			i2s_renesas_ra_release_stream(&dev_data->tx_stream);
			dev_data->tx_configured = false;
		}

		if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
			drop_queue(dev, I2S_DIR_RX);
			i2s_renesas_ra_release_stream(&dev_data->rx_stream);
			dev_data->rx_configured = false;
		}

		dev_data->state = I2S_STATE_NOT_READY;

		return 0;
	}

	if (i2s_cfg->channels != 2) {
		LOG_ERR("Unsupported number of channels: %u", i2s_cfg->channels);
		return -EINVAL;
	}

	if (i2s_cfg->mem_slab == NULL) {
		LOG_ERR("No memory block to store data");
		return -EINVAL;
	}

	if (i2s_cfg->block_size == 0) {
		LOG_ERR("Block size must be greater than 0");
		return -EINVAL;
	}

	switch (i2s_cfg->word_size) {
	case 8:
		pcm_width = I2S_PCM_WIDTH_8_BITS;
		word_length = I2S_WORD_LENGTH_8_BITS;
		frame_size_bytes = 2;
		break;
	case 16:
		pcm_width = I2S_PCM_WIDTH_16_BITS;
		word_length = I2S_WORD_LENGTH_16_BITS;
		frame_size_bytes = 4;
		break;
	case 24:
		pcm_width = I2S_PCM_WIDTH_24_BITS;
		word_length = I2S_WORD_LENGTH_24_BITS;
		frame_size_bytes = 8;
		break;
	case 32:
		pcm_width = I2S_PCM_WIDTH_32_BITS;
		word_length = I2S_WORD_LENGTH_32_BITS;
		frame_size_bytes = 8;
		break;
	default:
		LOG_ERR("Unsupported word size: %u", i2s_cfg->word_size);
		return -EINVAL;
	}

	if ((i2s_cfg->block_size % frame_size_bytes) != 0) {
		LOG_ERR("Block size must be multiple of frame size");
		return -EINVAL;
	}

	/*
	 * For master mode, bit clock and frame clock is generated from internal
	 * For slave mode, bit clock and frame clock is get from external
	 */
	if (i2s_cfg->options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) {
		operating_mode = I2S_MODE_SLAVE;
	} else {
		operating_mode = I2S_MODE_MASTER;
		ret = renesas_ra_ssie_set_clock_divider(dev, i2s_cfg);
		if (ret < 0) {
			return ret;
		}
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		dev_data->tx_cfg = *i2s_cfg;
		dev_data->tx_configured = true;
		if (dev_data->full_duplex == false) {
			dev_data->rx_configured = false;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		dev_data->rx_cfg = *i2s_cfg;
		dev_data->rx_configured = true;
		if (dev_data->full_duplex == false) {
			dev_data->tx_configured = false;
		}
	}

	fsp_err = R_SSI_Close(&dev_data->fsp_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to configure the device");
		return -EIO;
	}

#ifdef CONFIG_I2S_RENESAS_RA_SSIE_DTC
	fsp_cfg->p_transfer_tx = dev_data->tx_configured ? &dev_data->tx_transfer : NULL;
	fsp_cfg->p_transfer_rx = dev_data->rx_configured ? &dev_data->rx_transfer : NULL;
#endif
	fsp_cfg->pcm_width = pcm_width;
	fsp_cfg->word_length = word_length;
	fsp_cfg->operating_mode = operating_mode;
	fsp_cfg->p_extend = &dev_data->fsp_ext_cfg;

	fsp_err = R_SSI_Open(&dev_data->fsp_ctrl, fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to configure the device");
		return -EIO;
	}

	dev_data->state = I2S_STATE_READY;

	return 0;
}

static const struct i2s_config *i2s_renesas_ra_ssie_get_config(const struct device *dev,
							       enum i2s_dir dir)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;

	if (dir == I2S_DIR_TX) {
		return dev_data->tx_configured ? &dev_data->tx_cfg : NULL;
	}

	if (dir == I2S_DIR_RX) {
		return dev_data->rx_configured ? &dev_data->rx_cfg : NULL;
	}

	/* dir == I2S_DIR_BOTH */
	return dev_data->rx_configured ? &dev_data->rx_cfg
				       : (dev_data->tx_configured ? &dev_data->tx_cfg : NULL);
}

static int i2s_renesas_ra_ssie_write(const struct device *dev, void *mem_block, size_t size)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;
	struct renesas_ra_ssie_stream tx_stream = {.mem_block = mem_block, .mem_block_len = size};
	int ret;

	if (!dev_data->tx_configured) {
		LOG_ERR("Device is not configured");
		return -EIO;
	}

	if (dev_data->state != I2S_STATE_RUNNING && dev_data->state != I2S_STATE_READY) {
		LOG_ERR("Cannot write in state: %d", dev_data->state);
		return -EIO;
	}

	if (size > dev_data->tx_cfg.block_size) {
		LOG_ERR("This device can only write blocks up to %u bytes",
			dev_data->tx_cfg.block_size);
		return -EIO;
	}

	ret = i2s_renesas_ra_put_stream(&dev_data->tx_queue, &tx_stream);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int i2s_renesas_ra_ssie_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;
	struct renesas_ra_ssie_stream rx_stream;
	k_timeout_t timeout =
		(dev_data->state == I2S_STATE_ERROR) ? K_NO_WAIT : K_MSEC(dev_data->rx_cfg.timeout);
	int ret;

	if (!dev_data->rx_configured) {
		LOG_ERR("Device is not configured");
		return -EIO;
	}

	/* If read in not ready state, return ERROR */
	if (dev_data->state == I2S_STATE_NOT_READY) {
		LOG_ERR("RX invalid state: %d", (int)dev_data->state);
		return -EIO;
	}

	ret = i2s_renesas_ra_get_stream(&dev_data->rx_queue, &rx_stream, timeout);
	if (ret == -ENOMSG) {
		return -EIO;
	}

	if (ret == 0) {
		*mem_block = rx_stream.mem_block;
		*size = rx_stream.mem_block_len;
	}

	return ret;
}

static int i2s_renesas_ra_ssie_trigger(const struct device *dev, enum i2s_dir dir,
				       enum i2s_trigger_cmd cmd)
{
	struct renesas_ra_ssie_data *dev_data = dev->data;
	bool configured = false;
	int ret = 0;

	if (dir == I2S_DIR_BOTH) {
		if (dev_data->full_duplex == false) {
			LOG_ERR("I2S_DIR_BOTH is not supported for half-duplex device");
			return -ENOSYS;
		}

		configured = dev_data->tx_configured && dev_data->rx_configured;
	} else if (dir == I2S_DIR_TX) {
		configured = dev_data->tx_configured;
	} else if (dir == I2S_DIR_RX) {
		configured = dev_data->rx_configured;
	}

	if (!configured) {
		LOG_ERR("Device is not configured");
		return -EIO;
	}

	if (dev_data->state == I2S_STATE_RUNNING && dev_data->active_dir != dir) {
		LOG_ERR("Inappropriate trigger (%d/%d), active stream(s): %d", cmd, dir,
			dev_data->active_dir);
		return -EINVAL;
	}

	K_SPINLOCK(&dev_data->lock) {
		switch (cmd) {
		case I2S_TRIGGER_START:
			ret = renesas_ra_ssie_start_transfer(dev, dir);
			break;
		case I2S_TRIGGER_STOP:
			ret = renesas_ra_ssie_stop(dev);
			break;
		case I2S_TRIGGER_DRAIN:
			ret = renesas_ra_trigger_drain(dev, dir);
			break;
		case I2S_TRIGGER_DROP:
			ret = renesas_ra_trigger_drop(dev, dir);
			break;
		case I2S_TRIGGER_PREPARE:
			ret = renesas_ra_trigger_prepare(dev, dir);
			break;
		default:
			LOG_ERR("Invalid trigger: %d", cmd);
			ret = -EINVAL;
		}
	}

	return ret;
}

static int i2s_renesas_ra_ssie_init(const struct device *dev)
{
	const struct renesas_ra_ssie_config *config = dev->config;
	struct renesas_ra_ssie_data *dev_data = dev->data;
	fsp_err_t fsp_err;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to start ssie bus clock, err=%d", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s: pinctrl config failed.", __func__);
		return ret;
	}

	k_msgq_init(&dev_data->tx_queue, (char *)dev_data->tx_msgs,
		    sizeof(struct renesas_ra_ssie_stream),
		    CONFIG_I2S_RENESAS_RA_SSIE_TX_BLOCK_COUNT);
	k_msgq_init(&dev_data->rx_queue, (char *)dev_data->rx_msgs,
		    sizeof(struct renesas_ra_ssie_stream),
		    CONFIG_I2S_RENESAS_RA_SSIE_RX_BLOCK_COUNT);

	fsp_err = R_SSI_Open(&dev_data->fsp_ctrl, &dev_data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to initialize the device");
		return -EIO;
	}

	config->irq_config_func(dev);

	ret = audio_clock_enable(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(i2s, i2s_renesas_ra_drv_api) = {
	.configure = i2s_renesas_ra_ssie_configure,
	.config_get = i2s_renesas_ra_ssie_get_config,
	.read = i2s_renesas_ra_ssie_read,
	.write = i2s_renesas_ra_ssie_write,
	.trigger = i2s_renesas_ra_ssie_trigger,
};

#define EVENT_SSI_INT(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SSI, channel, _INT))
#define EVENT_SSI_TXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SSI, channel, _TXI))
#define EVENT_SSI_RXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SSI, channel, _RXI))

#define SSIE_RA_HALF_DUPLEX_INIT(index)                                                            \
	R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, ssi_rt, irq)] =                                    \
		EVENT_SSI_TXI(DT_INST_PROP(index, channel));                                       \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, ssi_rt, irq),                                       \
		    DT_INST_IRQ_BY_NAME(index, ssi_rt, priority), ssi_rt_isr,                      \
		    DEVICE_DT_INST_GET(index), 0);                                                 \
	irq_enable(DT_INST_IRQ_BY_NAME(index, ssi_rt, irq));

#define SSIE_RA_FULL_DUPLEX_INIT(index)                                                            \
	R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, ssi_txi, irq)] =                                   \
		EVENT_SSI_TXI(DT_INST_PROP(index, channel));                                       \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, ssi_txi, irq),                                      \
		    DT_INST_IRQ_BY_NAME(index, ssi_txi, priority), ssi_txi_isr, (NULL), 0);        \
	irq_enable(DT_INST_IRQ_BY_NAME(index, ssi_txi, irq));                                      \
                                                                                                   \
	R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, ssi_rxi, irq)] =                                   \
		EVENT_SSI_RXI(DT_INST_PROP(index, channel));                                       \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, ssi_rxi, irq),                                      \
		    DT_INST_IRQ_BY_NAME(index, ssi_rxi, priority), ssi_rxi_isr, (NULL), 0);        \
	irq_enable(DT_INST_IRQ_BY_NAME(index, ssi_rxi, irq));

#define RA_SSIE_RX_IRQ_BY_NAME(index, prop)                                                        \
	COND_CODE_1(DT_INST_PROP(index, full_duplex), (DT_INST_IRQ_BY_NAME(index, ssi_rxi, prop)), \
		(DT_INST_IRQ_BY_NAME(index, ssi_rt, prop)))

#define RA_SSIE_TX_IRQ_BY_NAME(index, prop)                                                        \
	COND_CODE_1(DT_INST_PROP(index, full_duplex), (DT_INST_IRQ_BY_NAME(index, ssi_txi, prop)), \
		(DT_INST_IRQ_BY_NAME(index, ssi_rt, prop)))

#ifndef CONFIG_I2S_RENESAS_RA_SSIE_DTC
#define SSIE_DTC_INIT(index)
#else
#define SSIE_DTC_TX_SOURCE(index)                                                                  \
	COND_CODE_1(DT_INST_PROP(index, full_duplex), (DT_INST_IRQ_BY_NAME(index, ssi_txi, irq)),  \
		(DT_INST_IRQ_BY_NAME(index, ssi_rt, irq)))

#define SSIE_DTC_RX_SOURCE(index)                                                                  \
	COND_CODE_1(DT_INST_PROP(index, full_duplex), (DT_INST_IRQ_BY_NAME(index, ssi_rxi, irq)),  \
		(DT_INST_IRQ_BY_NAME(index, ssi_rt, irq)))

#define SSIE_DTC_INIT(index)                                                                       \
	.tx_transfer_info =                                                                        \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,       \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,       \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                          \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,  \
			.transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 0,                                                               \
	},                                                                                         \
	.tx_transfer_cfg_extend = {.activation_source = SSIE_DTC_TX_SOURCE(index)},                \
	.tx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &renesas_ra_ssie_data_##index.tx_transfer_info,                  \
			.p_extend = &renesas_ra_ssie_data_##index.tx_transfer_cfg_extend,          \
	},                                                                                         \
	.tx_transfer =                                                                             \
		{                                                                                  \
			.p_ctrl = &renesas_ra_ssie_data_##index.tx_transfer_ctrl,                  \
			.p_cfg = &renesas_ra_ssie_data_##index.tx_transfer_cfg,                    \
			.p_api = &g_transfer_on_dtc,                                               \
	},                                                                                         \
	.rx_transfer_info =                                                                        \
		{                                                                                  \
			.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED, \
			.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,  \
			.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                          \
			.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,       \
			.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,        \
			.transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE,                     \
			.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                     \
			.p_dest = (void *)NULL,                                                    \
			.p_src = (void const *)NULL,                                               \
			.num_blocks = 0,                                                           \
			.length = 0,                                                               \
	},                                                                                         \
	.rx_transfer_cfg_extend =                                                                  \
		{                                                                                  \
			.activation_source = SSIE_DTC_RX_SOURCE(index),                            \
	},                                                                                         \
	.rx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = &renesas_ra_ssie_data_##index.rx_transfer_info,                  \
			.p_extend = &renesas_ra_ssie_data_##index.rx_transfer_cfg_extend,          \
	},                                                                                         \
	.rx_transfer = {                                                                           \
		.p_ctrl = &renesas_ra_ssie_data_##index.rx_transfer_ctrl,                          \
		.p_cfg = &renesas_ra_ssie_data_##index.rx_transfer_cfg,                            \
		.p_api = &g_transfer_on_dtc,                                                       \
	},
#endif

#define RENESAS_RA_SSIE_CLOCK_SOURCE(index)                                                        \
	COND_CODE_1(                                                                               \
		DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR_BY_NAME(index, audio_clock), pwm_clock),    \
		(SSI_AUDIO_CLOCK_INTERNAL), (SSI_AUDIO_CLOCK_EXTERNAL))

#define SSIE_RA_IRQ_INIT(index)                                                                    \
	COND_CODE_1(DT_INST_PROP(index, full_duplex), (SSIE_RA_FULL_DUPLEX_INIT(index)),           \
	(SSIE_RA_HALF_DUPLEX_INIT(index)))

#define SSIE_RA_INIT(index)                                                                        \
	static void renesas_ra_i2s_ssie_irq_config_func##index(const struct device *dev)           \
	{                                                                                          \
		SSIE_RA_IRQ_INIT(index)                                                            \
                                                                                                   \
		/* ssi_if */                                                                       \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, ssi_if, irq)] =                            \
			EVENT_SSI_INT(DT_INST_PROP(index, channel));                               \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, ssi_if, irq),                               \
			    DT_INST_IRQ_BY_NAME(index, ssi_if, priority), ssi_int_isr, (NULL), 0); \
		irq_enable(DT_INST_IRQ_BY_NAME(index, ssi_if, irq));                               \
	}                                                                                          \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static struct renesas_ra_ssie_config renesas_ra_ssie_config_##index = {                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_config_func = renesas_ra_i2s_ssie_irq_config_func##index,                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, mstp),  \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, stop_bit),    \
			},                                                                         \
		.audio_clock_dev =                                                                 \
			DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR_BY_NAME(index, audio_clock)),    \
	};                                                                                         \
	static const ssi_extended_cfg_t ssi_extended_cfg_t_##index = {                             \
		.audio_clock = RENESAS_RA_SSIE_CLOCK_SOURCE(index),                                \
		.bit_clock_div = SSI_CLOCK_DIV_1,                                                  \
	};                                                                                         \
	static struct renesas_ra_ssie_data renesas_ra_ssie_data_##index = {                        \
		.state = I2S_STATE_NOT_READY,                                                      \
		.stop_with_draining = false,                                                       \
		.trigger_drop = false,                                                             \
		.full_duplex = DT_INST_PROP(index, full_duplex) ? true : false,                    \
		.fsp_ext_cfg = ssi_extended_cfg_t_##index,                                         \
		.fsp_cfg = {.channel = DT_INST_PROP(index, channel),                               \
			    .operating_mode = I2S_MODE_MASTER,                                     \
			    .pcm_width = I2S_PCM_WIDTH_16_BITS,                                    \
			    .word_length = I2S_WORD_LENGTH_16_BITS,                                \
			    .ws_continue = I2S_WS_CONTINUE_OFF,                                    \
			    .p_callback = renesas_ra_ssie_callback,                                \
			    .p_context = (void *)DEVICE_DT_INST_GET(index),                        \
			    .p_extend = &ssi_extended_cfg_t_##index,                               \
			    .txi_irq = RA_SSIE_TX_IRQ_BY_NAME(index, irq),                         \
			    .rxi_irq = RA_SSIE_RX_IRQ_BY_NAME(index, irq),                         \
			    .int_irq = DT_INST_IRQ_BY_NAME(index, ssi_if, irq),                    \
			    .txi_ipl = RA_SSIE_TX_IRQ_BY_NAME(index, priority),                    \
			    .rxi_ipl = RA_SSIE_RX_IRQ_BY_NAME(index, priority),                    \
			    .idle_err_ipl = DT_INST_IRQ_BY_NAME(index, ssi_if, priority),          \
			    .p_transfer_tx = NULL,                                                 \
			    .p_transfer_rx = NULL},                                                \
		SSIE_DTC_INIT(index)};                                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i2s_renesas_ra_ssie_init, NULL,                              \
			      &renesas_ra_ssie_data_##index, &renesas_ra_ssie_config_##index,      \
			      POST_KERNEL, CONFIG_I2S_INIT_PRIORITY, &i2s_renesas_ra_drv_api);

DT_INST_FOREACH_STATUS_OKAY(SSIE_RA_INIT)
