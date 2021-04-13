/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <drivers/i2s.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <nrfx_i2s.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2s_nrfx, CONFIG_I2S_LOG_LEVEL);

struct stream_cfg {
	struct i2s_config cfg;
	nrfx_i2s_config_t nrfx_cfg;
};

struct i2s_nrfx_drv_data {
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;
	struct stream_cfg tx;
	struct k_msgq tx_queue;
	struct stream_cfg rx;
	struct k_msgq rx_queue;
	const uint32_t *last_tx_buffer;
	enum i2s_state state;
	enum i2s_dir active_dir;
	bool tx_configured : 1;
	bool rx_configured : 1;
	bool stop          : 1; /* stop after the current (TX or RX) block */
	bool discard_rx    : 1; /* discard further RX blocks */
	bool request_clock : 1;
	volatile bool next_tx_buffer_needed;
};

struct i2s_nrfx_drv_cfg {
	nrfx_i2s_data_handler_t data_handler;
	nrfx_i2s_config_t nrfx_def_cfg;
	enum clock_source {
		PCLK32M,
		PCLK32M_HFXO,
		ACLK
	} clk_src;
};

/* Finds the clock settings that give the frame clock frequency closest to
 * the one requested, taking into account the hardware limitations.
 */
static void find_suitable_clock(const struct i2s_nrfx_drv_cfg *drv_cfg,
				nrfx_i2s_config_t *config,
				const struct i2s_config *i2s_cfg)
{
	static const struct {
		uint16_t        ratio_val;
		nrf_i2s_ratio_t ratio_enum;
	} ratios[] = {
		{  32, NRF_I2S_RATIO_32X },
		{  48, NRF_I2S_RATIO_48X },
		{  64, NRF_I2S_RATIO_64X },
		{  96, NRF_I2S_RATIO_96X },
		{ 128, NRF_I2S_RATIO_128X },
		{ 192, NRF_I2S_RATIO_192X },
		{ 256, NRF_I2S_RATIO_256X },
		{ 384, NRF_I2S_RATIO_384X },
		{ 512, NRF_I2S_RATIO_512X }
	};
	const uint32_t src_freq =
		(NRF_I2S_HAS_CLKCONFIG && drv_cfg->clk_src == ACLK)
		/* The I2S_NRFX_DEVICE() macro contains build assertions that
		 * make sure that the ACLK clock source is only used when it is
		 * available and only with the "hfclkaudio-frequency" property
		 * defined, but the default value of 0 here needs to be used to
		 * prevent compilation errors when the property is not defined
		 * (this expression will be eventually optimized away then).
		 */
		? DT_PROP_OR(DT_NODELABEL(clock), hfclkaudio_frequency, 0)
		: 32*1000*1000UL;
	uint32_t bits_per_frame = 2 * i2s_cfg->word_size;
	uint32_t best_diff = UINT32_MAX;
	uint8_t r, best_r = 0;
	nrf_i2s_mck_t best_mck_cfg = 0;

	for (r = 0; r < ARRAY_SIZE(ratios); ++r) {
		/* Only multiples of the frame width can be used as ratios. */
		if ((ratios[r].ratio_val % bits_per_frame) != 0) {
			continue;
		}

		if (IS_ENABLED(CONFIG_SOC_SERIES_NRF53X)) {
			uint32_t requested_mck =
				i2s_cfg->frame_clk_freq * ratios[r].ratio_val;
			/* As specified in the nRF5340 PS:
			 *
			 * MCKFREQ = 4096 * floor(f_MCK * 1048576 /
			 *                        (f_source + f_MCK / 2))
			 * f_actual = f_source /
			 *            floor(1048576 * 4096 / MCKFREQ)
			 */
			uint32_t mck_factor =
				(uint32_t)((requested_mck * 1048576ULL) /
					   (src_freq + requested_mck / 2));
			uint32_t actual_mck = src_freq / (1048576 / mck_factor);

			uint32_t lrck_freq = actual_mck / ratios[r].ratio_val;
			uint32_t diff = lrck_freq >= i2s_cfg->frame_clk_freq
					? (lrck_freq - i2s_cfg->frame_clk_freq)
					: (i2s_cfg->frame_clk_freq - lrck_freq);

			if (diff < best_diff) {
				best_mck_cfg = mck_factor * 4096;
				best_r = r;
				/* Stop if an exact match is found. */
				if (diff == 0) {
					break;
				}

				best_diff = diff;
			}
		} else {
			static const struct {
				uint8_t       divider_val;
				nrf_i2s_mck_t divider_enum;
			} dividers[] = {
				{   8, NRF_I2S_MCK_32MDIV8 },
				{  10, NRF_I2S_MCK_32MDIV10 },
				{  11, NRF_I2S_MCK_32MDIV11 },
				{  15, NRF_I2S_MCK_32MDIV15 },
				{  16, NRF_I2S_MCK_32MDIV16 },
				{  21, NRF_I2S_MCK_32MDIV21 },
				{  23, NRF_I2S_MCK_32MDIV23 },
				{  30, NRF_I2S_MCK_32MDIV30 },
				{  31, NRF_I2S_MCK_32MDIV31 },
				{  32, NRF_I2S_MCK_32MDIV32 },
				{  42, NRF_I2S_MCK_32MDIV42 },
				{  63, NRF_I2S_MCK_32MDIV63 },
				{ 125, NRF_I2S_MCK_32MDIV125 }
			};

			for (uint8_t d = 0; d < ARRAY_SIZE(dividers); ++d) {
				uint32_t mck_freq =
					src_freq / dividers[d].divider_val;
				uint32_t lrck_freq =
					mck_freq / ratios[r].ratio_val;
				uint32_t diff =
					lrck_freq >= i2s_cfg->frame_clk_freq
					? (lrck_freq - i2s_cfg->frame_clk_freq)
					: (i2s_cfg->frame_clk_freq - lrck_freq);

				if (diff < best_diff) {
					best_mck_cfg = dividers[d].divider_enum;
					best_r = r;
					/* Stop if an exact match is found. */
					if (diff == 0) {
						break;
					}

					best_diff = diff;
				}

				/* Since dividers are in ascending order, stop
				 * checking next ones for the current ratio
				 * after resulting LRCK frequency falls below
				 * the one requested.
				 */
				if (lrck_freq < i2s_cfg->frame_clk_freq) {
					break;
				}
			}
		}
	}

	config->mck_setup = best_mck_cfg;
	config->ratio = ratios[best_r].ratio_enum;
}

