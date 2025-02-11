/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include "i2s_litex.h"
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(i2s_litex);

#define MODULO_INC(val, max)                                                   \
	{					                               \
		val = (val == max - 1) ? 0 : val + 1;                          \
	}

/**
 * @brief Enable i2s device
 *
 * @param reg base register of device
 */
static void i2s_enable(uintptr_t reg)
{
	uint8_t reg_data = litex_read8(reg + I2S_CONTROL_OFFSET);

	litex_write8(reg_data | I2S_ENABLE, reg + I2S_CONTROL_OFFSET);
}

/**
 * @brief Disable i2s device
 *
 * @param reg base register of device
 */
static void i2s_disable(uintptr_t reg)
{
	uint8_t reg_data = litex_read8(reg + I2S_CONTROL_OFFSET);

	litex_write8(reg_data & ~(I2S_ENABLE), reg + I2S_CONTROL_OFFSET);
}

/**
 * @brief Reset i2s fifo
 *
 * @param reg base register of device
 */
static void i2s_reset_fifo(uintptr_t reg)
{
	uint8_t reg_data = litex_read8(reg + I2S_CONTROL_OFFSET);

	litex_write8(reg_data | I2S_FIFO_RESET, reg + I2S_CONTROL_OFFSET);
}

/**
 * @brief Get i2s format handled by device
 *
 * @param reg base register of device
 *
 * @return currently supported format or error
 *			when format can't be handled
 */
static i2s_fmt_t i2s_get_foramt(uintptr_t reg)
{
	uint8_t reg_data = litex_read32(reg + I2S_CONFIG_OFFSET);

	reg_data &= I2S_CONF_FORMAT_MASK;
	if (reg_data == LITEX_I2S_STANDARD) {
		return I2S_FMT_DATA_FORMAT_I2S;
	} else if (reg_data == LITEX_I2S_LEFT_JUSTIFIED) {
		return I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED;
	}
	return -EINVAL;
}

/**
 * @brief Get i2s sample width handled by device
 *
 * @param reg base register of device
 *
 * @return i2s sample width in bits
 */
static uint32_t i2s_get_sample_width(uintptr_t reg)
{
	uint32_t reg_data = litex_read32(reg + I2S_CONFIG_OFFSET);

	reg_data &= I2S_CONF_SAMPLE_WIDTH_MASK;
	return reg_data >> I2S_CONF_SAMPLE_WIDTH_OFFSET;
}

/**
 * @brief Get i2s audio sampling rate handled by device
 *
 * @param reg base register of device
 *
 * @return audio sampling rate in Hz
 */
static uint32_t i2s_get_audio_freq(uintptr_t reg)
{
	uint32_t reg_data = litex_read32(reg + I2S_CONFIG_OFFSET);

	reg_data &= I2S_CONF_LRCK_MASK;
	return reg_data >> I2S_CONF_LRCK_FREQ_OFFSET;
}

/**
 * @brief Enable i2s interrupt in event register
 *
 * @param reg base register of device
 * @param irq_type irq type to be enabled one of I2S_EV_READY or I2S_EV_ERROR
 */
static void i2s_irq_enable(uintptr_t reg, int irq_type)
{
	__ASSERT_NO_MSG(irq_type == I2S_EV_READY || irq_type == I2S_EV_ERROR);

	uint8_t reg_data = litex_read8(reg + I2S_EV_ENABLE_OFFSET);

	litex_write8(reg_data | irq_type, reg + I2S_EV_ENABLE_OFFSET);
}

/**
 * @brief Disable i2s interrupt in event register
 *
 * @param reg base register of device
 * @param irq_type irq type to be disabled one of I2S_EV_READY or I2S_EV_ERROR
 */
static void i2s_irq_disable(uintptr_t reg, int irq_type)
{
	__ASSERT_NO_MSG(irq_type == I2S_EV_READY || irq_type == I2S_EV_ERROR);

	uint8_t reg_data = litex_read8(reg + I2S_EV_ENABLE_OFFSET);

	litex_write8(reg_data & ~(irq_type), reg + I2S_EV_ENABLE_OFFSET);
}

