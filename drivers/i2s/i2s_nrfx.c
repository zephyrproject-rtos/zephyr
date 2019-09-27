/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nordic Semiconductor nRF I2S
 */


#include <stdlib.h>
#include <i2s.h>
#include <nrfx_i2s.h>
#include "i2s_nrfx.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(i2s_nrfx, CONFIG_I2S_LOG_LEVEL);


#define LOG_ERROR(msg, ch_state) LOG_ERR("[Ch state: %u]%s", ch_state, msg)

#define DEV_CFG(dev) \
	(const struct i2s_nrfx_config *const)((dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct i2s_nrfx_data *const)(dev)->driver_data)

enum i2s_if_state {
	I2S_IF_NOT_READY = 0,
	I2S_IF_READY,
	I2S_IF_RESTARTING,
	I2S_IF_RUNNING,
	I2S_IF_STOPPING,
	I2S_IF_NEEDS_RESTART,
	I2S_IF_ERROR
};

struct i2s_nrfx_config {
	u8_t sck_pin;
	u8_t lrck_pin;
	u8_t mck_pin;
	u8_t sdout_pin;
	u8_t sdin_pin;
	void (*instance_init)(struct device *dev);
};

struct queue {
	void **queue_items;
	u8_t read_idx;
	u8_t write_idx;
	u8_t len;
};

struct channel_str {
	struct k_sem sem;
	struct k_mem_slab *mem_slab;
	s32_t timeout;
	enum i2s_state current_state;
	struct queue mem_block_queue;
	enum i2s_trigger_cmd last_trigger_cmd;
	struct i2s_config config;
};

struct i2s_nrfx_data {
	enum i2s_if_state state;
	size_t size;
	nrfx_i2s_buffers_t buffers;
	struct channel_str channel_tx;
	struct channel_str channel_rx;
};

static struct channel_str *channel_get(struct i2s_nrfx_data *i2s,
						 enum i2s_dir dir);
static inline struct i2s_nrfx_data *get_interface(void);
static int interface_set_state(struct i2s_nrfx_data *i2s,
				enum i2s_if_state new_state);
static int channel_stop(struct i2s_nrfx_data *i2s, enum i2s_dir dir);
static int channel_drop(struct i2s_nrfx_data *i2s, enum i2s_dir dir);
static int channel_drain(struct i2s_nrfx_data *i2s, enum i2s_dir dir);
static int channel_start(struct i2s_nrfx_data *i2s, enum i2s_dir dir);
static int channel_rx_store_data(struct i2s_nrfx_data *i2s, u32_t **buf);
static int channel_tx_get_data(struct i2s_nrfx_data *i2s, u32_t **buf);
static int channel_change_state(struct channel_str *channel,
				 enum i2s_state new_state);
static void channel_tx_callback(struct i2s_nrfx_data *i2s,
			nrfx_i2s_buffers_t const *p_released,
			u32_t status, nrfx_i2s_buffers_t *p_new_buffers);
static void channel_rx_callback(struct i2s_nrfx_data *i2s,
			nrfx_i2s_buffers_t const *p_released,
			u32_t status, nrfx_i2s_buffers_t *p_new_buffers);
static void channel_mem_clear(struct i2s_nrfx_data *i2s, enum i2s_dir dir);
static inline bool next_buffers_needed(u32_t status)
{
	return (status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)
			== NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED;
}
static inline bool transfer_stopped(u32_t status)
{
	return (status & NRFX_I2S_STATUS_TRANSFER_STOPPED)
			== NRFX_I2S_STATUS_TRANSFER_STOPPED;
}

/*
 *
 *  Queue management
 *
 */
static inline void queue_idx_inc(u8_t *idx, u8_t limit)
{
	*idx = (++(*idx) < limit) ? (*idx) : 0;
}

static inline bool queue_is_empty(struct queue *queue)
{
	return (queue->read_idx == queue->write_idx) ? true : false;
}

static void queue_init(struct queue *queue, u8_t len,
		       void **queue_items)
{
	queue->read_idx = 0;
	queue->write_idx = 0;
	queue->len = len;
	queue->queue_items = queue_items;
}

static int queue_add(struct queue *queue, void *data)
{
	u8_t wr_idx = queue->write_idx;

	assert(queue != NULL && data != NULL);
	queue_idx_inc(&wr_idx, queue->len);
	if (wr_idx == queue->read_idx) {
		/* cannot overwrite unread data */
		return -ENOMEM;
	}

	queue->queue_items[queue->write_idx] = data;
	queue->write_idx = wr_idx;
	return 0;
}

static int queue_fetch(struct queue *queue, void **data)
{
	assert(queue != NULL && data != NULL);
	if (queue_is_empty(queue)) {
		return -ENOMEM;
	}
	*data = queue->queue_items[queue->read_idx];
	queue_idx_inc(&queue->read_idx, queue->len);
	return 0;
}

/*
 * Interface service functions
 */

static void interface_error_service(struct i2s_nrfx_data *i2s,
					const char * const err_msg)
{
	assert(i2s != NULL);
	interface_set_state(i2s, I2S_IF_ERROR);
	LOG_ERR("%s", err_msg);
	nrfx_i2s_stop();
}

static int interface_set_state(struct i2s_nrfx_data *i2s,
				enum i2s_if_state new_state)
{
	bool change_forbidden = false;