static bool get_next_tx_buffer(struct i2s_nrfx_drv_data *drv_data,
			       nrfx_i2s_buffers_t *buffers)
{
	int ret = k_msgq_get(&drv_data->tx_queue,
			     &buffers->p_tx_buffer,
			     K_NO_WAIT);
	return (ret == 0);
}

static bool get_next_rx_buffer(struct i2s_nrfx_drv_data *drv_data,
			       nrfx_i2s_buffers_t *buffers)
{
	int ret = k_mem_slab_alloc(drv_data->rx.cfg.mem_slab,
				   (void **)&buffers->p_rx_buffer,
				   K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to allocate next RX buffer: %d",
			ret);
		return false;
	}

	return true;
}

static void free_tx_buffer(struct i2s_nrfx_drv_data *drv_data,
			   const void *buffer)
{
	k_mem_slab_free(drv_data->tx.cfg.mem_slab, (void **)&buffer);
	LOG_DBG("Freed TX %p", buffer);
}

static void free_rx_buffer(struct i2s_nrfx_drv_data *drv_data, void *buffer)
{
	k_mem_slab_free(drv_data->rx.cfg.mem_slab, &buffer);
	LOG_DBG("Freed RX %p", buffer);
}

static bool supply_next_buffers(struct i2s_nrfx_drv_data *drv_data,
				nrfx_i2s_buffers_t *next)
{
	drv_data->last_tx_buffer = next->p_tx_buffer;

	if (drv_data->active_dir != I2S_DIR_TX) { /* -> RX active */
		if (!get_next_rx_buffer(drv_data, next)) {
			drv_data->state = I2S_STATE_ERROR;
			nrfx_i2s_stop();
			return false;
		}
	}

	LOG_DBG("Next buffers: %p/%p", next->p_tx_buffer, next->p_rx_buffer);
	nrfx_i2s_next_buffers_set(next);
	return true;
}

