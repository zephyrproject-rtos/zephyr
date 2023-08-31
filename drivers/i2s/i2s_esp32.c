#define DT_DRV_COMPAT espressif_esp32_i2s

#include <string.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include "esp_check.h"
#include "i2s_esp32.h"
LOG_MODULE_REGISTER(i2s_ll_esp32);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "I2S series coder is not defined in DTS"
#endif

#define I2S_ESP32_RX_BLOCK_COUNT 1
#define I2S_ESP32_TX_BLOCK_COUNT 1
// TODO get address from DTS
#define I2S0_ADDR                (void *)0x6000F000

#define MODULO_INC(val, max)                                                                       \
	{                                                                                          \
		val = (++val < max) ? val : 0;                                                     \
	}

/*
 * Get data from the queue
 */
static int queue_get(struct ring_buf *rb, void **mem_block, size_t *size)
{
	unsigned int key;

	key = irq_lock();

	if (rb->tail == rb->head) {
		/* Ring buffer is empty */
		irq_unlock(key);
		return -ENOMEM;
	}

	*mem_block = rb->buf[rb->tail].mem_block;
	*size = rb->buf[rb->tail].size;
	MODULO_INC(rb->tail, rb->len);

	irq_unlock(key);

	return 0;
}

/*
 * Put data in the queue
 */
static int queue_put(struct ring_buf *rb, void *mem_block, size_t size)
{
	uint16_t head_next;
	unsigned int key;

	key = irq_lock();

	head_next = rb->head;
	MODULO_INC(head_next, rb->len);

	if (head_next == rb->tail) {
		/* Ring buffer is full */
		irq_unlock(key);
		return -ENOMEM;
	}

	rb->buf[rb->head].mem_block = mem_block;
	rb->buf[rb->head].size = size;
	rb->head = head_next;

	irq_unlock(key);

	return 0;
}

static int i2s_esp32_enable_clock(const struct device *dev)
{
	const struct i2s_esp32_cfg *cfg = dev->config;
	const struct device *clk = cfg->clock_dev;
	int ret;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clk, cfg->clock_subsys);
	if (ret != 0) {
		LOG_ERR("Could not enable I2S clock");
		return -EIO;
	}

	return 0;
}

static esp_err_t i2s_calculate_common_clock(i2s_hal_config_t *const hal_config,
					    i2s_hal_context_t *const hal_ctx,
					    i2s_hal_clock_cfg_t *clk_cfg)
{
	ESP_RETURN_ON_FALSE(clk_cfg, ESP_ERR_INVALID_ARG, TAG, "input clk_cfg is NULL");

	uint32_t multi;
	/* Calculate multiple */
	if (hal_config->mode & I2S_MODE_MASTER) {
		multi = I2S_MCLK_MULTIPLE_256;
	} else {
		/* Only need to set the multiple of mclk to sample rate for MASTER mode,
		 * because BCK and WS clock are provided by the external codec in SLAVE mode.
		 * The multiple should be big enough to get a high module clock which could detect
		 * the edges of externel clock more accurately, otherwise the data we receive or
		 * send would get a large latency and go wrong due to the slow module clock. But on
		 * ESP32 and ESP32S2, due to the different clock work mode in hardware, their
		 * multiple should be set to an appropriate range according to the sample bits, and
		 * this particular multiple finally aims at guaranteeing the bclk_div not smaller
		 * than 8, if not, the I2S may can't send data or send wrong data. Here use
		 * 'SOC_I2S_SUPPORTS_TDM' to differentialize other chips with ESP32 and ESP32S2.
		 */
		multi = I2S_LL_BASE_CLK / hal_config->sample_rate;
	}

	/* Set I2S bit clock */
	clk_cfg->bclk = hal_config->sample_rate * hal_config->total_chan * hal_config->sample_bits;
	/* If fixed_mclk and use_apll are set, use fixed_mclk as mclk frequency, otherwise calculate
	 * by mclk = sample_rate * multiple */
	clk_cfg->mclk = hal_config->sample_rate * I2S_MCLK_MULTIPLE_256;

	/* Calculate bclk_div = mclk / bclk */
	clk_cfg->bclk_div = clk_cfg->mclk / clk_cfg->bclk;
	/* Get I2S system clock by config source clock */
	i2s_hal_set_clock_src(hal_ctx, I2S_CLK_D2CLK);
	clk_cfg->sclk = I2S_LL_BASE_CLK;
	/* Get I2S master clock rough division, later will calculate the fine division parameters in
	 * HAL */
	clk_cfg->mclk_div = clk_cfg->sclk / clk_cfg->mclk;

	/* Check if the configuration is correct */
	ESP_RETURN_ON_FALSE(clk_cfg->mclk <= clk_cfg->sclk, ESP_ERR_INVALID_ARG, TAG,
			    "sample rate is too large");

	return ESP_OK;
}