	assert(i2s != NULL);
	switch (new_state) {
	case I2S_IF_STOPPING:
		if (i2s->state != I2S_IF_RUNNING &&
		    i2s->state != I2S_IF_NEEDS_RESTART) {
			change_forbidden = true;
		}
		break;
	case I2S_IF_NEEDS_RESTART:
		if (i2s->state != I2S_IF_RUNNING) {
			change_forbidden = true;
		}
		break;
	case I2S_IF_RUNNING:
		if (i2s->state != I2S_IF_RESTARTING &&
		    i2s->state != I2S_IF_READY) {
			change_forbidden = true;
		}
		break;
	case I2S_IF_READY:
		if (i2s->state != I2S_IF_STOPPING &&
		    i2s->state != I2S_IF_NOT_READY &&
		    i2s->state != I2S_IF_ERROR) {
			change_forbidden = true;
		}
		break;
	case I2S_IF_RESTARTING:
		if (i2s->state != I2S_IF_NEEDS_RESTART) {
			change_forbidden = true;
		}
		break;
	case I2S_IF_NOT_READY:
		if (i2s->state != I2S_IF_NOT_READY) {
			nrfx_i2s_uninit();
		}
		break;
	case I2S_IF_ERROR:
		break;
	default:
		LOG_ERR("Invalid interface state chosen");
		return -EINVAL;
	}
	if (change_forbidden) {
		interface_error_service(i2s,
				"Failed to change interface state");
		return -EIO;
	}
	i2s->state = new_state;
	return 0;
}

static inline enum i2s_if_state interface_get_state(
					struct i2s_nrfx_data *i2s)
{
	assert(i2s != NULL);
	return i2s->state;
}

static int interface_restart(struct i2s_nrfx_data *i2s)
{
	return interface_set_state(i2s, I2S_IF_NEEDS_RESTART);
}

static int interface_stop(struct i2s_nrfx_data *i2s)
{
	int ret;

	ret = interface_set_state(i2s, I2S_IF_STOPPING);
	if (ret < 0) {
		interface_error_service(i2s, "Failed to stop interface");
		return ret;
	}
	return 0;
}

static int interface_stop_restart(struct i2s_nrfx_data *i2s,
				struct channel_str *channel_to_stop_restart,
				enum i2s_state other_channel_state)
{
	int ret;

	ret = channel_change_state(channel_to_stop_restart,
					I2S_STATE_STOPPING);
	if (ret < 0) {
		return ret;
	}
	if (other_channel_state == I2S_STATE_RUNNING) {
		ret = interface_restart(i2s);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = interface_stop(i2s);
		if (ret < 0) {
			interface_error_service(i2s,
				"Failed to restart interface");
			return ret;
		}
	}
	return 0;
}

static int interface_start(struct i2s_nrfx_data *i2s)
{
	int ret;

	ret = interface_set_state(i2s, I2S_IF_RUNNING);
	if (ret < 0) {
		return ret;
	}

	/*nrfx_i2s_start() procedure needs buffer size in 32-bit word units*/
	nrfx_err_t status = nrfx_i2s_start(&i2s->buffers,
					   i2s->size / sizeof(u32_t), 0);

	if (status != NRFX_SUCCESS) {
		interface_error_service(i2s, "Failed to start interface");
		ret = -EIO;
	}

	return ret;
}

/* this function is called by 'nrfx_i2s_irq_handler()' which delivers:
 *  - 'p_released' - set of rx/tx buffers with received/sent data
 *  - 'status - bit field, at the moment:
 *      if NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED (1) is set: driver needs new
 *              buffers ('EVENT_TXPTRUPD' or 'EVENT_RXPTRUPD' is active)
 *      if NRFX_I2S_STATUS_TRANSFER_STOPPED (2) is set: driver has finished
 *              transmission ('EVENT_STOPPED' is active)
 */
static void interface_handler(nrfx_i2s_buffers_t const *p_released,
				    u32_t status)
{
	struct i2s_nrfx_data *i2s = get_interface();
	struct channel_str *rx_str = &i2s->channel_rx;
	struct channel_str *tx_str = &i2s->channel_tx;
	nrfx_i2s_buffers_t p_new_buffers;

	/* Call callbacks for tx/rx channels if they are not in idle state*/
	p_new_buffers.p_rx_buffer = NULL;
	p_new_buffers.p_tx_buffer = NULL;
	if (rx_str->current_state != I2S_STATE_READY &&
	    rx_str->current_state != I2S_STATE_NOT_READY) {
		channel_rx_callback(i2s, p_released, status,
					  &p_new_buffers);
	}
	if (tx_str->current_state != I2S_STATE_READY &&
	    tx_str->current_state != I2S_STATE_NOT_READY) {
		channel_tx_callback(i2s, p_released, status,
					  &p_new_buffers);
	}

	if (next_buffers_needed(status)) {
		if (interface_get_state(i2s) == I2S_IF_NEEDS_RESTART ||
		    interface_get_state(i2s) == I2S_IF_STOPPING) {
			/* if driver needs new buffers but user requested
			 * interface state change (e.g. called i2s_trigger())
			 * then peripheral needs to be stopped. In this case
			 * there is no need to set new buffers for driver.
			 * On next callback execution (this one will be caused
			 * by 'EVENT_STOPPED' - look 'else' below) interface
			 * will change state to:
			 *  - 'I2S_IF_RESTARTING' if there is at least one
			 *    channel involved in transmission
			 *  - 'I2S_IF_STOPPING' if no more data transmission
			 *    needed
			 */
			nrfx_i2s_stop();
			return;
		} else if (interface_get_state(i2s) == I2S_IF_RUNNING) {
			/* if driver requested new buffers and interface works
			 * normally then just set them
			 * (store 'TXD.PTR'/'RXD.PTR' registers)
			 */
			if (nrfx_i2s_next_buffers_set(&p_new_buffers) !=
						NRFX_SUCCESS) {
				interface_error_service(i2s,
						"Internal service error");
				return;
			}
		}
		i2s->buffers = p_new_buffers;
	} else {
		if (interface_get_state(i2s) == I2S_IF_NEEDS_RESTART) {
			if (interface_set_state(i2s, I2S_IF_RESTARTING) != 0) {
				interface_error_service(i2s,
						"Internal service error");
			}
		} else if (interface_get_state(i2s) == I2S_IF_STOPPING) {
			if (interface_set_state(i2s, I2S_IF_READY)) {
				interface_error_service(i2s,
						"Internal service error");
			}
		} else if (rx_str->current_state != I2S_STATE_RUNNING &&
			 tx_str->current_state != I2S_STATE_RUNNING) {
			if (interface_get_state(i2s) == I2S_IF_RUNNING) {
				interface_stop(i2s);
			}
		}
	}
	/* if nrfx driver sets 'NRFX_I2S_STATUS_TRANSFER_STOPPED' flag and the
	 * interface state is 'I2S_IF_RESTARTING' it means that last transfer
	 * before restart occurred. The peripheral will be stopped and started
	 * again (the reason could be e.g. start rx while tx works)
	 */
	if (transfer_stopped(status) &&
	    interface_get_state(i2s) == I2S_IF_RESTARTING) {
		int ret = interface_start(i2s);

		if (ret < 0) {
			interface_error_service(i2s, "Internal ISR error");
		}
		return;
	}
}