static void data_handler(const struct device *dev,
			 const nrfx_i2s_buffers_t *released, uint32_t status)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;
	bool stop_transfer = false;

	if (status & NRFX_I2S_STATUS_TRANSFER_STOPPED) {
		if (drv_data->state == I2S_STATE_STOPPING) {
			drv_data->state = I2S_STATE_READY;
		}
		if (drv_data->last_tx_buffer) {
			/* Usually, these pointers are equal, i.e. the last TX
			 * buffer that were to be transferred is released by the
			 * driver after it stops. The last TX buffer pointer is
			 * then set to NULL here so that the buffer can be freed
			 * below, just as any other TX buffer released by the
			 * driver. However, it may happen that the buffer is not
			 * released this way, for example, when the transfer
			 * ends with an error because an RX buffer allocation
			 * fails. In such case, the last TX buffer needs to be
			 * freed here.
			 */
			if (drv_data->last_tx_buffer != released->p_tx_buffer) {
				free_tx_buffer(drv_data,
					       drv_data->last_tx_buffer);
			}
			drv_data->last_tx_buffer = NULL;
		}
		nrfx_i2s_uninit();
		if (drv_data->request_clock) {
			(void)onoff_release(drv_data->clk_mgr);
		}
	}

	if (released == NULL) {
		/* This means that buffers for the next part of the transfer
		 * were not supplied and the previous ones cannot be released
		 * yet, as pointers to them were latched in the I2S registers.
		 * It is not an error when the transfer is to be stopped (those
		 * buffers will be released after the transfer actually stops).
		 */
		if (drv_data->state != I2S_STATE_STOPPING) {
			LOG_ERR("Next buffers not supplied on time");
			drv_data->state = I2S_STATE_ERROR;
		}
		nrfx_i2s_stop();
		return;
	}

	if (released->p_rx_buffer) {
		if (drv_data->discard_rx) {
			free_rx_buffer(drv_data, released->p_rx_buffer);
		} else {
			int ret = k_msgq_put(&drv_data->rx_queue,
					     &released->p_rx_buffer,
					     K_NO_WAIT);
			if (ret < 0) {
				LOG_ERR("No room in RX queue");
				drv_data->state = I2S_STATE_ERROR;
				stop_transfer = true;

				free_rx_buffer(drv_data, released->p_rx_buffer);
			} else {
				LOG_DBG("Queued RX %p", released->p_rx_buffer);

				/* If the TX direction is not active and
				 * the transfer should be stopped after
				 * the current block, stop the reception.
				 */
				if (drv_data->active_dir == I2S_DIR_RX &&
				    drv_data->stop) {
					drv_data->discard_rx = true;
					stop_transfer = true;
				}
			}
		}
	}

	if (released->p_tx_buffer) {
		/* If the last buffer that was to be transferred has just been
		 * released, it is time to stop the transfer.
		 */
		if (released->p_tx_buffer == drv_data->last_tx_buffer) {
			drv_data->discard_rx = true;
			stop_transfer = true;
		} else {
			free_tx_buffer(drv_data, released->p_tx_buffer);
		}
	}

	if (stop_transfer) {
		nrfx_i2s_stop();
	} else if (status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) {
		nrfx_i2s_buffers_t next = { 0 };

		if (drv_data->active_dir != I2S_DIR_RX) { /* -> TX active */
			if (drv_data->stop) {
				/* If the stream is to be stopped, don't get
				 * the next TX buffer from the queue, instead
				 * supply the one used last time (it won't be
				 * transferred, the stream will stop right
				 * before this buffer would be started again).
				 */
				next.p_tx_buffer = drv_data->last_tx_buffer;
			} else if (get_next_tx_buffer(drv_data, &next)) {
				/* Next TX buffer successfully retrieved from
				 * the queue, nothing more to do here.
				 */
			} else if (drv_data->state == I2S_STATE_STOPPING) {
				/* If there are no more TX blocks queued and
				 * the current state is STOPPING (so the DRAIN
				 * command was triggered) it is time to finish
				 * the transfer.
				 */
				drv_data->stop = true;
				/* Supply the same buffer as last time; it will
				 * not be transferred anyway, as the transfer
				 * will be stopped earlier.
				 */
				next.p_tx_buffer = drv_data->last_tx_buffer;
			} else {
				/* Next TX buffer cannot be supplied now.
				 * Defer it to when the user writes more data.
				 */
				drv_data->next_tx_buffer_needed = true;
				return;
			}
		}

		(void)supply_next_buffers(drv_data, &next);
	}
}