static int i2s_esp32_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	const struct i2s_esp32_cfg *const cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	i2s_hal_config_t *const hal_config = &dev_data->hal_cfg;

	if (i2s_cfg->channels != 2) {
		/* Only two channels supported */
		return -ENOSYS;
	}

	i2s_hal_init(&dev_data->hal_ctx, cfg->i2s_num);
	i2s_hal_enable_module_clock(&dev_data->hal_ctx);

	memset(hal_config, 0, sizeof(i2s_hal_config_t));

	/* For words greater than 16-bit the channel length is considered 32-bit */
	const uint32_t channel_length = i2s_cfg->word_size > 16U ? 32U : 16U;
	hal_config->total_chan = i2s_cfg->channels;
	hal_config->sample_bits = channel_length;
	/* chan_bits: default '0' means equal to 'sample_bits' */
	hal_config->chan_bits = hal_config->sample_bits;
	hal_config->sample_rate = i2s_cfg->frame_clk_freq;

	/* Works only because 2 channels max */
	hal_config->chan_fmt = I2S_CHANNEL_FMT_RIGHT_LEFT;
	hal_config->chan_mask = I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1;
	hal_config->active_chan = 2;

	/*
	 * comply with the i2s_config driver remark:
	 * When I2S data format is selected parameter channels is ignored,
	 * number of words in a frame is always 2.
	 */
	struct stream *stream;
	int ret;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
		hal_config->mode |= I2S_MODE_RX;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
		hal_config->mode |= I2S_MODE_TX;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	if (stream->state != I2S_STATE_NOT_READY && stream->state != I2S_STATE_READY) {
		LOG_ERR("invalid state");
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_OPT_FRAME_CLK_SLAVE ||
	    i2s_cfg->options & I2S_OPT_BIT_CLK_SLAVE) {
		stream->master = false;
	} else {
		stream->master = true;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		stream->queue_drop(stream);
		memset(&stream->cfg, 0, sizeof(struct i2s_config));
		stream->state = I2S_STATE_NOT_READY;
		return 0;
	}

	switch (i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		hal_config->comm_fmt = I2S_COMM_FORMAT_STAND_I2S;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		hal_config->comm_fmt = I2S_COMM_FORMAT_STAND_PCM_SHORT;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		hal_config->comm_fmt = I2S_COMM_FORMAT_STAND_PCM_LONG;
		break;
	default:
		LOG_ERR("I2S format not supported\n");
		return -ENOSYS;
	}

	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));

	/* set I2S Master Clock output in the MCK pin, enabled in the DT */
	if (stream->master) {
		hal_config->mode |= I2S_MODE_MASTER;
	} else {
		hal_config->mode |= I2S_MODE_SLAVE;
	}

	/* To get sclk, mclk, mclk_div bclk and bclk_div */
	ret = i2s_calculate_common_clock(&dev_data->hal_cfg, &dev_data->hal_ctx,
					 &dev_data->clk_cfg);
	if (ret != 0) {
		return -EINVAL;
	}

	i2s_hal_config_param(&dev_data->hal_ctx, hal_config);

	if (dir == I2S_DIR_RX) {
		i2s_hal_rx_clock_config(&dev_data->hal_ctx, &dev_data->clk_cfg);
		i2s_hal_set_rx_sample_bit(&dev_data->hal_ctx, hal_config->chan_bits,
					  hal_config->sample_bits);
		i2s_hal_rx_set_channel_style(&dev_data->hal_ctx, hal_config);
	} else if (dir == I2S_DIR_TX) {
		i2s_hal_tx_clock_config(&dev_data->hal_ctx, &dev_data->clk_cfg);
		i2s_hal_set_tx_sample_bit(&dev_data->hal_ctx, hal_config->chan_bits,
					  hal_config->sample_bits);
		i2s_hal_tx_set_channel_style(&dev_data->hal_ctx, hal_config);
	}

	stream->state = I2S_STATE_READY;
	return 0;
}

