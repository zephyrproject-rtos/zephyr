/*
 * Copyright (c) 2025 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(ads1118, CONFIG_ADC_LOG_LEVEL);

#define ADS1118_CONFIG_OS(x)      FIELD_PREP(BIT(15), (x))
#define ADS1118_CONFIG_MUX(x)     FIELD_PREP(GENMASK(14, 12), (x))
#define ADS1118_CONFIG_PGA(x)     FIELD_PREP(GENMASK(11, 9), (x))
#define ADS1118_CONFIG_MODE(x)    FIELD_PREP(BIT(8), (x))
#define ADS1118_CONFIG_DR(x)      FIELD_PREP(GENMASK(7, 5), (x))
#define ADS1118_CONFIG_TS_MODE(x) FIELD_PREP(BIT(4), (x))
#define ADS1118_CONFIG_PULL_UP(x) FIELD_PREP(BIT(3), (x))
#define ADS1118_CONFIG_NOP(x)     FIELD_PREP(GENMASK(2, 1), (x))
#define ADS1118_CONFIG_CNV_RDY(x) FIELD_PREP(BIT(0), (x))

#define ADS1118_REF_INTERNAL 3000

/* The ADS111X provides 16 bits of data in binary two's complement format
 * A positive full-scale (+FS) input produces an output code of 7FFFh and a
 * negative full-scale (–FS) input produces an output code of 8000h. Single
 * ended signal measurements only use the positive code range from
 * 0000h to 7FFFh
 */
#define ADS1118_RESOLUTION        16
#define ADS1118_MAX_CHANNEL_COUNT 4

enum {
	ADS1118_CONFIG_OS_NO_EFFECT, /*!< ADS1118, OS mode no effect. */
	ADS1118_CONFIG_OS_START,     /*!< ADS1118, OS mode start conversion. */
};

enum {
	ADS1118_CONFIG_MUX_DIFF_0_1 = 0, /*!< AINP is AIN0, and AINN is AIN1 (default). */
	ADS1118_CONFIG_MUX_DIFF_0_3 = 1, /*!< AINP is AIN0, and AINN is AIN3. */
	ADS1118_CONFIG_MUX_DIFF_1_3 = 2, /*!< AINP is AIN1, and AINN is AIN3. */
	ADS1118_CONFIG_MUX_DIFF_2_3 = 3, /*!< AINP is AIN2, and AINN is AIN3. */
	ADS1118_CONFIG_MUX_SINGLE_0 = 4, /*!< AINP is AIN0, and AINN is GND. */
	ADS1118_CONFIG_MUX_SINGLE_1 = 5, /*!< AINP is AIN1, and AINN is GND. */
	ADS1118_CONFIG_MUX_SINGLE_2 = 6, /*!< AINP is AIN2, and AINN is GND. */
	ADS1118_CONFIG_MUX_SINGLE_3 = 7, /*!< AINP is AIN3, and AINN is GND. */
};

enum {
	ADS1118_CONFIG_DR_8_128 = 0,    /*!< 8, 128 samples per second. */
	ADS1118_CONFIG_DR_16_250 = 1,   /*!< 16, 250 samples per second. */
	ADS1118_CONFIG_DR_32_490 = 2,   /*!< 32, 490 samples per second. */
	ADS1118_CONFIG_DR_64_920 = 3,   /*!< 64, 920 samples per second. */
	ADS1118_CONFIG_DR_128_1600 = 4, /*!< 128, 1600 samples per second (default). */
	ADS1118_CONFIG_DR_250_2400 = 5, /*!< 250, 2400 samples per second. */
	ADS1118_CONFIG_DR_475_3300 = 6, /*!< 475, 3300 samples per second. */
	ADS1118_CONFIG_DR_860_3300 = 7, /*!< 860, 3300 samples per second. */
	ADS1118_CONFIG_DR_DEFAULT = ADS1118_CONFIG_DR_128_1600 /*!< Default data rate. */
};

enum {
	ADS1118_CONFIG_PGA_6144 = 0, /*!< +/-6.144V range = Gain 2/3. */
	ADS1118_CONFIG_PGA_4096 = 1, /*!< +/-4.096V range = Gain 1. */
	ADS1118_CONFIG_PGA_2048 = 2, /*!< +/-2.048V range = Gain 2 (default). */
	ADS1118_CONFIG_PGA_1024 = 3, /*!< +/-1.024V range = Gain 4. */
	ADS1118_CONFIG_PGA_512 = 4,  /*!< +/-0.512V range = Gain 8. */
	ADS1118_CONFIG_PGA_256 = 5   /*!< +/-0.256V range = Gain 16. */
};

enum {
	ADS1118_CONFIG_MODE_CONTINUOUS = 0,  /*!< Continuous conversion mode. */
	ADS1118_CONFIG_MODE_SINGLE_SHOT = 1, /*!< Single-shot mode (default). */
};