/*
 * configuration functions
 */

static void cfg_reinit(struct i2s_nrfx_data *i2s)
{
	struct channel_str *ch_tx = &i2s->channel_tx;
	struct channel_str *ch_rx = &i2s->channel_rx;

	nrfx_i2s_stop();
	i2s->state = I2S_IF_READY;
	ch_tx->current_state = I2S_STATE_READY;
	ch_rx->current_state = I2S_STATE_READY;
}

static inline nrf_i2s_mck_t cfg_get_divider(
		struct i2s_clk_settings_t const *clk_set, u8_t word_size)
{
	u32_t sub_idx = (word_size >> 3) - 1;

	return clk_set->divider[sub_idx];
}

static inline nrf_i2s_ratio_t cfg_get_ratio(
		struct i2s_clk_settings_t const *clk_set, u8_t word_size)
{
	u32_t sub_idx = (word_size >> 3) - 1;

	return clk_set->ratio[sub_idx];
}

static void cfg_match_clock_settings(nrfx_i2s_config_t *config,
			       struct i2s_config const *i2s_cfg)
{
	const struct i2s_clk_settings_t i2s_clock_settings[] =
			NRFX_I2S_AVAILABLE_CLOCK_SETTINGS;
	u32_t des_s_r = (s32_t)i2s_cfg->frame_clk_freq;
	u8_t nb_of_settings_array_elems = ARRAY_SIZE(i2s_clock_settings);
	struct i2s_clk_settings_t const *chosen_settings =
		&i2s_clock_settings[nb_of_settings_array_elems - 1];

	for (u8_t i = 1; i < nb_of_settings_array_elems; ++i) {
		if (des_s_r < i2s_clock_settings[i].frequency) {
			u32_t diff_h =
				i2s_clock_settings[i].frequency - des_s_r;
			u32_t diff_l =
				abs((s32_t)des_s_r -
				(s32_t)i2s_clock_settings[i - 1].frequency);
			chosen_settings = (diff_h < diff_l) ?
					  (&i2s_clock_settings[i]) :
					  (&i2s_clock_settings[i - 1]);
			break;
		}
	}
	config->mck_setup = cfg_get_divider(chosen_settings,
					    i2s_cfg->word_size);
	config->ratio = cfg_get_ratio(chosen_settings,
				      i2s_cfg->word_size);
}

static int cfg_periph_config(struct device *dev,
			     nrfx_i2s_config_t *const drv_cfg,
			     struct i2s_config const *i2s_cfg)
{
	const struct i2s_nrfx_config *const dev_const_cfg = DEV_CFG(dev);
	struct i2s_nrfx_data *i2s = get_interface();

	assert(drv_cfg != NULL);
	drv_cfg->sck_pin = dev_const_cfg->sck_pin;
	drv_cfg->lrck_pin = dev_const_cfg->lrck_pin;
	drv_cfg->mck_pin = dev_const_cfg->mck_pin;
	drv_cfg->sdout_pin = dev_const_cfg->sdout_pin;
	drv_cfg->sdin_pin = dev_const_cfg->sdin_pin;
	if (i2s_cfg->mem_slab == NULL) {
		interface_error_service(i2s, "Config: Invalid memory slab");
		return -EINVAL;
	}

	/*configuration validity verification*/
	switch (i2s_cfg->word_size) {
	case 8:
		drv_cfg->sample_width = NRF_I2S_SWIDTH_8BIT;
		break;
	case 16:
		drv_cfg->sample_width = NRF_I2S_SWIDTH_16BIT;
		break;
	case 24:
		drv_cfg->sample_width = NRF_I2S_SWIDTH_24BIT;
		break;
	default:
		if (i2s_cfg->word_size < 8 || i2s_cfg->word_size > 32) {
			/*this value isn't compatible with I2S standard*/
			interface_error_service(i2s,
					"Config: Invalid word size");
			return -EINVAL;
		}
		interface_error_service(i2s,
			"Config: Unsupported word size");
		return -ENOTSUP;
	}

	/*format validity verification*/
	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		drv_cfg->alignment = NRF_I2S_ALIGN_LEFT;
		drv_cfg->format = NRF_I2S_FORMAT_I2S;
		break;

	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		drv_cfg->alignment = NRF_I2S_ALIGN_LEFT;
		drv_cfg->format = NRF_I2S_FORMAT_ALIGNED;
		break;

	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		drv_cfg->alignment = NRF_I2S_ALIGN_RIGHT;
		drv_cfg->format = NRF_I2S_FORMAT_ALIGNED;
		break;

	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		interface_error_service(i2s,
					"Config: Unsupported data format");
		return -ENOTSUP;