/**
 * @brief Clear all pending irqs
 *
 * @param reg base register of device
 */
static void i2s_clear_pending_irq(uintptr_t reg)
{
	uint8_t reg_data = litex_read8(reg + I2S_EV_PENDING_OFFSET);

	litex_write8(reg_data, reg + I2S_EV_PENDING_OFFSET);
}

/**
 * @brief Fast data copy function
 *
 * Each operation copies 32 bit data chunks
 * This function copies data from fifo into user buffer
 *
 * @param dst memory destination where fifo data will be copied to
 * @param size amount of data to be copied
 * @param sample_width width of single sample in bits
 * @param channels number of received channels
 */
static void i2s_copy_from_fifo(uint8_t *dst, size_t size, int sample_width,
			       int channels)
{
	uint32_t data;
	int chan_size = sample_width / 8;
#if CONFIG_I2S_LITEX_CHANNELS_CONCATENATED
	if (channels == 2) {
		for (size_t i = 0; i < size / chan_size; i += 4) {
			/* using sys_read function, as fifo is not a csr,
			 * but a contiguous memory space
			 */
			*(dst + i) = sys_read32(I2S_RX_FIFO_ADDR);
		}
	} else {
		for (size_t i = 0; i < size / chan_size; i += 2) {
			data = sys_read32(I2S_RX_FIFO_ADDR);
			*((uint16_t *)(dst + i)) = data & 0xffff;
		}
	}
#else
	int max_off = chan_size - 1;

	for (size_t i = 0; i < size / chan_size; ++i) {
		data = sys_read32(I2S_RX_FIFO_ADDR);
		for (int off = max_off; off >= 0; off--) {
#if CONFIG_I2S_LITEX_DATA_BIG_ENDIAN
			*(dst + i * chan_size + (max_off - off)) =
				data >> 8 * off;
#else
			*(dst + i * chan_size + off) = data >> 8 * off;
#endif
		}
		/* if mono, copy every left channel
		 * right channel is discarded
		 */
		if (channels == 1) {
			sys_read32(I2S_RX_FIFO_ADDR);
		}
	}
#endif
}

/**
 * @brief Fast data copy function
 *
 * Each operation copies 32 bit data chunks
 * This function copies data from user buffer into fifo
 *
 * @param src memory from which data will be copied to fifo
 * @param size amount of data to be copied in bytes
 * @param sample_width width of single sample in bits
 * @param channels number of received channels
 */
