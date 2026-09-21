/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/sleep.h"
#include "zephyr/sys/util_macro.h"
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <sys/errno.h>

LOG_MODULE_REGISTER(adc_ade7913, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DT_DRV_COMPAT adi_ade7913

#define ADE7913_SPI_READ  BIT(2)
#define ADE7913_SPI_WRITE 0

#define ADE7913_CONFIG_CLKOUT_EN     BIT(0)
#define ADE7913_CONFIG_PWRDWN_EN     BIT(2)
#define ADE7913_CONFIG_TEMP_EN       BIT(3)
#define ADE7913_CONFIG_ADC_FREQ_MASK GENMASK(5, 4)
#define ADE7913_CONFIG_SWRST         BIT(6)
#define ADE7913_CONFIG_BW            BIT(7)

#define ADE7913_STATUS0_RESET_ON BIT(0)
#define ADE7913_STATUS0_CRC_STAT BIT(1)
#define ADE7913_STATUS0_IC_PROT  BIT(2)

#define ADE7913_LOCK_KEY   0xCA
#define ADE7913_UNLOCK_KEY 0x9C

#define ADE7913_SYNC_SNAP_SYNC BIT(0)
#define ADE7913_SYNC_SNAP_SNAP BIT(1)

#define ADE7913_STATUS1_VERSION_MASK GENMASK(2, 0)
#define ADE7913_STATUS1_ADC_NA       BIT(3)

typedef enum {
	ADE7913_IWV = 0x00 << 3,
	ADE7913_V1WV = 0x01 << 3,
	ADE7913_V2WV = 0x02 << 3,
	ADE7913_ADC_CRC = 0x04 << 3,
	ADE7913_CTRL_CRC = 0x05 << 3,
	ADE7913_CNT_SNAPSHOT = 0x07 << 3,
	ADE7913_CONFIG = 0x08 << 3,
	ADE7913_STATUS0 = 0x09 << 3,
	ADE7913_LOCK = 0x0A << 3,
	ADE7913_SYNC_SNAP = 0x0B << 3,
	ADE7913_COUNTER0 = 0x0C << 3,
	ADE7913_COUNTER1 = 0x0D << 3,
	ADE7913_EMI_CTRL = 0x0E << 3,
	ADE7913_STATUS1 = 0x0F << 3,
	ADE7913_TEMPOS = 0x18 << 3
} ADE7913_REGISTERS;

enum ADE7913_ADC_FREQS {
	ADE7913_ADC_FREQ_8K,
	ADE7913_ADC_FREQ_4K,
	ADE7913_ADC_FREQ_2K,
	ADE7913_ADC_FREQ_1K
};

#define ADE7913_INIT_TIMEOUT_MS 100

struct ade7913_config {
	struct spi_dt_spec spi;
	bool clk_out_enable;
	enum ADE7913_ADC_FREQS frequency;
};

struct ade7913_data {
	struct adc_context ctx;

	int32_t *buffer;
	int32_t *repeat_buffer;
	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_ADE7913_ACQUISITION_THREAD_STACK_SIZE);
};

static int ade7913_read_register_u8(const struct device *dev, ADE7913_REGISTERS reg_enum,
				    uint8_t *data)
{
	const struct ade7913_config *cfg = dev->config;
	uint8_t opcode = ADE7913_SPI_READ | reg_enum;
	uint8_t buffer[1] = {0};
	int ret;

	struct spi_buf tx_buf = {
		.buf = &opcode,
		.len = sizeof(opcode),
	};
	struct spi_buf_set tx_buf_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[] = {
		{
			.buf = NULL,
			.len = sizeof(opcode),
		},
		{
			.buf = buffer,
			.len = sizeof(buffer),
		},
	};
	struct spi_buf_set rx_buf_set = {
		.buffers = rx_buf,
		.count = 2,
	};

	ret = spi_transceive_dt(&cfg->spi, &tx_buf_set, &rx_buf_set);
	if (ret < 0) {
		return ret;
	}

	*data = *buffer;
	return 0;
}

static int ade7913_wait_ready(const struct device *dev)
{
	uint8_t status0;
	int ret;

	for (int elapsed = 0; elapsed < ADE7913_INIT_TIMEOUT_MS; elapsed++) {
		ret = ade7913_read_register_u8(dev, ADE7913_STATUS0, &status0);
		if (ret < 0) {
			return ret;
		}
		if ((status0 & ADE7913_STATUS0_RESET_ON) == 0) {
			return 0;
		}

		k_msleep(1);
	}
	return -ETIMEDOUT;
}