	default:
		interface_error_service(i2s, "Config: Invalid data format");
		return -EINVAL;
	}
	if (i2s_cfg->format & I2S_FMT_CLK_FORMAT_MASK) {
		interface_error_service(i2s,
					"Config: Unsupported clock format");
		return -ENOTSUP;
	}

	/*mode options validity check*/
	if ((i2s_cfg->options & I2S_OPT_PINGPONG) ||
	    (i2s_cfg->options & I2S_OPT_LOOPBACK)) {
		interface_error_service(i2s,
			"Config: Unsupported mode setiings");
		return -ENOTSUP;
	}

	if (i2s_cfg->options & I2S_OPT_BIT_CLK_GATED) {
		if ((i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) &&
		    (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE)) {
			drv_cfg->mode = NRF_I2S_MODE_SLAVE;
		} else {
			if ((i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE) ||
			    (i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE)) {
				interface_error_service(i2s,
					"Config: Unsupported mode setiings");
				return -ENOTSUP;
			}
			drv_cfg->mode = NRF_I2S_MODE_MASTER;
		}
	} else {
		interface_error_service(i2s,
					"Config: Unsupported clock settings");
		return -ENOTSUP;
	}

	/*channel and size configuration validity check*/
	switch (i2s_cfg->channels) {
	case 2:
		drv_cfg->channels = NRF_I2S_CHANNELS_STEREO;
		break;
	case 1:
		drv_cfg->channels = NRF_I2S_CHANNELS_LEFT;
		break;
	default:
		interface_error_service(i2s,
				"Config: Invalid number of channels");
		return -EINVAL;
	}
	if (i2s_cfg->block_size == 0) {
		interface_error_service(i2s, "Config: Invalid block size");
		return -EINVAL;
	}
	i2s->size = i2s_cfg->block_size;
	cfg_match_clock_settings(drv_cfg, i2s_cfg);
	return 0;
}

/*
 *
 * API functions
 *
 */

static int i2s_nrfx_initialize(struct device *dev)
{
	struct i2s_nrfx_data *i2s = get_interface();
	const struct i2s_nrfx_config *const dev_const_cfg = DEV_CFG(dev);

	assert(dev != NULL);
	k_sem_init(&i2s->channel_rx.sem, 0, CONFIG_NRFX_I2S_RX_BLOCK_COUNT);
	k_sem_init(&i2s->channel_tx.sem, CONFIG_NRFX_I2S_TX_BLOCK_COUNT,
		   CONFIG_NRFX_I2S_TX_BLOCK_COUNT);
	dev_const_cfg->instance_init(dev);
	return 0;
}

static int i2s_nrfx_api_configure(struct device *dev, enum i2s_dir dir,
			   struct i2s_config *i2s_cfg)
{
	struct i2s_nrfx_data *i2s = get_interface();
	nrfx_i2s_config_t drv_cfg;
	struct channel_str *channel;
	const struct channel_str *other_channel = channel_get(i2s,
				dir == I2S_DIR_TX ? I2S_DIR_RX : I2S_DIR_TX);
	int ret;
	nrfx_err_t status;

	assert(i2s_cfg != NULL && dev != NULL);
	channel = channel_get(i2s, dir);
	assert(channel != NULL);

	/*for proper configuration transmission must be stopped */
	if (channel->current_state != I2S_STATE_NOT_READY &&
	    channel->current_state != I2S_STATE_READY) {
		LOG_ERROR("Config: Channel must be in ready/not ready state",
			  (u32_t)channel->current_state);
		interface_error_service(i2s,
			"Config: Invalid channel state");
		return -EIO;
	}
	if (interface_get_state(i2s) != I2S_IF_READY &&
	    interface_get_state(i2s) != I2S_IF_NOT_READY) {
		LOG_ERROR("Config: Interface must be ready/not ready state",
			  (u32_t)channel->current_state);
		return -EIO;
	}
	if (i2s_cfg->frame_clk_freq == 0) {
		/*reinit mode - cleaning channel data*/
		channel_mem_clear(i2s, dir);
		channel_change_state(channel, I2S_STATE_NOT_READY);
		interface_set_state(i2s, I2S_IF_NOT_READY);
		return 0;
	}

	if (other_channel->current_state != I2S_STATE_NOT_READY) {
		const struct i2s_config *other_ch_cfg = &other_channel->config;

		/*if another channel is already configured it is necessary to
		 * check configuration compatibility
		 */
		if (other_ch_cfg->word_size      != i2s_cfg->word_size      ||
		    other_ch_cfg->channels       != i2s_cfg->channels       ||
		    other_ch_cfg->format         != i2s_cfg->format         ||
		    other_ch_cfg->options        != i2s_cfg->options        ||
		    other_ch_cfg->frame_clk_freq != i2s_cfg->frame_clk_freq ||
		    other_ch_cfg->block_size     != i2s_cfg->block_size) {
			LOG_ERR("Config: Incompatible channel settings");
			return -EINVAL;
		}
	} else {
		/* Single channel reinitialisation
		 * When reintialisation with two channels is needed it is
		 * necessary to deinit at least one of them (call this function
		 * with 'frame_clk_freq' set to 0)
		 */
		channel_change_state(channel, I2S_STATE_NOT_READY);
		ret = interface_set_state(i2s, I2S_IF_NOT_READY);
		if (ret < 0) {
			channel_change_state(channel, I2S_STATE_ERROR);
			return -EIO;
		}
	}

	if (interface_get_state(i2s) == I2S_IF_NOT_READY) {
		/* peripheral configuration and driver initialization is needed
		 * only when interface is not configured (I2S_IF_NOT_READY)
		 */
		ret = cfg_periph_config(dev, &drv_cfg, i2s_cfg);
		/* disable channels in case of invalid configuration*/
		if (ret < 0) {
			LOG_ERROR("Config: Failed to configure peripheral",
				  (u32_t)channel->current_state);
			channel_change_state(channel, I2S_STATE_ERROR);
			return ret;
		}
		status = nrfx_i2s_init(&drv_cfg, interface_handler);
		if (status != NRFX_SUCCESS) {
			LOG_ERROR("Config: nrfx_i2s_init() returned value:",
				  (u32_t)channel->current_state);
			LOG_ERR("0x%x", status);
			channel_change_state(channel, I2S_STATE_ERROR);
			return -EIO;
		}
		ret = interface_set_state(i2s, I2S_IF_READY);
		if (ret < 0) {
			channel_change_state(channel, I2S_STATE_ERROR);
			return -EIO;
		}
	}
	ret = channel_change_state(channel, I2S_STATE_READY);
	if (ret < 0) {
		channel_change_state(channel, I2S_STATE_ERROR);
		return ret;
	}