static void i2s_copy_to_fifo(uint8_t *src, size_t size, int sample_width,
			     int channels)
{
	int chan_size = sample_width / 8;
#if CONFIG_I2S_LITEX_CHANNELS_CONCATENATED
	if (channels == 2) {
		for (size_t i = 0; i < size / chan_size; i += 4) {
			/* using sys_write function, as fifo is not a csr,
			 * but a contignous memory space
			 */
			sys_write32(*(src + i), I2S_TX_FIFO_ADDR);
		}
	} else {
		for (size_t i = 0; i < size / chan_size; i += 2) {
			sys_write32(*((uint16_t *)(src + i)), I2S_TX_FIFO_ADDR);
		}
	}
#else
	int max_off = chan_size - 1;
	uint32_t data;
	uint8_t *d_ptr = (uint8_t *)&data;

	for (size_t i = 0; i < size / chan_size; ++i) {
		for (int off = max_off; off >= 0; off--) {
#if CONFIG_I2S_LITEX_DATA_BIG_ENDIAN
			*(d_ptr + off) =
				*(src + i * chan_size + (max_off - off));
#else
			*(d_ptr + off) = *(src + i * chan_size + off);
#endif
		}
		sys_write32(data, I2S_TX_FIFO_ADDR);
		/* if mono send every left channel
		 * right channel will be same as left
		 */
		if (channels == 1) {
			sys_write32(data, I2S_TX_FIFO_ADDR);
		}
	}
#endif
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

static int i2s_litex_initialize(const struct device *dev)
{
	const struct i2s_litex_cfg *cfg = dev->config;
	struct i2s_litex_data *const dev_data = dev->data;

	k_sem_init(&dev_data->rx.sem, 0, CONFIG_I2S_LITEX_RX_BLOCK_COUNT);
	k_sem_init(&dev_data->tx.sem, CONFIG_I2S_LITEX_TX_BLOCK_COUNT - 1,
		   CONFIG_I2S_LITEX_TX_BLOCK_COUNT);

	cfg->irq_config(dev);
	return 0;
}

static int i2s_litex_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	struct i2s_litex_data *const dev_data = dev->data;
	const struct i2s_litex_cfg *const cfg = dev->config;
	struct stream *stream;
	int channels_concatenated = litex_read8(cfg->base + I2S_STATUS_OFFSET);
	int dev_audio_freq = i2s_get_audio_freq(cfg->base);
	int channel_div;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
		channels_concatenated &= I2S_RX_STAT_CHANNEL_CONCATENATED_MASK;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
		channels_concatenated &= I2S_TX_STAT_CHANNEL_CONCATENATED_MASK;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("either RX or TX direction must be selected");
		return -EINVAL;
	}

	if (stream->state != I2S_STATE_NOT_READY &&
	    stream->state != I2S_STATE_READY) {
		LOG_ERR("invalid state");
		return -EINVAL;
	}

	if (i2s_cfg->options & I2S_OPT_BIT_CLK_GATED) {
		LOG_ERR("invalid operating mode");
		return -EINVAL;
	}

	if (i2s_cfg->frame_clk_freq != dev_audio_freq) {
		LOG_WRN("invalid audio frequency sampling rate");
	}

	if (i2s_cfg->channels == 1) {
		channel_div = 2;
	} else if (i2s_cfg->channels == 2) {
		channel_div = 1;
	} else {
		LOG_ERR("invalid channels number");
		return -EINVAL;
	}
	int req_buf_s =
		(cfg->fifo_depth * (i2s_cfg->word_size / 8)) / channel_div;

	if (i2s_cfg->block_size < req_buf_s) {
		LOG_ERR("not enough space to allocate single buffer");
		LOG_ERR("fifo requires at least %i bytes", req_buf_s);
		return -EINVAL;
	} else if (i2s_cfg->block_size != req_buf_s) {
		LOG_WRN("the buffer is greater than required,"
			"only %"
			"i bytes of data are valid ",
			req_buf_s);
		/* The block_size field will be corrected to req_buf_s in the
		 * structure copied as stream configuration (see below).
		 */
	}

	int dev_sample_width = i2s_get_sample_width(cfg->base);

	if (i2s_cfg->word_size != 8U && i2s_cfg->word_size != 16U &&
	    i2s_cfg->word_size != 24U && i2s_cfg->word_size != 32U &&
	    i2s_cfg->word_size != dev_sample_width) {
		LOG_ERR("invalid word size");
		return -EINVAL;
	}

	int dev_format = i2s_get_foramt(cfg->base);

	if (dev_format != i2s_cfg->format) {
		LOG_ERR("unsupported I2S data format");
		return -EINVAL;
	}

#if CONFIG_I2S_LITEX_CHANNELS_CONCATENATED
#if CONFIG_I2S_LITEX_DATA_BIG_ENDIAN
	LOG_ERR("Big endian is not uspported "
			"when channels are conncatenated");
	return -EINVAL;
#endif
	if (channels_concatenated == 0) {
		LOG_ERR("invalid state. "
				"Your device is configured to send "
				"channels with padding. "
				"Please reconfigure driver");
		return -EINVAL;
	}

	if (i2s_cfg->word_size != 16) {
		LOG_ERR("invalid word size");
		return -EINVAL;
	}