static int ade7913_write_register(const struct device *dev, ADE7913_REGISTERS reg_enum,
				  uint8_t regcode)
{
	const struct ade7913_config *cfg = dev->config;
	uint8_t opcode = ADE7913_SPI_WRITE | reg_enum;
	uint8_t tx_t_buf[2] = {0};
	int ret;

	tx_t_buf[0] = opcode;
	tx_t_buf[1] = regcode;
	struct spi_buf tx_buf = {.buf = tx_t_buf, .len = sizeof(tx_t_buf)};
	struct spi_buf_set tx_buf_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	ret = spi_write_dt(&cfg->spi, &tx_buf_set);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ade7913_channel_setup(const struct device *dev __unused,
				 const struct adc_channel_cfg *channel_cfg)
{
	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_VDD_1) {
		LOG_ERR("unsupported channel reference '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition time '%d'", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->channel_id >= 3) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->differential != 1) {
		LOG_ERR("unsupported single-end mode");
		return -ENOTSUP;
	}

	return 0;
}

static int ade7913_validate_sequence(const struct adc_sequence *sequence)
{
	uint8_t channels = POPCOUNT(sequence->channels);
	size_t needed = channels * sizeof(uint32_t);

	if (!IN_RANGE(channels, 1, 3)) {
		return -EINVAL;
	}

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ade7913_data *data = CONTAINER_OF(ctx, struct ade7913_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ade7913_data *data = CONTAINER_OF(ctx, struct ade7913_data, ctx);

	data->repeat_buffer = data->buffer;
	k_sem_give(&data->sem);
	LOG_DBG("start_sampling");
}

static int ade7913_acquisition_one(const struct device *dev)
{
	struct ade7913_data *data = dev->data;
	const struct ade7913_config *cfg = dev->config;
	uint8_t opcode = ADE7913_SPI_READ;
	uint8_t buffer[3];
	int ret;

	struct spi_buf tx_buf = {
		.buf = &opcode,
		.len = sizeof(opcode),
	};
	struct spi_buf_set tx_buf_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf rx_buf[] = {
		{
			.buf = NULL,
			.len = sizeof(opcode),
		},
		{
			.buf = buffer,
			.len = sizeof(buffer),
		},
	};
	struct spi_buf_set rx_buf_set = {
		.buffers = rx_buf,
		.count = 2,
	};

	switch (data->ctx.sequence.channels) {
	case BIT(0):
		opcode |= ADE7913_IWV;
		break;
	case BIT(1):
		opcode |= ADE7913_V1WV;
		break;
	case BIT(2):
		opcode |= ADE7913_V2WV;
		break;
	default:
		break;
	}

	ret = spi_transceive_dt(&cfg->spi, &tx_buf_set, &rx_buf_set);
	if (ret < 0) {
		return ret;
	}

	*data->buffer = sign_extend(sys_get_be24(&buffer[0]), 24 - 1);

	data->buffer++;
	return 0;
}

static int ade7913_acquisition_all(const struct device *dev)
{
	struct ade7913_data *data = dev->data;
	const struct ade7913_config *cfg = dev->config;
	uint8_t opcode = ADE7913_SPI_READ;
	uint8_t buffer[9];
	int ret;

	struct spi_buf tx_buf = {
		.buf = &opcode,
		.len = sizeof(opcode),
	};
	struct spi_buf_set tx_buf_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf rx_buf[] = {
		{
			.buf = NULL,
			.len = sizeof(opcode),
		},
		{
			.buf = buffer,
			.len = sizeof(buffer),
		},
	};
	struct spi_buf_set rx_buf_set = {
		.buffers = rx_buf,
		.count = 2,
	};

	ret = spi_transceive_dt(&cfg->spi, &tx_buf_set, &rx_buf_set);
	if (ret < 0) {
		return ret;
	}

	if (IS_BIT_SET(data->ctx.sequence.channels, 0)) {

		*data->buffer = sign_extend(sys_get_be24(&buffer[0]), 24 - 1);
		data->buffer++;
	}

	if (IS_BIT_SET(data->ctx.sequence.channels, 1)) {

		*data->buffer = sign_extend(sys_get_be24(&buffer[3]), 24 - 1);
		data->buffer++;
	}

	if (IS_BIT_SET(data->ctx.sequence.channels, 2)) {

		*data->buffer = sign_extend(sys_get_be24(&buffer[6]), 24 - 1);
		data->buffer++;
	}

	return 0;
}

static int ade7913_acquisition(const struct device *dev)
{
	struct ade7913_data *data = dev->data;

	if (POPCOUNT(data->ctx.sequence.channels) == 1) {
		return ade7913_acquisition_one(dev);
	} else {
		return ade7913_acquisition_all(dev);
	}
}

static int ade7913_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ade7913_data *data = dev->data;
	int ret;

	if (sequence->resolution != 24) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > 3) {
		LOG_ERR("unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		LOG_ERR("unsupported calibration");
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling not supported");
		return -ENOTSUP;
	}

	ret = ade7913_validate_sequence(sequence);
	if (ret < 0) {
		LOG_ERR("invalid sequence / buffer too small");
		return ret;
	}

	data->buffer = sequence->buffer;
	data->repeat_buffer = data->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int ade7913_read_async(const struct device *dev, const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct ade7913_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = ade7913_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int ade7913_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return ade7913_read_async(dev, sequence, NULL);
}

static void ade7913_acquisition_thread(void *p1, void *p2, void *p3 __unused)
{
	const struct device *dev = p1;
	struct ade7913_data *data = p2;
	int ret;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		ret = ade7913_acquisition(dev);
		if (ret < 0) {
			adc_context_complete(&data->ctx, ret);
			continue;
		}

		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int ade7913_update_reg(const struct device *dev, ADE7913_REGISTERS reg_enum,
			      uint8_t update_mask, uint8_t update_value)
{
	int ret;
	uint8_t data;

	ret = ade7913_read_register_u8(dev, reg_enum, &data);

	if (ret < 0) {
		return ret;
	}
	data &= ~update_mask;
	data |= update_value;

	return ade7913_write_register(dev, reg_enum, (uint8_t)data);
}

static int ade7913_set_adc_freq(const struct device *dev, enum ADE7913_ADC_FREQS frequency)
{
	return ade7913_update_reg(dev, ADE7913_CONFIG, ADE7913_CONFIG_ADC_FREQ_MASK,
				  FIELD_PREP(ADE7913_CONFIG_ADC_FREQ_MASK, frequency));
}

static int ade7913_set_clkout_en(const struct device *dev, bool clkout_en)
{

	return ade7913_update_reg(dev, ADE7913_CONFIG, ADE7913_CONFIG_CLKOUT_EN,
				  FIELD_PREP(ADE7913_CONFIG_CLKOUT_EN, clkout_en));
}

static int ade7913_init(const struct device *dev)
{
	const struct ade7913_config *config = dev->config;
	struct ade7913_data *data = dev->data;
	int ret;

	adc_context_init(&data->ctx);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR_DEVICE_NOT_READY(config->spi.bus);
		return -ENODEV;
	}
	ret = ade7913_wait_ready(dev);
	if (ret < 0) {
		return ret;
	}
	ret = ade7913_set_clkout_en(dev, config->clk_out_enable);
	if (ret < 0) {
		return ret;
	}
	ret = ade7913_set_adc_freq(dev, config->frequency);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&data->sem, 0, 1);

	k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
			ade7913_acquisition_thread, (void *)dev, data, NULL,
			CONFIG_ADC_ADE7913_ACQUISITION_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, ade7913_api) = {
	.channel_setup = ade7913_channel_setup,
	.read = ade7913_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ade7913_read_async,
#endif

};

#define ADE7913_SPI_OP (SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8))

#define ADE7913_INIT(n)                                                                            \
	static struct ade7913_data ade7913_data_##n;                                               \
	static const struct ade7913_config ade7913_cfg_##n = {                                     \
		.spi = SPI_DT_SPEC_INST_GET(n, ADE7913_SPI_OP),                                    \
		.clk_out_enable = DT_INST_PROP(n, clk_out_enable),                                 \
		.frequency = (enum ADE7913_ADC_FREQS)DT_INST_ENUM_IDX_OR(n, clock_freq, 0),        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ade7913_init, NULL, &ade7913_data_##n, &ade7913_cfg_##n,          \
			      POST_KERNEL, CONFIG_ADC_ADE7913_INIT_PRIORITY, &ade7913_api);

DT_INST_FOREACH_STATUS_OKAY(ADE7913_INIT)