enum {
	ADS1118_CONFIG_MODE_TS_ADC = 0,  /*!< ADC mode (default). */
	ADS1118_CONFIG_MODE_TS_TEMP = 1, /*!< Temperature sensor mode. */
};

enum {
	ADS1118_CONFIG_PULL_UP_DISABLE = 0, /*!< No pull-up (default). */
	ADS1118_CONFIG_PULL_UP_ENABLE = 1,  /*!< Pull-up enabled. */
};

enum {
	ADS1118_CONFIG_NOP_NOP = 0,    /*!< No operation. */
	ADS1118_CONFIG_NOP_UPDATE = 1, /*!< Update the config register (default). */
};

enum {
	ADS1118_CONFIG_CNV_RDY = 0,       /*!< Conversion ready. */
	ADS1118_CONFIG_CNV_NOT_READY = 1, /*!< Conversion not ready */
};

struct ads1118_config {
	const struct spi_dt_spec spi;
	uint8_t resolution;
	bool is_temperature_mode;
	bool multiplexer;
#if CONFIG_ADC_ASYNC
	k_thread_stack_t *stack;
#endif
};

struct ads1118_data {
	const struct device *dev;
	struct adc_context ctx;
	struct k_sem acq_sem;
	k_timeout_t ready_time;
	bool differential;
	int32_t *buffer;
	int32_t *buffer_ptr;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;
#endif
};

static inline int ads1118_transceive(const struct device *dev, uint8_t *send_buf,
				     size_t send_buf_len, uint8_t *recv_buf, size_t recv_buf_len)
{
	int ret;
	const struct ads1118_config *cfg = dev->config;
	struct spi_buf tx_buf = {
		.buf = send_buf,
		.len = send_buf_len,
	};

	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf = {
		.buf = recv_buf,
		.len = recv_buf_len,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, NULL);
	if (ret != 0) {
		return ret;
	}

	return spi_read_dt(&cfg->spi, &rx);
}