static void purge_queue(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;
	void *mem_block;

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		while (k_msgq_get(&drv_data->tx_queue,
				  &mem_block,
				  K_NO_WAIT) == 0) {
			free_tx_buffer(drv_data, mem_block);
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		while (k_msgq_get(&drv_data->rx_queue,
				  &mem_block,
				  K_NO_WAIT) == 0) {
			free_rx_buffer(drv_data, mem_block);
		}
	}
}

static int i2s_nrfx_configure(const struct device *dev, enum i2s_dir dir,
			      const struct i2s_config *i2s_cfg)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;
	const struct i2s_nrfx_drv_cfg *drv_cfg = dev->config;
	nrfx_i2s_config_t nrfx_cfg;

	if (drv_data->state != I2S_STATE_READY) {
		LOG_ERR("Cannot configure in state: %d", drv_data->state);
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq == 0) { /* -> reset state */
		purge_queue(dev, dir);
		if (dir == I2S_DIR_TX || I2S_DIR_BOTH) {
			drv_data->tx_configured = false;
			memset(&drv_data->tx, 0, sizeof(drv_data->tx));
		}
		if (dir == I2S_DIR_RX || I2S_DIR_BOTH) {
			drv_data->rx_configured = false;
			memset(&drv_data->rx, 0, sizeof(drv_data->rx));
		}
		return 0;
	}

	__ASSERT_NO_MSG(i2s_cfg->mem_slab != NULL &&
			i2s_cfg->block_size != 0);

	if ((i2s_cfg->block_size % sizeof(uint32_t)) != 0) {
		LOG_ERR("This device can transfer only full 32-bit words");
		return -EINVAL;
	}

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

	if (i2s_cfg->channels == 2 ||
	    (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK)
		== I2S_FMT_DATA_FORMAT_I2S) {
		nrfx_cfg.channels = NRF_I2S_CHANNELS_STEREO;
	} else if (i2s_cfg->channels == 1) {
		nrfx_cfg.channels = NRF_I2S_CHANNELS_LEFT;
	} else {
		LOG_ERR("Unsupported number of channels: %u",
			i2s_cfg->channels);
		return -EINVAL;
	}

	if ((i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) &&
	    (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE)) {
		nrfx_cfg.mode = NRF_I2S_MODE_SLAVE;
	} else if (!(i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) &&
		   !(i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE)) {
		nrfx_cfg.mode = NRF_I2S_MODE_MASTER;
	} else {
		LOG_ERR("Unsupported operation mode: 0x%02x", i2s_cfg->options);
		return -EINVAL;
	}

	/* If the master clock generator is needed (i.e. in Master mode or when
	 * the MCK output is used), find a suitable clock configuration for it.
	 */
	if (nrfx_cfg.mode == NRF_I2S_MODE_MASTER ||
	    nrfx_cfg.mck_pin != NRFX_I2S_PIN_NOT_USED) {
		find_suitable_clock(drv_cfg, &nrfx_cfg, i2s_cfg);
		/* Unless the PCLK32M source is used with the HFINT oscillator
		 * (which is always available without any additional actions),
		 * it is required to request the proper clock to be running
		 * before starting the transfer itself.
		 */
		drv_data->request_clock = (drv_cfg->clk_src != PCLK32M);
	} else {
		nrfx_cfg.mck_setup = NRF_I2S_MCK_DISABLED;
		drv_data->request_clock = false;
	}

	if ((i2s_cfg->options & I2S_OPT_LOOPBACK) ||
	    (i2s_cfg->options & I2S_OPT_PINGPONG)) {
		LOG_ERR("Unsupported options: 0x%02x", i2s_cfg->options);
		return -EINVAL;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		drv_data->tx.cfg = *i2s_cfg;
		drv_data->tx.nrfx_cfg = nrfx_cfg;
		drv_data->tx_configured = true;
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		drv_data->rx.cfg = *i2s_cfg;
		drv_data->rx.nrfx_cfg = nrfx_cfg;
		drv_data->rx_configured = true;
	}

	return 0;
}