static int i2s_esp32_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	struct stream *stream;
	unsigned int key;
	int ret;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("Either RX or TX direction must be selected");
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d", stream->state);
			return -EIO;
		}

		__ASSERT_NO_MSG(stream->mem_block == NULL);

		ret = stream->stream_start(stream, dev);
		if (ret < 0) {
			LOG_ERR("START trigger failed %d", ret);
			return ret;
		}

		stream->state = I2S_STATE_RUNNING;
		stream->last_block = false;
		break;

	case I2S_TRIGGER_STOP:
		key = irq_lock();
		if (stream->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			LOG_ERR("STOP trigger: invalid state");
			return -EIO;
		}
		irq_unlock(key);
		stream->stream_disable(stream, dev);
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;
		stream->last_block = true;
		break;

	case I2S_TRIGGER_DRAIN:
		key = irq_lock();
		if (stream->state != I2S_STATE_RUNNING) {
			irq_unlock(key);
			LOG_ERR("DRAIN trigger: invalid state");
			return -EIO;
		}
		stream->stream_disable(stream, dev);
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;
		irq_unlock(key);
		break;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP trigger: invalid state");
			return -EIO;
		}
		stream->stream_disable(stream, dev);
		stream->queue_drop(stream);
		stream->state = I2S_STATE_READY;
		break;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE trigger: invalid state");
			return -EIO;
		}
		stream->state = I2S_STATE_READY;
		stream->queue_drop(stream);
		break;

	default:
		LOG_ERR("Unsupported trigger command");
		return -EINVAL;
	}

	return 0;
}

static int i2s_esp32_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	int ret;

	if (dev_data->rx.state == I2S_STATE_NOT_READY) {
		return -EIO;
	}

	if (dev_data->rx.state != I2S_STATE_ERROR) {
		ret = k_sem_take(&dev_data->rx.sem, SYS_TIMEOUT_MS(dev_data->rx.cfg.timeout));
		if (ret < 0) {
			return ret;
		}
	}

	/* Get data from the beginning of RX queue */
	ret = queue_get(&dev_data->rx.mem_block_queue, mem_block, size);
	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

static int i2s_esp32_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	int ret;

	if (dev_data->tx.state != I2S_STATE_RUNNING && dev_data->tx.state != I2S_STATE_READY) {
		return -EIO;
	}

	ret = k_sem_take(&dev_data->tx.sem, SYS_TIMEOUT_MS(dev_data->tx.cfg.timeout));
	if (ret < 0) {
		return ret;
	}

	/* Add data to the end of the TX queue */
	queue_put(&dev_data->tx.mem_block_queue, mem_block, size);

	return 0;
}

static const struct i2s_driver_api i2s_esp32_driver_api = {
	.configure = i2s_esp32_configure,
	.read = i2s_esp32_read,
	.write = i2s_esp32_write,
	.trigger = i2s_esp32_trigger,
};

#define ESP32_DMA_NUM_CHANNELS 8
static const struct device *active_dma_rx_channel[ESP32_DMA_NUM_CHANNELS];
static const struct device *active_dma_tx_channel[ESP32_DMA_NUM_CHANNELS];

static int reload_dma(const struct device *dev_dma, uint32_t channel, struct dma_config *dcfg,
		      void *src, void *dst, uint32_t blk_size)
{
	int ret;

	ret = dma_reload(dev_dma, channel, (uint32_t)src, (uint32_t)dst, blk_size);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(dev_dma, channel);

	return ret;
}

static int start_dma(const struct device *dev_dma, uint32_t channel, struct dma_config *dcfg,
		     void *src, void *dst, uint8_t fifo_threshold, uint32_t blk_size)
{
	struct dma_block_config blk_cfg;
	int ret;

	memset(&blk_cfg, 0, sizeof(blk_cfg));
	blk_cfg.block_size = blk_size;
	blk_cfg.source_address = (uint32_t)src;
	blk_cfg.dest_address = (uint32_t)dst;

	blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk_cfg.fifo_mode_control = fifo_threshold;

	dcfg->head_block = &blk_cfg;

	ret = dma_config(dev_dma, channel, dcfg);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(dev_dma, channel);

	return ret;
}