static int ads1118_config_reg_read(const struct device *dev, uint16_t *config_reg)
{
	int ret;
	uint8_t tx_buf[4] = {0};
	uint8_t rx_buf[4] = {0};

	ret = ads1118_transceive(dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
	if (ret != 0) {
		return ret;
	}
	*config_reg = sys_get_be16(&rx_buf[2]);

	return 0;
}

static int ads1118_config_reg_write(const struct device *dev, uint16_t write_data)
{
	uint8_t tx_buf[2];
	uint8_t rx_buf[2];

	sys_put_be16(write_data, tx_buf);

	return ads1118_transceive(dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
}

static int ads1118_configure_gain(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	switch (channel_cfg->gain) {
	case ADC_GAIN_2_3:
		return ADS1118_CONFIG_PGA_6144;
	case ADC_GAIN_1:
		return ADS1118_CONFIG_PGA_4096;
	case ADC_GAIN_2:
		return ADS1118_CONFIG_PGA_2048;
	case ADC_GAIN_4:
		return ADS1118_CONFIG_PGA_1024;
	case ADC_GAIN_8:
		return ADS1118_CONFIG_PGA_512;
	case ADC_GAIN_16:
		return ADS1118_CONFIG_PGA_256;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int ads1118_acq_time_to_dr(const struct device *dev, uint16_t acq_time)
{
	int odr;
	uint32_t ready_time_us;
	struct ads1118_data *data = dev->data;
	uint16_t acq_value = ADC_ACQ_TIME_VALUE(acq_time);

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		acq_value = ADS1118_CONFIG_DR_DEFAULT;
	} else {
		if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
			return -EINVAL;
		}
	}

	switch (acq_value) {
	case ADS1118_CONFIG_DR_8_128:
		odr = ADS1118_CONFIG_DR_8_128;
		ready_time_us = (1000 * 1000) / 8;
		break;
	case ADS1118_CONFIG_DR_16_250:
		odr = ADS1118_CONFIG_DR_16_250;
		ready_time_us = (1000 * 1000) / 16;
		break;
	case ADS1118_CONFIG_DR_32_490:
		odr = ADS1118_CONFIG_DR_32_490;
		ready_time_us = (1000 * 1000) / 32;
		break;
	case ADS1118_CONFIG_DR_64_920:
		odr = ADS1118_CONFIG_DR_64_920;
		ready_time_us = (1000 * 1000) / 64;
		break;
	case ADS1118_CONFIG_DR_128_1600:
		odr = ADS1118_CONFIG_DR_128_1600;
		ready_time_us = (1000 * 1000) / 128;
		break;
	case ADS1118_CONFIG_DR_250_2400:
		odr = ADS1118_CONFIG_DR_250_2400;
		ready_time_us = (1000 * 1000) / 250;
		break;
	case ADS1118_CONFIG_DR_475_3300:
		odr = ADS1118_CONFIG_DR_475_3300;
		ready_time_us = (1000 * 1000) / 475;
		break;
	case ADS1118_CONFIG_DR_860_3300:
		odr = ADS1118_CONFIG_DR_860_3300;
		ready_time_us = (1000 * 1000) / 860;
		break;
	default:
		return -EINVAL;
	}

	/* As per datasheet acquisition time is a bit longer wait a bit more
	 * to ensure data ready at first try
	 */
	data->ready_time = K_USEC(ready_time_us + 100);

	return odr;
}

static int ads1118_configure_multiplexer(const struct device *dev,
					 const struct adc_channel_cfg *channel_cfg)
{
	const struct ads1118_config *ads_config = dev->config;
	struct ads1118_data *data = dev->data;
	int config = ADS1118_CONFIG_MUX_DIFF_0_1;

	if (ads_config->multiplexer) {
		/* the device has an input multiplexer */
		if (channel_cfg->differential) {
			if (channel_cfg->input_positive == 0 && channel_cfg->input_negative == 1) {
				config = ADS1118_CONFIG_MUX_DIFF_0_1;
			} else if (channel_cfg->input_positive == 0 &&
				   channel_cfg->input_negative == 3) {
				config = ADS1118_CONFIG_MUX_DIFF_0_3;
			} else if (channel_cfg->input_positive == 1 &&
				   channel_cfg->input_negative == 3) {
				config = ADS1118_CONFIG_MUX_DIFF_1_3;
			} else if (channel_cfg->input_positive == 2 &&
				   channel_cfg->input_negative == 3) {
				config = ADS1118_CONFIG_MUX_DIFF_2_3;
			} else {
				LOG_ERR("unsupported input positive '%d' and input negative '%d'",
					channel_cfg->input_positive, channel_cfg->input_negative);
				return -ENOTSUP;
			}
		} else {
			if (channel_cfg->input_positive <= 3) {
				config = ADS1118_CONFIG_MUX_SINGLE_0 + channel_cfg->input_positive;
			} else {
				LOG_ERR("unsupported input positive '%d'",
					channel_cfg->input_positive);
				return -ENOTSUP;
			}
		}
	} else {
		/* only differential supported without multiplexer */
		if (!((channel_cfg->differential) &&
		      (channel_cfg->input_positive == 0 && channel_cfg->input_negative == 1))) {
			LOG_ERR("unsupported input positive '%d' and input negative '%d'",
				channel_cfg->input_positive, channel_cfg->input_negative);
			return -ENOTSUP;
		}
	}
	data->differential = channel_cfg->differential;

	return config;
}

static int ads1118_setup(const struct device *dev, const struct adc_channel_cfg *channel_cfg)
{
	const struct ads1118_config *ads_config = dev->config;
	int ret;
	uint16_t config = 0;

	config |= ADS1118_CONFIG_MODE(ADS1118_CONFIG_MODE_SINGLE_SHOT);

	ret = ads1118_configure_multiplexer(dev, channel_cfg);
	if (ret < 0) {
		return ret;
	}
	config |= ADS1118_CONFIG_MUX(ret);

	ret = ads1118_configure_gain(dev, channel_cfg);
	if (ret < 0) {
		return ret;
	}
	config |= ADS1118_CONFIG_PGA(ret);

	ret = ads1118_acq_time_to_dr(dev, channel_cfg->acquisition_time);
	if (ret < 0) {
		return ret;
	}
	config |= ADS1118_CONFIG_DR(ret);

	if (ads_config->is_temperature_mode) {
		config |= ADS1118_CONFIG_TS_MODE(ADS1118_CONFIG_MODE_TS_TEMP);
	}

	config |= ADS1118_CONFIG_OS(ADS1118_CONFIG_OS_START);
	config |= ADS1118_CONFIG_NOP(ADS1118_CONFIG_NOP_UPDATE);

	return ads1118_config_reg_write(dev, config);
}

static int ads1118_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id >= ADS1118_MAX_CHANNEL_COUNT) {
		LOG_DBG("Unsupported Channel");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_DBG("Unsupported Reference Voltage");
		return -ENOTSUP;
	}

	return ads1118_setup(dev, channel_cfg);
}