static const struct i2s_config *i2s_nrfx_config_get(const struct device *dev,
						    enum i2s_dir dir)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;

	if (dir == I2S_DIR_TX && drv_data->tx_configured) {
		return &drv_data->tx.cfg;
	}
	if (dir == I2S_DIR_RX && drv_data->rx_configured) {
		return &drv_data->rx.cfg;
	}

	return NULL;
}

static int i2s_nrfx_read(const struct device *dev,
			 void **mem_block, size_t *size)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;
	int ret;

	if (!drv_data->rx_configured) {
		LOG_ERR("Device is not configured");
		return -EIO;
	}

	ret = k_msgq_get(&drv_data->rx_queue,
			 mem_block,
			 (drv_data->state == I2S_STATE_ERROR)
				? K_NO_WAIT
				: SYS_TIMEOUT_MS(drv_data->rx.cfg.timeout));
	if (ret == -ENOMSG) {
		return -EIO;
	}

	LOG_DBG("Released RX %p", *mem_block);

	if (ret == 0) {
		*size = drv_data->rx.cfg.block_size;
	}

	return ret;
}

static int i2s_nrfx_write(const struct device *dev,
			  void *mem_block, size_t size)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;

	if (!drv_data->tx_configured) {
		LOG_ERR("Device is not configured");
		return -EIO;
	}

	if (drv_data->state != I2S_STATE_RUNNING &&
	    drv_data->state != I2S_STATE_READY) {
		LOG_ERR("Cannot write in state: %d", drv_data->state);
		return -EIO;
	}

	if (size != drv_data->tx.cfg.block_size) {
		LOG_ERR("This device can only write blocks of %u bytes",
			drv_data->tx.cfg.block_size);
		return -EIO;
	}

	if (drv_data->state == I2S_STATE_RUNNING &&
	    drv_data->next_tx_buffer_needed) {
		nrfx_i2s_buffers_t next = { .p_tx_buffer = mem_block };

		drv_data->next_tx_buffer_needed = false;

		LOG_DBG("Next TX %p", mem_block);

		if (!supply_next_buffers(drv_data, &next)) {
			return -EIO;
		}
	} else {
		int ret = k_msgq_put(&drv_data->tx_queue,
				     &mem_block,
				     SYS_TIMEOUT_MS(drv_data->tx.cfg.timeout));
		if (ret < 0) {
			return ret;
		}

		LOG_DBG("Queued TX %p", mem_block);
	}

	return 0;
}