	/*store configuration*/
	if (dir == I2S_DIR_RX) {
		i2s->buffers.p_rx_buffer = NULL;
	}
	if (dir == I2S_DIR_TX) {
		i2s->buffers.p_tx_buffer = NULL;
	}
	channel->mem_slab = i2s_cfg->mem_slab;
	channel->timeout = i2s_cfg->timeout;
	channel->config = *i2s_cfg;
	return ret;
}

static struct i2s_config *i2s_nrfx_config_get(struct device *dev,
					     enum i2s_dir dir)
{
	struct i2s_nrfx_data *i2s = get_interface();

	assert(dev != NULL);
	return &channel_get(i2s, dir)->config;
}

static int i2s_nrfx_read(struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_nrfx_data *i2s = get_interface();
	struct channel_str *ch_rx = &i2s->channel_rx;
	int ret;

	assert(dev != NULL && mem_block != NULL && size != NULL);
	*size = 0;
	if ((ch_rx->current_state == I2S_STATE_NOT_READY ||
	     ch_rx->current_state == I2S_STATE_ERROR) &&
	     queue_is_empty(&ch_rx->mem_block_queue)) {
		return -EIO;
	}
	ret = k_sem_take(&ch_rx->sem, ch_rx->timeout);
	if (ret < 0) {
		return ret;
	}
	ret = queue_fetch(&ch_rx->mem_block_queue, mem_block);
	if (ret < 0) {
		return -EIO;
	}
	*size = i2s->size;
	return 0;
}

static int i2s_nrfx_write(struct device *dev, void *mem_block, size_t size)
{
	struct i2s_nrfx_data *i2s = get_interface();
	struct channel_str *ch_tx = &i2s->channel_tx;
	int ret;

	assert(dev != NULL && mem_block != NULL);
	if (ch_tx->current_state != I2S_STATE_READY &&
	    ch_tx->current_state != I2S_STATE_RUNNING) {
		return -EIO;
	}
	if (size != i2s->size) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}
	ret = k_sem_take(&ch_tx->sem, ch_tx->timeout);
	if (ret < 0) {
		return ret;
	}
	ret = queue_add(&ch_tx->mem_block_queue, mem_block);
	if (ret < 0) {
		return ret;
	}
	return 0;
}



static int i2s_nrfx_trigger(struct device *dev, enum i2s_dir dir,
			    enum i2s_trigger_cmd cmd)
{
	struct i2s_nrfx_data *i2s = get_interface();
	struct channel_str *channel;
	int ret;

	assert(dev != NULL);
	if ((interface_get_state(i2s) == I2S_IF_STOPPING) ||
	    (interface_get_state(i2s) == I2S_IF_NEEDS_RESTART)) {
		if (cmd != I2S_TRIGGER_PREPARE) {
			/* This case is not an error - it only provides
			 * that user can't trigger at the moment due to the
			 * unstable interface state (it's just changing).
			 * User should call it again after while. The API
			 * doesn't provide return value for this case.
			 */
			LOG_INF("Wait for stable state");
			return -EIO;
		}
	}
	channel = channel_get(i2s, dir);
	assert(channel != NULL);
	switch (cmd) {
	case I2S_TRIGGER_START:
		if (channel->current_state != I2S_STATE_READY) {
			LOG_ERROR("Failed to execute I2S_TRIGGER_START",
				  (u32_t)channel->current_state);
			return -EIO;
		}
		ret = channel_start(i2s, dir);
		break;

	case I2S_TRIGGER_STOP:

		if (channel->current_state != I2S_STATE_RUNNING) {
			LOG_ERROR("Failed to execute I2S_TRIGGER_STOP",
				  (u32_t)channel->current_state);
			return -EIO;
		}
		ret = channel_stop(i2s, dir);
		break;

	case I2S_TRIGGER_DRAIN:
		if (channel->current_state != I2S_STATE_RUNNING) {
			LOG_ERROR("Failed to execute I2S_TRIGGER_DRAIN",
				  (u32_t)channel->current_state);
			return -EIO;
		}
		ret = channel_drain(i2s, dir);
		break;

	case I2S_TRIGGER_DROP:
		if (channel->current_state == I2S_STATE_NOT_READY) {
			LOG_ERROR("Failed to execute I2S_TRIGGER_DROP",
				  (u32_t)channel->current_state);
			return -EIO;
		}
		ret = channel_drop(i2s, dir);
		break;

	case I2S_TRIGGER_PREPARE:
	{
		if (channel->current_state != I2S_STATE_ERROR) {
			LOG_ERROR("Failed to execute I2S_TRIGGER_PREPARE",
				  (u32_t)channel->current_state);
			return -EIO;
		}
		ret = channel_drop(i2s, dir);
		break;
	}
	default:
		LOG_ERROR("Invalid trigger command",
			  (u32_t)channel->current_state);
		return -EINVAL;
	}
	if (ret < 0) {
		LOG_ERROR("Error trigger while execution",
			  (u32_t)channel->current_state);
		channel_change_state(channel, I2S_STATE_ERROR);
		return ret;
	}
	channel->last_trigger_cmd = cmd;
	return 0;
}

/*
 * channel management functions
 */

static struct channel_str *channel_get(struct i2s_nrfx_data *i2s,
						 enum i2s_dir dir)
{
	assert(i2s != NULL);
	switch (dir) {
	case I2S_DIR_RX:
		return &i2s->channel_rx;
	case I2S_DIR_TX:
		return &i2s->channel_tx;
	}
	return NULL;
}

static int channel_change_state(struct channel_str *channel,
				 enum i2s_state new_state)
{
	bool change_forbidden = false;
	enum i2s_state old_state = channel->current_state;