static const struct device *get_dev_from_rx_dma_channel(uint32_t dma_channel);
static const struct device *get_dev_from_tx_dma_channel(uint32_t dma_channel);
static void rx_stream_disable(struct stream *stream, const struct device *dev);
static void tx_stream_disable(struct stream *stream, const struct device *dev);

/* This function is executed in the interrupt context */
static void dma_rx_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	const struct device *dev = get_dev_from_rx_dma_channel(channel);
	const struct i2s_esp32_cfg *cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct stream *stream = &dev_data->rx;
	void *mblk_tmp;
	int ret;

	if (status < 0) {
		ret = -EIO;
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	__ASSERT_NO_MSG(stream->mem_block != NULL);

	/* Stop reception if there was an error */
	if (stream->state == I2S_STATE_ERROR) {
		goto rx_disable;
	}

	mblk_tmp = stream->mem_block;

	/* Prepare to receive the next data block */
	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}

	// TODO get dynamic address of the I2S module
	ret = reload_dma(stream->dev_dma, stream->dma_channel, &stream->dma_cfg, I2S0_ADDR,
			 stream->mem_block, stream->cfg.block_size);
	if (ret < 0) {
		goto rx_disable;
	}

	/* All block data received */
	ret = queue_put(&stream->mem_block_queue, mblk_tmp, stream->cfg.block_size);
	if (ret < 0) {
		stream->state = I2S_STATE_ERROR;
		goto rx_disable;
	}
	k_sem_give(&stream->sem);

	/* Stop reception if we were requested */
	if (stream->state == I2S_STATE_STOPPING) {
		stream->state = I2S_STATE_READY;
		goto rx_disable;
	}

	return;

rx_disable:
	rx_stream_disable(stream, dev);
}

static void dma_tx_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	const struct device *dev = get_dev_from_tx_dma_channel(channel);
	const struct i2s_esp32_cfg *cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct stream *stream = &dev_data->tx;
	size_t mem_block_size;
	int ret;

	if (status < 0) {
		ret = -EIO;
		stream->state = I2S_STATE_ERROR;
		goto tx_disable;
	}

	__ASSERT_NO_MSG(stream->mem_block != NULL);

	/* All block data sent */
	k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
	stream->mem_block = NULL;

	/* Stop transmission if there was an error */
	if (stream->state == I2S_STATE_ERROR) {
		LOG_ERR("TX error detected");
		goto tx_disable;
	}

	/* Stop transmission if we were requested */
	if (stream->last_block) {
		stream->state = I2S_STATE_READY;
		goto tx_disable;
	}

	/* Prepare to send the next data block */
	ret = queue_get(&stream->mem_block_queue, &stream->mem_block, &mem_block_size);
	if (ret < 0) {
		if (stream->state == I2S_STATE_STOPPING) {
			stream->state = I2S_STATE_READY;
		} else {
			stream->state = I2S_STATE_ERROR;
		}
		goto tx_disable;
	}
	k_sem_give(&stream->sem);

	ret = reload_dma(stream->dev_dma, stream->dma_channel, &stream->dma_cfg, stream->mem_block,
			 I2S0_ADDR, stream->cfg.block_size);
	if (ret < 0) {
		goto tx_disable;
	}

	return;

tx_disable:
	tx_stream_disable(stream, dev);
}

static uint32_t i2s_esp32_irq_count;
static uint32_t i2s_esp32_irq_ovr_count;

static void i2s_esp32_isr(const struct device *dev)
{
	const struct i2s_esp32_cfg *cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	struct stream *stream = &dev_data->rx;

	stream->state = I2S_STATE_ERROR;

	// TODO configure interrupt
	/* OVR error must be explicitly cleared */
	/*if (LL_I2S_IsActiveFlag_OVR(cfg->i2s)) {
		i2s_esp32_irq_ovr_count++;
		LL_I2S_ClearFlag_OVR(cfg->i2s);
	}*/

	i2s_esp32_irq_count++;
}

