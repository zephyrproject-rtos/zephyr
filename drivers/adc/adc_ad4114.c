/*
 * Copyright (c) 2024 Pierrick Curt
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(ADC_AD4114, CONFIG_ADC_LOG_LEVEL);

#define DT_DRV_COMPAT adi_ad4114_adc

#define AD4114_CMD_READ       0x40
#define AD4114_CMD_WRITE      0x0
#define AD4114_CHAN_NUMBER    16
#define AD4114_ADC_RESOLUTION 24U

enum ad4114_reg {
	AD4114_STATUS_REG = 0x00,
	AD4114_MODE_REG = 0x01,
	AD4114_IFMODE_REG = 0x02,
	AD4114_REGCHECK = 0x03,
	AD4114_DATA_REG = 0x04,
	AD4114_GPIOCON_REG = 0x06,
	AD4114_ID_REG = 0x07,
	AD4114_CHANNEL_0_REG = 0x10,
	AD4114_CHANNEL_1_REG = 0x11,
	AD4114_CHANNEL_2_REG = 0x12,
	AD4114_CHANNEL_3_REG = 0x13,
	AD4114_CHANNEL_4_REG = 0x14,
	AD4114_CHANNEL_5_REG = 0x15,
	AD4114_CHANNEL_6_REG = 0x16,
	AD4114_CHANNEL_7_REG = 0x17,
	AD4114_CHANNEL_8_REG = 0x18,
	AD4114_CHANNEL_9_REG = 0x19,
	AD4114_CHANNEL_10_REG = 0x1A,
	AD4114_CHANNEL_11_REG = 0x1B,
	AD4114_CHANNEL_12_REG = 0x1C,
	AD4114_CHANNEL_13_REG = 0x1D,
	AD4114_CHANNEL_14_REG = 0x1E,
	AD4114_CHANNEL_15_REG = 0x1F,
	AD4114_SETUPCON0_REG = 0x20,
	AD4114_SETUPCON1_REG = 0x21,
	AD4114_SETUPCON2_REG = 0x22,
	AD4114_SETUPCON3_REG = 0x23,
	AD4114_SETUPCON4_REG = 0x24,
	AD4114_SETUPCON5_REG = 0x25,
	AD4114_SETUPCON6_REG = 0x26,
	AD4114_SETUPCON7_REG = 0x27,
	AD4114_FILTCON0_REG = 0x28,
	AD4114_FILTCON1_REG = 0x29,
	AD4114_FILTCON2_REG = 0x2A,
	AD4114_FILTCON3_REG = 0x2B,
	AD4114_FILTCON4_REG = 0x2C,
	AD4114_FILTCON5_REG = 0x2D,
	AD4114_FILTCON6_REG = 0x2E,
	AD4114_FILTCON7_REG = 0x2F,
	AD4114_OFFSET0_REG = 0x30,
	AD4114_OFFSET1_REG = 0x31,
	AD4114_OFFSET2_REG = 0x32,
	AD4114_OFFSET3_REG = 0x33,
	AD4114_OFFSET4_REG = 0x34,
	AD4114_OFFSET5_REG = 0x35,
	AD4114_OFFSET6_REG = 0x36,
	AD4114_OFFSET7_REG = 0x37,
	AD4114_GAIN0_REG = 0x38,
	AD4114_GAIN1_REG = 0x39,
	AD4114_GAIN2_REG = 0x3A,
	AD4114_GAIN3_REG = 0x3B,
	AD4114_GAIN4_REG = 0x3C,
	AD4114_GAIN5_REG = 0x3D,
	AD4114_GAIN6_REG = 0x3E,
	AD4114_GAIN7_REG = 0x3F,
};

struct adc_ad4114_config {
	struct spi_dt_spec spi;
	uint16_t resolution;
	uint16_t map_input[AD4114_CHAN_NUMBER];
};

struct adc_ad4114_data {
	struct adc_context ctx;
	const struct device *dev;
	struct k_thread thread;
	struct k_sem sem;
	uint16_t channels;
	uint16_t channels_cfg;
	uint32_t *buffer;
	uint32_t *repeat_buffer;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_AD4114_ACQUISITION_THREAD_STACK_SIZE);
};

static int ad4114_write_reg(const struct device *dev, enum ad4114_reg reg_addr, uint8_t *buffer,
			    size_t reg_size)
{
	int ret;
	const struct adc_ad4114_config *config = dev->config;
	uint8_t buffer_tx[5] = {0}; /* One byte command, max 4 bytes data */

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	buffer_tx[0] = AD4114_CMD_WRITE | reg_addr;

	if (reg_size > 4) {
		LOG_ERR("Invalid size, max data write size is 4");
		return -ENOMEM;
	}
	/* Fill the data */
	for (uint8_t index = 0; index < reg_size; index++) {
		buffer_tx[1 + index] = buffer[index];
	}

	ret = spi_write_dt(&config->spi, &tx);
	if (ret != 0) {
		LOG_ERR("%s: error writing register 0x%X (%d)", dev->name, reg_addr, ret);
		return ret;
	}

	return ret;
}