	assert(channel != NULL);
	switch (new_state) {
		break;
	case I2S_STATE_READY:
		if (old_state == I2S_STATE_READY) {
			change_forbidden = true;
		}
		break;
	case I2S_STATE_RUNNING:
		if (old_state != I2S_STATE_READY) {
			change_forbidden = true;
		}
		break;
	case I2S_STATE_STOPPING:
		if (old_state != I2S_STATE_RUNNING) {
			change_forbidden = true;
		}
		break;
	case I2S_STATE_NOT_READY:
	case I2S_STATE_ERROR:
		break;
	default:
		LOG_ERR("Invalid channel state chosen");
		channel_change_state(channel, I2S_STATE_ERROR);
		return -EINVAL;
	}

	if (change_forbidden) {
		LOG_ERROR("Failed to change channel state",
			  (u32_t)channel->current_state);
		channel_change_state(channel, I2S_STATE_ERROR);
		return -EIO;
	}
	channel->current_state = new_state;
	return 0;
}

static int channel_start(struct i2s_nrfx_data *i2s, enum i2s_dir dir)
{
	int ret;
	unsigned int key;
	struct channel_str * const channel_to_start = channel_get(i2s, dir);

	ret = channel_change_state(channel_to_start, I2S_STATE_RUNNING);
	if (ret < 0) {
		return ret;
	}
	if (interface_get_state(i2s) != I2S_IF_RUNNING &&
	    interface_get_state(i2s) != I2S_IF_READY) {
		LOG_ERR("Invalid interface state");
		return -EIO;
	}
	if (dir == I2S_DIR_RX) {
		ret = k_mem_slab_alloc(channel_to_start->mem_slab,
			(void **)&i2s->buffers.p_rx_buffer, K_NO_WAIT);
	} else {
		ret = channel_tx_get_data(i2s,
			(u32_t **)&i2s->buffers.p_tx_buffer);
	}
	if (ret < 0) {
		LOG_ERROR(dir == I2S_DIR_RX ? "Memory allocation error" :
				      "Queue fetching error",
				      (u32_t)channel_to_start->current_state);
		return ret;
	}
	key = irq_lock();
	if (interface_get_state(i2s) == I2S_IF_RUNNING) {
		ret = interface_restart(i2s);
	} else if (interface_get_state(i2s) == I2S_IF_READY) {
		ret = interface_start(i2s);
	}
	irq_unlock(key);
	if (ret < 0) {
		LOG_ERROR("Failed to start/restart interface",
			  (u32_t)channel_to_start->current_state);
		return ret;
	}
	return 0;
}

static void channel_tx_mem_clear(struct i2s_nrfx_data *i2s,
				 void const *first_block)
{
	struct channel_str * const ch_tx = &i2s->channel_tx;
	void *mem_block = (void *)first_block;

	assert(i2s != NULL);
	if (first_block == NULL) {
		if (channel_tx_get_data(i2s, (u32_t **)&mem_block) != 0) {
			return;
		}
	}
	do {
		k_mem_slab_free(ch_tx->mem_slab,
			(void **)&mem_block);
	} while (channel_tx_get_data(i2s, (u32_t **)&mem_block) == 0);
	while (ch_tx->sem.count < ch_tx->sem.limit) {
		k_sem_give(&ch_tx->sem);
	}
}

static void channel_rx_mem_clear(struct i2s_nrfx_data *i2s)
{
	struct channel_str * const ch_rx = &i2s->channel_rx;
	void *mem_block;

	assert(i2s != NULL);
	while (queue_fetch(&ch_rx->mem_block_queue,
				(void **)&mem_block) == 0) {
		k_mem_slab_free(ch_rx->mem_slab, (void **)&mem_block);
	}
	while (ch_rx->sem.count) {
		if (k_sem_take(&ch_rx->sem, K_NO_WAIT) < 0) {
			return;
		}
	}
}

static void channel_mem_clear(struct i2s_nrfx_data *i2s, enum i2s_dir dir)
{
	if (dir == I2S_DIR_RX) {
		channel_rx_mem_clear(i2s);
	} else {
		channel_tx_mem_clear(i2s, NULL);
	}
}

static int channel_drop(struct i2s_nrfx_data *i2s, enum i2s_dir dir)
{
	int ret;
	struct channel_str * const channel_to_drop = channel_get(i2s, dir);
	struct channel_str * const other_channel = channel_get(i2s,
			dir == I2S_DIR_TX ? I2S_DIR_RX : I2S_DIR_TX);

	if (channel_to_drop->current_state == I2S_STATE_RUNNING) {
		ret =  interface_stop_restart(i2s, channel_to_drop,
					      other_channel->current_state);
		if (ret < 0) {
			interface_error_service(i2s,
					"Failed to restart interface");
			return ret;
		}
	} else {
		cfg_reinit(i2s);
		channel_mem_clear(i2s, dir);
	}
	return 0;
}

static int channel_stop(struct i2s_nrfx_data *i2s, enum i2s_dir dir)
{
	struct channel_str * const channel_to_stop = channel_get(i2s, dir);
	struct channel_str * const other_channel = channel_get(i2s,
			dir == I2S_DIR_TX ? I2S_DIR_RX : I2S_DIR_TX);

	return interface_stop_restart(i2s, channel_to_stop,
				      other_channel->current_state);
}

static int channel_drain(struct i2s_nrfx_data *i2s, enum i2s_dir dir)
{
	struct channel_str * const channel_to_drain = channel_get(i2s, dir);

	return dir == I2S_DIR_RX ? channel_stop(i2s, dir) :
		 channel_change_state(channel_to_drain, I2S_STATE_STOPPING);
}

static bool channel_check_empty(struct channel_str *channel)
{
	return queue_is_empty(&channel->mem_block_queue);
}