#endif

	memcpy(&stream->cfg, i2s_cfg, sizeof(struct i2s_config));
	stream->cfg.block_size = req_buf_s;

	stream->state = I2S_STATE_READY;
	return 0;
}

static int i2s_litex_read(const struct device *dev, void **mem_block,
			  size_t *size)
{
	struct i2s_litex_data *const dev_data = dev->data;
	int ret;

	if (dev_data->rx.state == I2S_STATE_NOT_READY) {
		LOG_DBG("invalid state");
		return -ENOMEM;
	}
	/* just to implement timeout*/
	ret = k_sem_take(&dev_data->rx.sem,
			 SYS_TIMEOUT_MS(dev_data->rx.cfg.timeout));
	if (ret < 0) {
		return ret;
	}
	/* Get data from the beginning of RX queue */
	return queue_get(&dev_data->rx.mem_block_queue, mem_block, size);
}

static int i2s_litex_write(const struct device *dev, void *mem_block,
			   size_t size)
{
	struct i2s_litex_data *const dev_data = dev->data;
	const struct i2s_litex_cfg *cfg = dev->config;
	int ret;

	if (dev_data->tx.state != I2S_STATE_RUNNING &&
	    dev_data->tx.state != I2S_STATE_READY) {
		LOG_DBG("invalid state");
		return -EIO;
	}
	/* just to implement timeout */
	ret = k_sem_take(&dev_data->tx.sem,
			 SYS_TIMEOUT_MS(dev_data->tx.cfg.timeout));
	if (ret < 0) {
		return ret;
	}
	/* Add data to the end of the TX queue */
	ret = queue_put(&dev_data->tx.mem_block_queue, mem_block, size);
	if (ret < 0) {
		return ret;
	}

	if (dev_data->tx.state == I2S_STATE_READY) {
		i2s_irq_enable(cfg->base, I2S_EV_READY);
		dev_data->tx.state = I2S_STATE_RUNNING;
	}
	return ret;
}

static int i2s_litex_trigger(const struct device *dev, enum i2s_dir dir,
			     enum i2s_trigger_cmd cmd)
{
	struct i2s_litex_data *const dev_data = dev->data;
	const struct i2s_litex_cfg *const cfg = dev->config;
	struct stream *stream;

	if (dir == I2S_DIR_RX) {
		stream = &dev_data->rx;
	} else if (dir == I2S_DIR_TX) {
		stream = &dev_data->tx;
	} else if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	} else {
		LOG_ERR("either RX or TX direction must be selected");
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d",
				stream->state);
			return -EIO;
		}
		__ASSERT_NO_MSG(stream->mem_block == NULL);
		i2s_reset_fifo(cfg->base);
		i2s_enable(cfg->base);
		i2s_irq_enable(cfg->base, I2S_EV_READY);
		stream->state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING &&
		    stream->state != I2S_STATE_READY) {
			LOG_ERR("STOP trigger: invalid state");
			return -EIO;
		}
		i2s_disable(cfg->base);
		i2s_irq_disable(cfg->base, I2S_EV_READY);
		stream->state = I2S_STATE_READY;
		break;

	default:
		LOG_ERR("unsupported trigger command");
		return -EINVAL;
	}
	return 0;
}

static inline void clear_rx_fifo(const struct i2s_litex_cfg *cfg)
{
	for (int i = 0; i < I2S_RX_FIFO_DEPTH; i++) {
		sys_read32(I2S_RX_FIFO_ADDR);
	}
	i2s_clear_pending_irq(cfg->base);
}