static int ad4114_read_reg(const struct device *dev, enum ad4114_reg reg_addr, uint8_t *buffer,
			   size_t reg_size)
{
	int ret;
	const struct adc_ad4114_config *config = dev->config;

	uint8_t buffer_tx[6] = {0};
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)] = {0xFF};
	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};
	buffer_tx[0] = AD4114_CMD_READ | reg_addr;

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret != 0) {
		LOG_ERR("%s: error reading register 0x%X (%d)", dev->name, reg_addr, ret);
		return ret;
	}

	/* Copy received data in output buffer */
	for (uint8_t index = 0; index < reg_size; index++) {
		buffer[index] = buffer_rx[index + 1];
	}

	return ret;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ad4114_data *data = CONTAINER_OF(ctx, struct adc_ad4114_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ad4114_data *data = CONTAINER_OF(ctx, struct adc_ad4114_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_ad4114x_validate_buffer_size(const struct device *dev,
					    const struct adc_sequence *sequence)
{
	uint8_t channels;
	size_t needed;

	channels = POPCOUNT(sequence->channels);
	needed = channels * sizeof(uint32_t);

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int adc_ad4114_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_ad4114_data *data = dev->data;
	const struct adc_ad4114_config *config = dev->config;
	int ret;
	uint8_t write_reg[2];
	uint8_t status;

	ret = adc_ad4114x_validate_buffer_size(dev, sequence);
	if (ret < 0) {
		LOG_ERR("insufficient buffer size");
		return ret;
	}

	data->channels_cfg = sequence->channels;
	for (uint32_t i = 0U; i < AD4114_CHAN_NUMBER; i++) {
		if ((BIT(i) & sequence->channels) != 0) {
			write_reg[0] = 0x80 | (uint8_t)((config->map_input[i] >> 8) & 0xFF);
			write_reg[1] = (uint8_t)(config->map_input[i] & 0xFF);
			LOG_DBG("Enable channel %d with mapping %X %X, raw %X", i, write_reg[0],
				write_reg[1], config->map_input[i]);
			ad4114_write_reg(dev, AD4114_CHANNEL_0_REG + i, write_reg, 2);
		} else {
			LOG_DBG("Disable channel %d", i);
			write_reg[0] = 0x0;
			write_reg[1] = 0x0;
			ad4114_write_reg(dev, AD4114_CHANNEL_0_REG + i, write_reg, 2);
		}
	}

	/* Configure the buffer */
	data->buffer = sequence->buffer;

	while ((status & 0x80) != 0x80) {
		/* Wait for acquiistion start */
		ad4114_read_reg(dev, AD4114_STATUS_REG, &status, 1);
		/* Wait 10us between two status read */
		k_usleep(10);
	}

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static void adc_ad4114_acquisition_thread(struct adc_ad4114_data *data)
{
	uint8_t value[4] = {0};
	uint32_t buffer_values[AD4114_CHAN_NUMBER];
	bool is_ended = false;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		while (data->channels != 0) {
			ad4114_read_reg(data->dev, AD4114_DATA_REG, value, 4);
			/* Check the read channel */
			if ((value[3] & 0xF0) != 0) {
				LOG_DBG("Error read on :  %X ", value[3]);
			} else {
				LOG_DBG("Success read on %d: value  %X ", value[3],
					(value[2] << 16 | value[1] << 8 | value[0]));
				/* success read, store it */
				buffer_values[value[3]] =
					(value[0] << 16 | value[1] << 8 | value[2]);
				WRITE_BIT(data->channels, value[3], 0);
				/* Disable the channel after read success */
				uint8_t write_reg[2] = {0};

				ad4114_write_reg(data->dev, AD4114_CHANNEL_0_REG + value[3],
						 write_reg, 2);
			}
			if (data->channels == 0) {
				is_ended = true;
			}
			/* Wait before next status ready check: the minimal acquisition time for a
			 * channel is 100us. So wait 10us betwen each check to avoid to use CPU for
			 * nothing.
			 */
			k_usleep(10);
		}

		if (is_ended) {
			is_ended = false;
			for (uint8_t i = 0U; i < AD4114_CHAN_NUMBER; i++) {
				if ((BIT(i) & data->channels_cfg) != 0) {
					*data->buffer++ = buffer_values[i];
					LOG_DBG("Read channel %d value :  %X ", i,
						buffer_values[i]);
				}
			}
			adc_context_on_sampling_done(&data->ctx, data->dev);
		}
		/* Wait 1ms before checking if a new sequence acquisition is asked */
		k_usleep(1000);
	}
}

static int adc_ad4114_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{

	/* Todo in the futur we can manage here :
	 * filters
	 * gain
	 * offsets
	 * special configuration : we can update map_input here to override the device
	 * tree setup
	 */
	if (channel_cfg->channel_id >= AD4114_CHAN_NUMBER) {
		LOG_ERR("invalid channel id %d", channel_cfg->channel_id);
		return -EINVAL;
	}
	return 0;
}

static int adc_ad4114_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc_ad4114_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = adc_ad4114_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_ad4114_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_ad4114_read_async(dev, sequence, NULL);
}

static int adc_ad4114_init(const struct device *dev)
{
	int err;
	const struct adc_ad4114_config *config = dev->config;
	struct adc_ad4114_data *data = dev->data;
	uint8_t id[2] = {0};
	uint8_t gain[3];
	uint8_t write_reg[2];
	uint8_t status = 0;
	k_tid_t tid;

	data->dev = dev;
	k_sem_init(&data->sem, 0, 1);
	adc_context_init(&data->ctx);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("spi bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	ad4114_read_reg(dev, AD4114_ID_REG, id, 2);
	/* Check that this is the expected ID : 0x30DX, where x is don’t care */
	if ((((id[0] << 8) | id[1]) & 0xFFF0) != 0x30D0) {
		LOG_ERR("Read wrong ID register 0x%X 0x%X", id[0], id[1]);
		return -EIO;
	}

	ad4114_read_reg(dev, AD4114_STATUS_REG, &status, 1);
	LOG_INF("Found AD4114 with status %d", status);

	/* Configure gain to 0x400000 */
	gain[0] = 0x40;
	gain[1] = 0x00;
	gain[2] = 0x00;
	ad4114_write_reg(dev, AD4114_GAIN0_REG, gain, 3);
	ad4114_write_reg(dev, AD4114_GAIN1_REG, gain, 3);

	/* Bit 6: DATA_STAT = 1 */
	write_reg[0] = 0x0;
	write_reg[1] = 0x40;
	ad4114_write_reg(dev, AD4114_IFMODE_REG, write_reg, 2);

	/* Bit 12: BI_UNIPOLARx = 0
	 * Bit 9:8 INBUFx = 11
	 */
	write_reg[0] = 0x3;
	write_reg[1] = 0x0;
	ad4114_write_reg(dev, AD4114_SETUPCON0_REG, write_reg, 2);

	/* Bit 12: BI_UNIPOLARx = 1
	 * Bit 9:8 INBUFx = 11
	 */
	write_reg[0] = 0x13;
	write_reg[1] = 0x0;
	ad4114_write_reg(dev, AD4114_SETUPCON1_REG, write_reg, 2);

	/* Bit 15: REF_EN = 1
	 * Bit 3:2: CLOCKSEL = 11
	 */
	write_reg[0] = 0x80;
	write_reg[1] = 0xC;
	ad4114_write_reg(dev, AD4114_MODE_REG, write_reg, 2);

	tid = k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
			      (k_thread_entry_t)adc_ad4114_acquisition_thread, data, NULL, NULL,
			      CONFIG_ADC_AD4114_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		err = k_thread_name_set(tid, "adc_ad4114");
		if (err < 0) {
			return err;
		}
	}

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static DEVICE_API(adc, adc_ad4114_api) = {
	.channel_setup = adc_ad4114_channel_setup,
	.read = adc_ad4114_read,
};

#define FILL_MAP_INPUTS(node_id, prop, idx)                                                        \
	{                                                                                          \
		.key_index = DT_NODE_CHILD_IDX(node_id),                                           \
		.press_mv = DT_PROP_BY_IDX(node_id, prop, idx),                                    \
	}

#define ADC_AD4114_DEVICE(inst)                                                                    \
	static struct adc_ad4114_data adc_ad4114_data_##inst;                                      \
	static const struct adc_ad4114_config adc_ad4114_config_##inst = {                         \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0),                             \
		.resolution = AD4114_ADC_RESOLUTION,                                               \
		.map_input = DT_INST_PROP(inst, map_inputs),                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, adc_ad4114_init, NULL, &adc_ad4114_data_##inst,                \
			      &adc_ad4114_config_##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,    \
			      &adc_ad4114_api)                                                     \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, map_inputs) == AD4114_CHAN_NUMBER);

DT_INST_FOREACH_STATUS_OKAY(ADC_AD4114_DEVICE)