static int i2s_esp32_initialize(const struct device *dev)
{
	const struct i2s_esp32_cfg *cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	int ret, i;

	/* Enable I2S clock propagation */
	ret = i2s_esp32_enable_clock(dev);
	if (ret < 0) {
		LOG_ERR("%s: clock enabling failed: %d", __func__, ret);
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("I2S pinctrl setup failed (%d)", ret);
		return ret;
	}

	cfg->irq_config(dev);

	ret = k_sem_init(&dev_data->rx.sem, 0, I2S_ESP32_RX_BLOCK_COUNT);
	if (ret != 0) {
		return ret;
	}
	ret = k_sem_init(&dev_data->tx.sem, I2S_ESP32_TX_BLOCK_COUNT, I2S_ESP32_TX_BLOCK_COUNT);
	if (ret != 0) {
		return ret;
	}

	for (i = 0; i < ESP32_DMA_NUM_CHANNELS; i++) {
		active_dma_rx_channel[i] = NULL;
		active_dma_tx_channel[i] = NULL;
	}

	/* Get the binding to the DMA device */
	if (!device_is_ready(dev_data->tx.dev_dma)) {
		LOG_ERR("%s device not ready", dev_data->tx.dev_dma->name);
		return -ENODEV;
	}
	if (!device_is_ready(dev_data->rx.dev_dma)) {
		LOG_ERR("%s device not ready", dev_data->rx.dev_dma->name);
		return -ENODEV;
	}

	LOG_INF("%s inited", dev->name);

	return 0;
}

static int rx_stream_start(struct stream *stream, const struct device *dev)
{
	struct i2s_esp32_data *const dev_data = dev->data;
	i2s_hal_context_t *hal_ctx = &dev_data->hal_ctx;
	int ret;

	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	i2s_hal_stop_rx(hal_ctx);
	i2s_hal_reset_rx(hal_ctx);
	i2s_hal_reset_rx_fifo(hal_ctx);

	/* remember active RX DMA channel (used in callback) */
	active_dma_rx_channel[stream->dma_channel] = dev;

	ret = start_dma(stream->dev_dma, stream->dma_channel, &stream->dma_cfg, I2S0_ADDR,
			stream->mem_block, stream->fifo_threshold, stream->cfg.block_size);
	if (ret < 0) {
		LOG_ERR("Failed to start RX DMA transfer: %d", ret);
		return ret;
	}

	i2s_hal_start_rx(hal_ctx);

	return 0;
}

static int tx_stream_start(struct stream *stream, const struct device *dev)
{

	struct i2s_esp32_data *const dev_data = dev->data;
	i2s_hal_context_t *hal_ctx = &dev_data->hal_ctx;
	size_t mem_block_size;
	int ret;

	ret = queue_get(&stream->mem_block_queue, &stream->mem_block, &mem_block_size);
	if (ret < 0) {
		return ret;
	}
	k_sem_give(&stream->sem);

	i2s_hal_stop_tx(hal_ctx);
	i2s_hal_reset_tx(hal_ctx);
	i2s_hal_reset_tx_fifo(hal_ctx);

	/* remember active TX DMA channel (used in callback) */
	active_dma_tx_channel[stream->dma_channel] = dev;

	ret = start_dma(stream->dev_dma, stream->dma_channel, &stream->dma_cfg, stream->mem_block,
			I2S0_ADDR, stream->fifo_threshold, stream->cfg.block_size);
	if (ret < 0) {
		LOG_ERR("Failed to start TX DMA transfer: %d", ret);
		return ret;
	}

	i2s_hal_start_tx(hal_ctx);

	return 0;
}

static void rx_stream_disable(struct stream *stream, const struct device *dev)
{
	const struct i2s_esp32_cfg *cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	i2s_hal_context_t *hal_ctx = &dev_data->hal_ctx;

	// TODO set DMA interrupt
	//	LL_I2S_DisableDMAReq_RX(cfg->i2s);
	//	LL_I2S_DisableIT_ERR(cfg->i2s);

	dma_stop(stream->dev_dma, stream->dma_channel);
	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
		stream->mem_block = NULL;
	}

	i2s_hal_stop_rx(hal_ctx);

	active_dma_rx_channel[stream->dma_channel] = NULL;
}

static void tx_stream_disable(struct stream *stream, const struct device *dev)
{
	const struct i2s_esp32_cfg *cfg = dev->config;
	struct i2s_esp32_data *const dev_data = dev->data;
	i2s_hal_context_t *hal_ctx = &dev_data->hal_ctx;

	// TODO set DMA interrupt
	//	LL_I2S_DisableDMAReq_TX(cfg->i2s);
	//	LL_I2S_DisableIT_ERR(cfg->i2s);

	dma_stop(stream->dev_dma, stream->dma_channel);
	if (stream->mem_block != NULL) {
		k_mem_slab_free(stream->cfg.mem_slab, &stream->mem_block);
		stream->mem_block = NULL;
	}

	i2s_hal_stop_rx(hal_ctx);

	active_dma_tx_channel[stream->dma_channel] = NULL;
}