static void i2s_litex_isr_rx(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct i2s_litex_cfg *cfg = dev->config;
	struct i2s_litex_data *data = dev->data;
	struct stream *stream = &data->rx;
	int ret;

	/* Prepare to receive the next data block */
	ret = k_mem_slab_alloc(stream->cfg.mem_slab, &stream->mem_block,
			       K_NO_WAIT);
	if (ret < 0) {
		clear_rx_fifo(cfg);
		return;
	}
	i2s_copy_from_fifo((uint8_t *)stream->mem_block, stream->cfg.block_size,
			   stream->cfg.word_size, stream->cfg.channels);
	i2s_clear_pending_irq(cfg->base);

	ret = queue_put(&stream->mem_block_queue, stream->mem_block,
			stream->cfg.block_size);
	if (ret < 0) {
		LOG_WRN("Couldn't copy data "
				"from RX fifo to the ring "
				"buffer (no space left) - "
				"dropping a frame");
		return;
	}

	k_sem_give(&stream->sem);
}

static void i2s_litex_isr_tx(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct i2s_litex_cfg *cfg = dev->config;
	struct i2s_litex_data *data = dev->data;
	size_t mem_block_size;
	struct stream *stream = &data->tx;
	int ret;

	ret = queue_get(&stream->mem_block_queue, &stream->mem_block,
			&mem_block_size);
	if (ret < 0) {
		i2s_irq_disable(cfg->base, I2S_EV_READY);
		stream->state = I2S_STATE_READY;
		return;
	}
	k_sem_give(&stream->sem);
	i2s_copy_to_fifo((uint8_t *)stream->mem_block, mem_block_size,
			 stream->cfg.word_size, stream->cfg.channels);
	i2s_clear_pending_irq(cfg->base);

	k_mem_slab_free(stream->cfg.mem_slab, stream->mem_block);
}

static const struct i2s_driver_api i2s_litex_driver_api = {
	.configure = i2s_litex_configure,
	.read = i2s_litex_read,
	.write = i2s_litex_write,
	.trigger = i2s_litex_trigger,
};

#define I2S_INIT(dir)                                                          \
									       \
	static struct queue_item rx_ring_buf[CONFIG_I2S_LITEX_RX_BLOCK_COUNT]; \
	static struct queue_item tx_ring_buf[CONFIG_I2S_LITEX_TX_BLOCK_COUNT]; \
									       \
	static struct i2s_litex_data i2s_litex_data_##dir = {                  \
		.dir.mem_block_queue.buf = dir##_ring_buf,                     \
		.dir.mem_block_queue.len =                                     \
			sizeof(dir##_ring_buf) / sizeof(struct queue_item),    \
	};                                                                     \
									       \
	static void i2s_litex_irq_config_func_##dir(const struct device *dev); \
									       \
	static struct i2s_litex_cfg i2s_litex_cfg_##dir = {                    \
		.base = DT_REG_ADDR(DT_NODELABEL(i2s_##dir)), \
		.fifo_base =                                                   \
			DT_REG_ADDR_BY_NAME(DT_NODELABEL(i2s_##dir), fifo),    \
		.fifo_depth = DT_PROP(DT_NODELABEL(i2s_##dir), fifo_depth),    \
		.irq_config = i2s_litex_irq_config_func_##dir                  \
	};                                                                     \
	DEVICE_DT_DEFINE(DT_NODELABEL(i2s_##dir), i2s_litex_initialize,        \
				NULL, &i2s_litex_data_##dir,		       \
				&i2s_litex_cfg_##dir, POST_KERNEL,             \
				CONFIG_I2S_INIT_PRIORITY,		       \
				&i2s_litex_driver_api);			       \
									       \
	static void i2s_litex_irq_config_func_##dir(const struct device *dev)  \
	{                                                                      \
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(i2s_##dir)),                  \
					DT_IRQ(DT_NODELABEL(i2s_##dir),	       \
						priority),		       \
					i2s_litex_isr_##dir,		       \
					DEVICE_DT_GET(DT_NODELABEL(i2s_##dir)), 0);\
		irq_enable(DT_IRQN(DT_NODELABEL(i2s_##dir)));                  \
	}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2s_rx), okay)
I2S_INIT(rx);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2s_tx), okay)
I2S_INIT(tx);
#endif