static int channel_tx_get_data(struct i2s_nrfx_data *i2s, u32_t **buf)
{
	struct channel_str *ch_tx = &i2s->channel_tx;
	int ret;

	assert(i2s != NULL);
	ret = queue_fetch(&ch_tx->mem_block_queue, (void **)buf);
	if (ret < 0) {
		return ret;
	}
	k_sem_give(&ch_tx->sem);
	return 0;
}

/* In case of constant tx transmission this callback:
 * - frees tx buffer which has just been transmitted via I2S interface
 * - gets new buffer from queue which will be passed to nrfx driver as
 *   next to be transferred (content of 'TXD.PTR' register).
 *   Status parameter informs about handler execution reason:
 *   - if 1 - next buffer is needed
 *   - if 0 - transfer is finishing
 */
static void channel_tx_callback(struct i2s_nrfx_data *i2s,
			nrfx_i2s_buffers_t const *p_released,
			u32_t status, nrfx_i2s_buffers_t *p_new_buffers)
{
	struct channel_str *ch_tx = &i2s->channel_tx;
	u32_t *mem_block = NULL;
	int get_data_ret = 1;

	if (ch_tx->current_state == I2S_STATE_RUNNING &&
	    interface_get_state(i2s) == I2S_IF_NEEDS_RESTART) {
		/* This code services case when tx channel transmits data
		 * constantly while rx channel is beginning/finishing its
		 * transfer (user called' i2s_trigger()' with 'I2S_DIR_RX'
		 * parameter)).
		 * In this case NRF I2S peripheral needs to be restarted.
		 */
		if (p_released->p_tx_buffer != NULL &&
		    next_buffers_needed(status)) {
			/* when interface needs to be restarted and
			 * last event was 'EVENT_STOPPED' then we don't
			 * free this buffer - it will be used after
			 * after interface restarts - tx transmission
			 * will be still running and user don't want to
			 * lose data.
			 */
			k_mem_slab_free(ch_tx->mem_slab,
					(void **)&p_released->p_tx_buffer);
		}
		get_data_ret = channel_tx_get_data(i2s, &mem_block);
		if (get_data_ret == 0) {
			k_mem_slab_free(ch_tx->mem_slab,
					(void **)&mem_block);
		}
		return;
	}
	if (p_released->p_tx_buffer != NULL) {
		k_mem_slab_free(ch_tx->mem_slab,
				(void **)&p_released->p_tx_buffer);
	}
	if (next_buffers_needed(status)) {
		get_data_ret = channel_tx_get_data(i2s, &mem_block);
	}
	if (ch_tx->current_state == I2S_STATE_STOPPING) {
		/* finishing tx transfer caused by user trigger command*/
		enum i2s_trigger_cmd ch_cmd = ch_tx->last_trigger_cmd;

		if (next_buffers_needed(status)) {
			switch (ch_cmd) {
			case I2S_TRIGGER_DROP:
				if (get_data_ret == 0) {
					channel_tx_mem_clear(i2s, mem_block);
				}
				break;
			case I2S_TRIGGER_STOP:
				if (get_data_ret == 0) {
					k_mem_slab_free(ch_tx->mem_slab,
							(void **)&mem_block);
				}
				break;
			case I2S_TRIGGER_DRAIN:
				break;
			default:
				channel_change_state(ch_tx, I2S_STATE_ERROR);
				return;
			}
		} else {
			int ret = channel_change_state(ch_tx, I2S_STATE_READY);

			if (ret < 0) {
				channel_change_state(ch_tx, I2S_STATE_ERROR);
				return;
			}
			if (ch_cmd == I2S_TRIGGER_DRAIN) {
				return;
			}
		}
		i2s->buffers.p_tx_buffer = NULL;
	} else if (ch_tx->current_state == I2S_STATE_ERROR) {
		return;
	} else if (get_data_ret < 0) {
		interface_error_service(i2s, "TX internal callback error");
		channel_change_state(ch_tx, I2S_STATE_ERROR);
		return;
	} else if (channel_check_empty(ch_tx) == true) {
		/*underrun error occurred*/
		if (get_data_ret == 0) {
			k_mem_slab_free(ch_tx->mem_slab,
					(void **)&mem_block);
		}
		interface_error_service(i2s, "TX underrun error");
		channel_change_state(ch_tx, I2S_STATE_ERROR);
		return;
	}
	/* continue transmission */
	p_new_buffers->p_tx_buffer = mem_block;
}

/* In case of constant rx transmission this callback:
 * - stores in queue rx buffer which has just been received via I2S interface
 * - allocates new rx buffer for next transfer purpose
 * Status parameter informs about handler execution reason:
 *   - if 1 - next buffer is needed
 *   - if 0 - transfer is finishing
 */