static void rx_queue_drop(struct stream *stream)
{
	size_t size;
	void *mem_block;

	while (queue_get(&stream->mem_block_queue, &mem_block, &size) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, &mem_block);
	}

	k_sem_reset(&stream->sem);
}

static void tx_queue_drop(struct stream *stream)
{
	size_t size;
	void *mem_block;
	unsigned int n = 0U;

	while (queue_get(&stream->mem_block_queue, &mem_block, &size) == 0) {
		k_mem_slab_free(stream->cfg.mem_slab, &mem_block);
		n++;
	}

	for (; n > 0; n--) {
		k_sem_give(&stream->sem);
	}
}

static const struct device *get_dev_from_rx_dma_channel(uint32_t dma_channel)
{
	return active_dma_rx_channel[dma_channel];
}

static const struct device *get_dev_from_tx_dma_channel(uint32_t dma_channel)
{
	return active_dma_tx_channel[dma_channel];
}

#define I2S(idx) DT_NODELABEL(i2s##idx)

/* src_dev and dest_dev should be 'MEMORY' or 'PERIPHERAL'. */
#define I2S_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)                               \
	.dir = {.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                   \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),                     \
		.dma_cfg =                                                                         \
			{                                                                          \
				.block_count = 2,                                                  \
				.dma_slot = GDMA_TRIG_PERIPH_I2S##index,                           \
				.channel_direction = src_dev##_TO_##dest_dev,                      \
				.source_data_size = 2,    /* 16bit default */                      \
				.dest_data_size = 2,      /* 16bit default */                      \
				.source_burst_length = 1, /* SINGLE transfer */                    \
				.dest_burst_length = 1,                                            \
				.channel_priority = 1,                                             \
				.dma_callback = dma_##dir##_callback,                              \
			},                                                                         \
		.fifo_threshold = 1,                                                               \
		.stream_start = dir##_stream_start,                                                \
		.stream_disable = dir##_stream_disable,                                            \
		.queue_drop = dir##_queue_drop,                                                    \
		.mem_block_queue.buf = dir##_##index##_ring_buf,                                   \
		.mem_block_queue.len = ARRAY_SIZE(dir##_##index##_ring_buf)}

#define I2S_ESP32_INIT(index)                                                                      \
                                                                                                   \
	static void i2s_esp32_irq_config_func_##index(const struct device *dev);                   \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct i2s_esp32_cfg i2s_esp32_config_##index = {                             \
		.irq_config = i2s_esp32_irq_config_func_##index,                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(I2S(index))),                            \
		.clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(I2S(index), offset),        \
		.i2s_num = index,                                                                  \
	};                                                                                         \
                                                                                                   \
	struct queue_item rx_##index##_ring_buf[I2S_ESP32_RX_BLOCK_COUNT + 1];                     \
	struct queue_item tx_##index##_ring_buf[I2S_ESP32_TX_BLOCK_COUNT + 1];                     \
                                                                                                   \
	static struct i2s_esp32_data i2s_esp32_data_##index = {                                    \
		UTIL_AND(DT_INST_DMAS_HAS_NAME(index, rx),                                         \
			 I2S_DMA_CHANNEL_INIT(index, rx, RX, PERIPHERAL, MEMORY)),                 \
		UTIL_AND(DT_INST_DMAS_HAS_NAME(index, tx),                                         \
			 I2S_DMA_CHANNEL_INIT(index, tx, TX, MEMORY, PERIPHERAL)),                 \
		.hal_ctx =                                                                         \
			{                                                                          \
				.dev = (i2s_dev_t *)DT_REG_ADDR(I2S(index)),                       \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, &i2s_esp32_initialize, NULL, &i2s_esp32_data_##index,         \
			      &i2s_esp32_config_##index, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,    \
			      &i2s_esp32_driver_api);                                              \
                                                                                                   \
	static void i2s_esp32_irq_config_func_##index(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2s_esp32_isr,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

DT_INST_FOREACH_STATUS_OKAY(I2S_ESP32_INIT)