static int start_transfer(struct i2s_nrfx_drv_data *drv_data)
{
	nrfx_i2s_buffers_t initial_buffers = { 0 };
	int ret;

	if (drv_data->active_dir != I2S_DIR_RX && /* -> TX to be started */
	    !get_next_tx_buffer(drv_data, &initial_buffers)) {
		LOG_ERR("No TX buffer available");
		ret = -ENOMEM;
	} else if (drv_data->active_dir != I2S_DIR_TX && /* -> RX to be started */
		   !get_next_rx_buffer(drv_data, &initial_buffers)) {
		/* Failed to allocate next RX buffer */
		ret = -ENOMEM;
	} else {
		uint32_t block_size = (drv_data->active_dir == I2S_DIR_TX)
				      ? drv_data->tx.cfg.block_size
				      : drv_data->rx.cfg.block_size;
		nrfx_err_t err;

		drv_data->last_tx_buffer = initial_buffers.p_tx_buffer;

		err = nrfx_i2s_start(&initial_buffers,
				     block_size / sizeof(uint32_t), 0);
		if (err == NRFX_SUCCESS) {
			return 0;
		}

		LOG_ERR("Failed to start I2S transfer: 0x%08x", err);
		ret = -EIO;
	}

	nrfx_i2s_uninit();
	if (drv_data->request_clock) {
		(void)onoff_release(drv_data->clk_mgr);
	}

	if (initial_buffers.p_tx_buffer) {
		free_tx_buffer(drv_data, initial_buffers.p_tx_buffer);
	}
	if (initial_buffers.p_rx_buffer) {
		free_rx_buffer(drv_data, initial_buffers.p_rx_buffer);
	}

	drv_data->state = I2S_STATE_ERROR;
	return ret;
}

static void clock_started_callback(struct onoff_manager *mgr,
				   struct onoff_client *cli,
				   uint32_t state,
				   int res)
{
	struct i2s_nrfx_drv_data *drv_data =
		CONTAINER_OF(cli, struct i2s_nrfx_drv_data, clk_cli);

	/* The driver state can be set back to READY at this point if the DROP
	 * command was triggered before the clock has started. Do not start
	 * the actual transfer in such case.
	 */
	if (drv_data->state == I2S_STATE_READY) {
		nrfx_i2s_uninit();
		(void)onoff_release(drv_data->clk_mgr);
	} else {
		(void)start_transfer(drv_data);
	}
}

static int trigger_start(const struct device *dev)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;
	const struct i2s_nrfx_drv_cfg *drv_cfg = dev->config;
	nrfx_err_t err;
	int ret;
	const nrfx_i2s_config_t *nrfx_cfg = (drv_data->active_dir == I2S_DIR_TX)
					    ? &drv_data->tx.nrfx_cfg
					    : &drv_data->rx.nrfx_cfg;

	err = nrfx_i2s_init(nrfx_cfg, drv_cfg->data_handler);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize I2S: 0x%08x", err);
		return -EIO;
	}

	drv_data->state = I2S_STATE_RUNNING;

#if NRF_I2S_HAS_CLKCONFIG
	nrf_i2s_clk_configure(NRF_I2S0,
			      drv_cfg->clk_src == ACLK ? NRF_I2S_CLKSRC_ACLK
						       : NRF_I2S_CLKSRC_PCLK32M,
			      false);