static void channel_rx_callback(struct i2s_nrfx_data *i2s,
			nrfx_i2s_buffers_t const *p_released,
			u32_t status, nrfx_i2s_buffers_t *p_new_buffers)
{
	struct channel_str *ch_rx = &i2s->channel_rx;
	int ret;

	if (p_released->p_rx_buffer != NULL && next_buffers_needed(status)) {
		/* Content of received buffer is valuable.
		 * If 'EVENT_STOPPED' generated 'then next_buffers_needed()'
		 * returns false - function 'channel_rx_store_data()' won't
		 * execute because buffer didn't fill.
		 */
		ret = channel_rx_store_data(i2s,
					   (u32_t **)&p_released->p_rx_buffer);
		if (ret < 0) {
			return;
		}
	}
	if (ch_rx->current_state == I2S_STATE_STOPPING) {
		/* finishing rx transfer caused by user trigger command*/
		enum i2s_trigger_cmd ch_cmd = ch_rx->last_trigger_cmd;

		if (next_buffers_needed(status)) {
			switch (ch_cmd) {
			case I2S_TRIGGER_DROP:
				channel_rx_mem_clear(i2s);
				break;
			case I2S_TRIGGER_DRAIN:
			case I2S_TRIGGER_STOP:
				break;
			default:
				LOG_ERROR("RX Callback command unknown",
					  (u32_t)ch_rx->current_state);
				channel_change_state(ch_rx, I2S_STATE_ERROR);
				return;
			}
		} else {
			if (p_released->p_rx_buffer) {
				ret = channel_rx_store_data(i2s,
					(u32_t **)&p_released->p_rx_buffer);
				if (ret < 0) {
					return;
				}
			}
			int ret = channel_change_state(ch_rx,
					I2S_STATE_READY);
			if (ret < 0) {
				channel_change_state(ch_rx, I2S_STATE_ERROR);
				return;
			}
		}
		i2s->buffers.p_rx_buffer = NULL;
		return;
	} else if (ch_rx->current_state == I2S_STATE_RUNNING &&
		   interface_get_state(i2s) == I2S_IF_NEEDS_RESTART) {
		return;
	} else if (ch_rx->current_state == I2S_STATE_ERROR) {
		if (p_released->p_rx_buffer != NULL) {
			k_mem_slab_free(ch_rx->mem_slab,
					(void **)&p_released->p_rx_buffer);
		}
		return;
	}
	if (next_buffers_needed(status)) {
		ret = k_mem_slab_alloc(ch_rx->mem_slab,
				       (void **)&p_new_buffers->p_rx_buffer,
				       K_NO_WAIT);
		if (ret < 0) {
			/*overrun error occurred*/
			interface_error_service(i2s, "RX overrun error");
			channel_change_state(ch_rx, I2S_STATE_ERROR);
			return;
		}
	}
}

static int channel_rx_store_data(struct i2s_nrfx_data *i2s, u32_t **buf)
{
	struct channel_str *ch_rx = &i2s->channel_rx;
	int ret;

	assert(i2s != NULL);
	ret = queue_add(&ch_rx->mem_block_queue, *buf);
	if (ret < 0) {
		return ret;
	}
	k_sem_give(&ch_rx->sem);

	return 0;
}

static void isr(void *arg)
{
	/* 'nrfx_i2s_irq_handler()' calls 'interface_handler()' which in turn
	 * can call:
	 *  - 'channel_tx_callback()' when tx channel is running
	 *  - 'channel_rx_callback()' when rx channel is running
	 */
	nrfx_i2s_irq_handler();
}

#define I2S_NRFX_DEVICE(idx)						      \
static void *q_rx_##idx##_buf[CONFIG_NRFX_I2S_RX_BLOCK_COUNT + 1];	      \
static void *q_tx_##idx##_buf[CONFIG_NRFX_I2S_TX_BLOCK_COUNT + 1];	      \
static void i2s_nrfx_irq_##idx##_config(struct device *dev);		      \
									      \
static void setup_instance_##idx(struct device *dev)			      \
{									      \
	struct i2s_nrfx_data *i2s = get_interface();			      \
									      \
	queue_init(&i2s->channel_tx.mem_block_queue,			      \
		CONFIG_NRFX_I2S_TX_BLOCK_COUNT + 1, &q_tx_##idx##_buf[0]);    \
	queue_init(&i2s->channel_rx.mem_block_queue,			      \
		CONFIG_NRFX_I2S_RX_BLOCK_COUNT + 1, &q_rx_##idx##_buf[0]);    \
	i2s_nrfx_irq_##idx##_config(dev);				      \
}									      \
									      \
static const struct i2s_nrfx_config channel_cfg_##idx = {		      \
	.sck_pin = DT_NORDIC_NRF_I2S_I2S_##idx##_SCK_PIN,		      \
	.lrck_pin = DT_NORDIC_NRF_I2S_I2S_##idx##_LRCK_PIN,		      \
	.mck_pin = DT_NORDIC_NRF_I2S_I2S_##idx##_MCK_PIN,		      \
	.sdout_pin = DT_NORDIC_NRF_I2S_I2S_##idx##_SDOUT_PIN,		      \
	.sdin_pin = DT_NORDIC_NRF_I2S_I2S_##idx##_SDIN_PIN,		      \
	.instance_init = setup_instance_##idx,				      \
};									      \
									      \
static struct i2s_nrfx_data channels_data_##idx = {			      \
	.state = I2S_IF_NOT_READY,					      \
	.size = 0,							      \
	.channel_tx = {							      \
		.current_state = I2S_STATE_NOT_READY,			      \
		.last_trigger_cmd = I2S_TRIGGER_PREPARE,		      \
	},								      \
	.channel_rx = {							      \
		.current_state = I2S_STATE_NOT_READY,			      \
		.last_trigger_cmd = I2S_TRIGGER_PREPARE,		      \
	}								      \
};									      \
									      \
DEVICE_AND_API_INIT(i2s_##idx, DT_NORDIC_NRF_I2S_I2S_##idx##_LABEL,	      \
		    &i2s_nrfx_initialize, &channels_data_##idx,		      \
		    &channel_cfg_##idx, POST_KERNEL,			      \
		    CONFIG_I2S_INIT_PRIORITY, &i2s_nrf_driver_api);	      \
									      \
static void i2s_nrfx_irq_##idx##_config(struct device *dev)		      \
{									      \
	IRQ_CONNECT(DT_NORDIC_NRF_I2S_I2S_##idx##_IRQ_0,		      \
		DT_NORDIC_NRF_I2S_I2S_##idx##_IRQ_0_PRIORITY,		      \
		isr, DEVICE_GET(i2s_##idx), 0);				      \
	irq_enable(DT_NORDIC_NRF_I2S_I2S_##idx##_IRQ_0);		      \
}

static const struct i2s_driver_api i2s_nrf_driver_api = {
	.configure = i2s_nrfx_api_configure,
	.read = i2s_nrfx_read,
	.write = i2s_nrfx_write,
	.trigger = i2s_nrfx_trigger,
	.config_get = i2s_nrfx_config_get,
};

I2S_NRFX_DEVICE(0);

static inline struct i2s_nrfx_data *get_interface(void)
{
	return DEV_DATA(DEVICE_GET(i2s_0));
}