static int ads1118_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int32_t);

	if (sequence->options) {
		needed *= 1 + sequence->options->extra_samplings;
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads1118_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ads1118_config *config = dev->config;
	struct ads1118_data *data = dev->data;
	uint8_t resolution = data->differential ? config->resolution : config->resolution - 1;

	if (sequence->resolution != resolution) {
		return -EINVAL;
	}

	if (sequence->channels != BIT(0) && sequence->channels != BIT(1) &&
	    sequence->channels != BIT(2) && sequence->channels != BIT(3)) {
		LOG_ERR("invalid channel");
		return -EINVAL;
	}

	return ads1118_validate_buffer_size(sequence);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads1118_data *data = CONTAINER_OF(ctx, struct ads1118_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads1118_data *data = CONTAINER_OF(ctx, struct ads1118_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acq_sem);
}

static int ads1118_adc_start_read(const struct device *dev, const struct adc_sequence *sequence,
				  bool wait)
{
	int ret;
	struct ads1118_data *data = dev->data;

	ret = ads1118_validate_sequence(dev, sequence);
	if (ret != 0) {
		LOG_ERR("sequence validation failed");
		return ret;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return ret;
}

static int ads1118_read_sample(const struct device *dev, uint32_t *buffer)
{
	int ret;
	uint8_t tx_buf[4] = {0};
	uint8_t rx_buf[4] = {0};

	ret = ads1118_transceive(dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
	if (ret != 0) {
		return ret;
	}

	*buffer = sys_get_be16(&rx_buf[0]);

	return 0;
}

static int ads1118_check_data_ready(const struct device *dev)
{
	uint16_t reg_cfg;
	struct ads1118_data *data = dev->data;

	k_sleep(data->ready_time);

	return ads1118_config_reg_read(dev, &reg_cfg);
}

static int ads1118_perform_read(const struct device *dev)
{
	int ret;
	struct ads1118_data *data = dev->data;

	k_sem_take(&data->acq_sem, K_FOREVER);
	ret = ads1118_check_data_ready(dev);
	if (ret != 0) {
		adc_context_complete(&data->ctx, ret);
		return ret;
	}

	ret = ads1118_read_sample(dev, data->buffer);
	if (ret != 0) {
		adc_context_complete(&data->ctx, ret);
		return ret;
	}

	data->buffer++;
	adc_context_on_sampling_done(&data->ctx, dev);

	return 0;
}

#if CONFIG_ADC_ASYNC

static void ads1118_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

	while (true) {
		ads1118_perform_read(dev);
	}
}

static int ads1118_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	int rc;
	struct ads1118_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	rc = ads1118_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, rc);

	return rc;
}

static int ads1118_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc;
	struct ads1118_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	rc = ads1118_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, rc);

	return rc;
}
#else
static int ads1118_read(const struct device *dev, const struct adc_sequence *seq)
{
	int ret;
	struct ads1118_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	ret = ads1118_adc_start_read(dev, seq, false);

	while (ret == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		ret = ads1118_perform_read(dev);
	}

	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif /* CONFIG_ADC_ASYNC */

static int ads1118_init(const struct device *dev)
{
	const struct ads1118_config *cfg = dev->config;
	struct ads1118_data *data = dev->data;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("ADS1118 is not ready");
		return -ENODEV;
	}

	data->dev = dev;
	adc_context_init(&data->ctx);
	k_sem_init(&data->acq_sem, 0, 1);

#if CONFIG_ADC_ASYNC
	k_tid_t tid = k_thread_create(&data->thread, cfg->stack,
				      CONFIG_ADC_ADS1118_ACQUISITION_THREAD_STACK_SIZE,
				      ads1118_acquisition_thread, (void *)dev, NULL, NULL,
				      CONFIG_ADC_ADS1118_ASYNC_THREAD_INIT_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ads1118");
#endif
	adc_context_unlock_unconditionally(&data->ctx);

	LOG_INF("ADS1118 Initialised");

	return 0;
}

#define DT_DRV_COMPAT ti_ads1118

static DEVICE_API(adc, api) = {
	.channel_setup = ads1118_channel_setup,
	.read = ads1118_read,
	.ref_internal = ADS1118_REF_INTERNAL,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads1118_adc_read_async,
#endif
};

/* clang-format off */
#define ADC_ADS1118_INST_DEFINE(n)                                                                 \
	IF_ENABLED(CONFIG_ADC_ASYNC,                                                               \
		(static K_KERNEL_STACK_DEFINE(thread_stack_##n,				           \
			CONFIG_ADC_ADS1118_ACQUISITION_THREAD_STACK_SIZE);))                       \
                                                                                                   \
	static const struct ads1118_config config_##n = {                                          \
		.spi = SPI_DT_SPEC_INST_GET(n,                                                     \
				SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8)),             \
		.resolution = ADS1118_RESOLUTION,                                                  \
		.multiplexer = true,                                                               \
		.is_temperature_mode = DT_INST_PROP(n, ti_temperature_mode_enable),                \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.stack = thread_stack_##n))                          \
	};                                                                                         \
                                                                                                   \
	static struct ads1118_data data_##n;                                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ads1118_init, NULL, &data_##n, &config_##n, POST_KERNEL,          \
			      CONFIG_ADC_INIT_PRIORITY, &api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS1118_INST_DEFINE)