#endif

	/* If it is required to use certain HF clock, request it to be running
	 * first. If not, start the transfer directly.
	 */
	if (drv_data->request_clock) {
		sys_notify_init_callback(&drv_data->clk_cli.notify,
					 clock_started_callback);
		ret = onoff_request(drv_data->clk_mgr, &drv_data->clk_cli);
		if (ret < 0) {
			nrfx_i2s_uninit();
			drv_data->state = I2S_STATE_READY;

			LOG_ERR("Failed to request clock: %d", ret);
			return -EIO;
		}
	} else {
		ret = start_transfer(drv_data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int i2s_nrfx_trigger(const struct device *dev,
			    enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;
	bool configured = false;
	bool cmd_allowed;

	/* This driver does not use the I2S_STATE_NOT_READY value.
	 * Instead, if a given stream is not configured, the respective
	 * flag (tx_configured or rx_configured) is cleared.
	 */
	if (dir == I2S_DIR_BOTH) {
		configured = drv_data->tx_configured && drv_data->rx_configured;
	} else if (dir == I2S_DIR_TX) {
		configured = drv_data->tx_configured;
	} else if (dir == I2S_DIR_RX) {
		configured = drv_data->rx_configured;
	}

	if (!configured) {
		LOG_ERR("Device is not configured");
		return -EIO;
	}

	if (dir == I2S_DIR_BOTH &&
	    (memcmp(&drv_data->tx.nrfx_cfg,
		    &drv_data->rx.nrfx_cfg,
		    sizeof(drv_data->rx.nrfx_cfg)) != 0
	     ||
	     (drv_data->tx.cfg.block_size != drv_data->rx.cfg.block_size))) {
		LOG_ERR("TX and RX configurations are different");
		return -EIO;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		cmd_allowed = (drv_data->state == I2S_STATE_READY);
		break;
	case I2S_TRIGGER_STOP:
	case I2S_TRIGGER_DRAIN:
		cmd_allowed = (drv_data->state == I2S_STATE_RUNNING);
		break;
	case I2S_TRIGGER_DROP:
		cmd_allowed = configured;
		break;
	case I2S_TRIGGER_PREPARE:
		cmd_allowed = (drv_data->state == I2S_STATE_ERROR);
		break;
	default:
		LOG_ERR("Invalid trigger: %d", cmd);
		return -EINVAL;
	}

	if (!cmd_allowed) {
		return -EIO;
	}

	/* For triggers applicable to the RUNNING state (i.e. STOP, DRAIN,
	 * and DROP), ensure that the command is applied to the streams
	 * that are currently active (this device cannot e.g. stop only TX
	 * without stopping RX).
	 */
	if (drv_data->state == I2S_STATE_RUNNING &&
	    drv_data->active_dir != dir) {
		LOG_ERR("Inappropriate trigger (%d/%d), active stream(s): %d",
			cmd, dir, drv_data->active_dir);
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		drv_data->stop = false;
		drv_data->discard_rx = false;
		drv_data->active_dir = dir;
		drv_data->next_tx_buffer_needed = false;
		return trigger_start(dev);

	case I2S_TRIGGER_STOP:
		drv_data->state = I2S_STATE_STOPPING;
		drv_data->stop = true;
		return 0;

	case I2S_TRIGGER_DRAIN:
		drv_data->state = I2S_STATE_STOPPING;
		/* If only RX is active, DRAIN is equivalent to STOP. */
		drv_data->stop = (drv_data->active_dir == I2S_DIR_RX);
		return 0;

	case I2S_TRIGGER_DROP:
		if (drv_data->state != I2S_STATE_READY) {
			drv_data->discard_rx = true;
			nrfx_i2s_stop();
		}
		purge_queue(dev, dir);
		drv_data->state = I2S_STATE_READY;
		return 0;

	case I2S_TRIGGER_PREPARE:
		purge_queue(dev, dir);
		drv_data->state = I2S_STATE_READY;
		return 0;

	default:
		LOG_ERR("Invalid trigger: %d", cmd);
		return -EINVAL;
	}
}

static void init_clock_manager(const struct device *dev)
{
	struct i2s_nrfx_drv_data *drv_data = dev->data;
	clock_control_subsys_t subsys;

#if NRF_CLOCK_HAS_HFCLKAUDIO
	const struct i2s_nrfx_drv_cfg *drv_cfg = dev->config;

	if (drv_cfg->clk_src == ACLK) {
		subsys = CLOCK_CONTROL_NRF_SUBSYS_HFAUDIO;
	} else
#endif
	{
		subsys = CLOCK_CONTROL_NRF_SUBSYS_HF;
	}

	drv_data->clk_mgr = z_nrf_clock_control_get_onoff(subsys);
	__ASSERT_NO_MSG(drv_data->clk_mgr != NULL);
}

static const struct i2s_driver_api i2s_nrf_drv_api = {
	.configure = i2s_nrfx_configure,
	.config_get = i2s_nrfx_config_get,
	.read = i2s_nrfx_read,
	.write = i2s_nrfx_write,
	.trigger = i2s_nrfx_trigger,
};

#define I2S(idx) DT_NODELABEL(i2s##idx)

#define I2S_PIN(idx, name)					\
	COND_CODE_1(DT_NODE_HAS_PROP(I2S(idx), name##_pin),	\
		    (DT_PROP(I2S(idx), name##_pin)),		\
		    (NRFX_I2S_PIN_NOT_USED))
#define I2S_CLK_SRC(idx)  DT_STRING_TOKEN(I2S(idx), clock_source)

#define I2S_NRFX_DEVICE(idx)						     \
	static void *tx_msgs[CONFIG_I2S_NRFX_TX_BLOCK_COUNT];		     \
	static void *rx_msgs[CONFIG_I2S_NRFX_RX_BLOCK_COUNT];		     \
	static struct i2s_nrfx_drv_data i2s_nrfx_data##idx = {		     \
		.state = I2S_STATE_READY,				     \
	};								     \
	static int i2s_nrfx_init##idx(const struct device *dev)		     \
	{								     \
		IRQ_CONNECT(DT_IRQN(I2S(idx)), DT_IRQ(I2S(idx), priority),   \
			    nrfx_isr, nrfx_i2s_irq_handler, 0);		     \
		irq_enable(DT_IRQN(I2S(idx)));				     \
		k_msgq_init(&i2s_nrfx_data##idx.tx_queue,		     \
			    (char *)tx_msgs, sizeof(void *),		     \
			    CONFIG_I2S_NRFX_TX_BLOCK_COUNT);		     \
		k_msgq_init(&i2s_nrfx_data##idx.rx_queue,		     \
			    (char *)rx_msgs, sizeof(void *),		     \
			    CONFIG_I2S_NRFX_RX_BLOCK_COUNT);		     \
		init_clock_manager(dev);				     \
		return 0;						     \
	}								     \
	static void data_handler##idx(nrfx_i2s_buffers_t const *p_released,  \
				      uint32_t status)			     \
	{								     \
		data_handler(DEVICE_DT_GET(I2S(idx)), p_released, status);   \
	}								     \
	static const struct i2s_nrfx_drv_cfg i2s_nrfx_cfg##idx = {	     \
		.data_handler = data_handler##idx,			     \
		.nrfx_def_cfg = NRFX_I2S_DEFAULT_CONFIG(I2S_PIN(idx, sck),   \
							I2S_PIN(idx, lrck),  \
							I2S_PIN(idx, mck),   \
							I2S_PIN(idx, sdout), \
							I2S_PIN(idx, sdin)), \
		.clk_src = I2S_CLK_SRC(idx),				     \
	};								     \
	BUILD_ASSERT(I2S_CLK_SRC(idx) != ACLK || NRF_I2S_HAS_CLKCONFIG,	     \
		"Clock source ACLK is not available.");			     \
	BUILD_ASSERT(I2S_CLK_SRC(idx) != ACLK ||			     \
		     DT_NODE_HAS_PROP(DT_NODELABEL(clock),		     \
				      hfclkaudio_frequency),		     \
		"Clock source ACLK requires the hfclkaudio-frequency "	     \
		"property to be defined in the nordic,nrf-clock node.");     \
	DEVICE_DT_DEFINE(I2S(idx), i2s_nrfx_init##idx, NULL,		     \
			 &i2s_nrfx_data##idx, &i2s_nrfx_cfg##idx,	     \
			 POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,		     \
			 &i2s_nrf_drv_api);

/* Existing SoCs only have one I2S instance. */
I2S_NRFX_DEVICE(0);
