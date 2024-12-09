/*
 * Copyright (c) 2024 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/adc/ads131m02.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(ads131m02, CONFIG_ADC_LOG_LEVEL);

#define ADS131M02_DEVICE_ID		0x22

/* Device settings Registers */
#define ADS131M02_ID_REG		0x00
#define ADS131M02_STATUS_REG		0x01

/* Global settings Registers */
#define ADS131M02_MODE_REG		0x02
#define ADS131M02_CLOCK_REG		0x03
#define ADS131M02_GAIN_REG		0x04
#define ADS131M02_CFG_REG		0x06
#define ADS131M02_THRESH_MSB_REG	0x07
#define ADS131M02_THRESH_LSB_REG	0x08

/* Channel 0 settings Registers */
#define ADS131M02_CH0_CFG_REG		0x09
#define ADS131M02_CH0_OCAL_MSB_REG	0x0A
#define ADS131M02_CH0_OCAL_LSB_REG	0x0B
#define ADS131M02_CH0_GCAL_MSB_REG	0x0C
#define ADS131M02_CH0_GCAL_LSB_REG	0x0D

/* Channel 1 settings Registers */
#define ADS131M02_CH1_CFG_REG		0x0E
#define ADS131M02_CH1_OCAL_MSB_REG	0x0F
#define ADS131M02_CH1_OCAL_LSB_REG	0x10
#define ADS131M02_CH1_GCAL_MSB_REG	0x11
#define ADS131M02_CH1_GCAL_LSB_REG	0x12

/* Register Map CRC Registers */
#define ADS131M02_REGMAP_CRC_REG	0x3E

#define ADC_CHANNEL_0			0
#define ADC_CHANNEL_1			1
#define ADS131M02_REF_INTERNAL		1200
#define ADS131M02_RESOLUTION		24

/* ADS131M02 cmds */
#define ADS131M02_NULL_CMD		0x0000
#define ADS131M02_RESET_CMD		0x0011
#define ADS131M02_STANDBY_CMD		0x0022
#define ADS131M02_WAKEUP_CMD		0x0033
#define ADS131M02_LOCK_CMD		0x0555
#define ADS131M02_UNLOCK_CMD		0x0655
#define ADS131M02_RREG_CMD		0xA000
#define ADS131M02_WREG_CMD		0x6000

#define ADS131M02_RESET_RSP		0xFF22

#define ADS131M02_GAIN0_MASK		GENMASK(2, 0)
#define ADS131M02_GAIN1_MASK		GENMASK(6, 4)
#define ADS131M02_CHANNEL0_ENABLE	BIT(8)
#define ADS131M02_CHANNEL1_ENABLE	BIT(9)
#define ADS131M02_DRDY_CH0_MASK		BIT(0)
#define ADS131M02_DRDY_CH1_MASK		BIT(1)
#define ADS131M02_OSR_256_MASK		BIT(2)
#define ADS131M02_OSR_512_MASK		BIT(3)
#define ADS131M02_OSR_1024_MASK		BIT(3) | BIT(2)
#define ADS131M02_OSR_2048_MASK		BIT(4)
#define ADS131M02_OSR_4096_MASK		BIT(4) | BIT(2)
#define ADS131M02_OSR_8192_MASK		BIT(4) | BIT(3)
#define ADS131M02_OSR_16384_MASK	BIT(4) | BIT(3) | BIT(2)
#define ADS131M02_GC_MODE_MASK		BIT(8)
#define ADS131M02_GC_DELAY_MASK		GENMASK(12, 9)
#define ADS131M02_PWR_HR		BIT(1) | BIT(0)
#define ADS131M02_PWR_LP		BIT(0)

#define ADS131M02_DISABLE_ADC		0x000E
#define ADS131M02_RESET_DELAY		100

#define ADS131M02_GAIN_1		0
#define ADS131M02_GAIN_2		1
#define ADS131M02_GAIN_4		2
#define ADS131M02_GAIN_8		3
#define ADS131M02_GAIN_16		4
#define ADS131M02_GAIN_32		5
#define ADS131M02_GAIN_64		6
#define ADS131M02_GAIN_128		7

#define ADS131M02_GET_GAIN(channel_id, gain)	\
		FIELD_PREP(channel_id == 0 ? ADS131M02_GAIN0_MASK : \
			   ADS131M02_GAIN1_MASK, gain)

enum ads131m02_data_rate {
	/* SPS */
	ADS131M02_DR_250,
	ADS131M02_DR_500,
	ADS131M02_DR_1k,
	ADS131M02_DR_2k,
	ADS131M02_DR_4k,
	ADS131M02_DR_8k,
	ADS131M02_DR_16k,
	ADS131M02_DR_32k,
};

struct ads131m02_config {
	const struct spi_dt_spec spi;
	const struct gpio_dt_spec gpio_drdy;
};

struct ads131m02_data {
	struct adc_context ctx;
	struct k_sem acq_sem;
	struct k_sem drdy_sem;
	struct gpio_callback callback_drdy;
	int32_t *buffer;
	int32_t *buffer_ptr;
};

static inline int ads131m02_transceive(const struct device *dev,
				       uint8_t *send_buf, size_t send_buf_len,
				       uint8_t *recv_buf, size_t recv_buf_len)
{
	int ret;
	const struct ads131m02_config *cfg = dev->config;

	struct spi_buf tx_buf = {
		.buf = send_buf,
		.len = send_buf_len,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

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

static int ads131m02_reg_read(const struct device *dev, uint16_t addr,
			      uint8_t *read_buf, size_t read_buf_len)
{
	uint16_t temp;
	uint8_t tx_buf[3] = {0};

	temp = (uint16_t)(ADS131M02_RREG_CMD | (addr << 7));
	sys_put_be16(temp, tx_buf);

	return ads131m02_transceive(dev, tx_buf, sizeof(tx_buf),
				    read_buf, read_buf_len);
}

static int ads131m02_reg_write(const struct device *dev, uint16_t addr,
			       uint16_t write_data)
{
	uint16_t temp;
	uint8_t tx_buf[6] = {0};
	uint8_t rx_buf[3] = {0};

	temp = (uint16_t)(ADS131M02_WREG_CMD | (addr << 7));
	sys_put_be16(temp, tx_buf);
	sys_put_be16(write_data, &tx_buf[3]);

	return ads131m02_transceive(dev, tx_buf, sizeof(tx_buf),
				    rx_buf, sizeof(rx_buf));

}

static inline int ads131m02_configure_gain(const struct device *dev,
					   const struct adc_channel_cfg *channel_cfg)
{
	uint16_t gain_cfg;

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_1);
		break;
	case ADC_GAIN_2:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_2);
		break;
	case ADC_GAIN_4:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_4);
		break;
	case ADC_GAIN_8:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_8);
		break;
	case ADC_GAIN_16:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_16);
		break;
	case ADC_GAIN_32:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_32);
		break;
	case ADC_GAIN_64:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_64);
		break;
	case ADC_GAIN_128:
		gain_cfg = ADS131M02_GET_GAIN(channel_cfg->channel_id,
					      ADS131M02_GAIN_128);
		break;
	default:
		return -EINVAL;
	}

	return ads131m02_reg_write(dev, ADS131M02_GAIN_REG, gain_cfg);
}

static inline int ads131m02_acquistion_time(uint16_t acq_time, uint16_t *enable)
{
	uint16_t acq_value = ADC_ACQ_TIME_VALUE(acq_time);

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		*enable |=  ADS131M02_OSR_1024_MASK;
		return 0;
	}

	if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		return -EINVAL;
	}

	if (acq_time == ADC_ACQ_TIME_MAX) {
		*enable |= ADS131M02_OSR_16384_MASK;
		return 0;
	}

	switch (acq_value) {
	case ADS131M02_DR_250:
		*enable |= ADS131M02_OSR_16384_MASK;
		break;
	case ADS131M02_DR_500:
		*enable |= ADS131M02_OSR_8192_MASK;
		break;
	case ADS131M02_DR_1k:
		*enable |= ADS131M02_OSR_4096_MASK;
		break;
	case ADS131M02_DR_2k:
		*enable |= ADS131M02_OSR_2048_MASK;
		break;
	case ADS131M02_DR_4k:
		*enable |= ADS131M02_OSR_1024_MASK;
		break;
	case ADS131M02_DR_8k:
		*enable |= ADS131M02_OSR_512_MASK;
		break;
	case ADS131M02_DR_16k:
		*enable |= ADS131M02_OSR_256_MASK;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ads131m02_setup(const struct device *dev,
			   const struct adc_channel_cfg *channel_cfg)
{
	int ret;
	uint16_t enable;
	uint8_t read_data[3] = {0};

	ret  = ads131m02_reg_read(dev, ADS131M02_CLOCK_REG, read_data,
				  sizeof(read_data));
	if (ret != 0) {
		return ret;
	}

	enable = sys_get_be16(read_data);
	switch (channel_cfg->channel_id) {
	case ADC_CHANNEL_0:
		enable |= ADS131M02_CHANNEL0_ENABLE;
		break;
	case ADC_CHANNEL_1:
		enable |= ADS131M02_CHANNEL1_ENABLE;
		break;
	default:
		return -EINVAL;
	}

	enable &= ~(ADS131M02_OSR_16384_MASK);
	ret = ads131m02_acquistion_time(channel_cfg->acquisition_time, &enable);
	if (ret < 0) {
		return ret;
	}

	return ads131m02_reg_write(dev, ADS131M02_CLOCK_REG, enable);
}

static int ads131m02_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	int ret;

	if (channel_cfg->channel_id != 0 && channel_cfg->channel_id != 1) {
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_DBG("Unsupported Reference Voltage");
		return -ENOTSUP;
	}

	if (!channel_cfg->differential) {
		return -EINVAL;
	}

	ret = ads131m02_configure_gain(dev, channel_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = ads131m02_setup(dev, channel_cfg);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int ads131m02_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int32_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads131m02_validate_sequence(const struct adc_sequence *sequence)
{
	if (sequence->resolution != ADS131M02_RESOLUTION) {
		return -EINVAL;
	}

	if (sequence->channels != BIT(0) && sequence->channels != BIT(1)) {
		LOG_ERR("invalid channel");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		return -EINVAL;
	}

	return ads131m02_validate_buffer_size(sequence);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct ads131m02_data *data = CONTAINER_OF(ctx, struct ads131m02_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads131m02_data *data = CONTAINER_OF(ctx, struct ads131m02_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acq_sem);
}

static int ads131m02_adc_start_read(const struct device *dev,
				    const struct adc_sequence *sequence,
				    bool wait)
{
	int ret;
	struct ads131m02_data *data = dev->data;

	ret = ads131m02_validate_sequence(sequence);
	if (ret != 0) {
		LOG_ERR("sequence validation failed");
		return ret;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);
	if (wait) {
		ret = adc_context_wait_for_completion(&data->ctx);
	}

	return ret;
}

static int ads131m02_wait_drdy(const struct device *dev)
{
	struct ads131m02_data *data = dev->data;

	return k_sem_take(&data->drdy_sem,
			  ADC_CONTEXT_WAIT_FOR_COMPLETION_TIMEOUT);
}

static int ads131m02_read_sample(const struct device *dev,
				 uint32_t channels, uint32_t *buffer)
{
	int ret;
	uint16_t int_status;
	uint8_t tx_buf[4] = {0};
	uint8_t rx_buf[12] = {0};

	ret = ads131m02_transceive(dev, tx_buf, sizeof(tx_buf),
				   rx_buf, sizeof(rx_buf));
	if (ret != 0) {
		return ret;
	}

	int_status = sys_get_be16(&rx_buf[0]);
	if ((int_status & ADS131M02_DRDY_CH0_MASK) && (channels & BIT(0))) {
		*buffer = sys_get_be24(&rx_buf[3]);
	} else if ((int_status & ADS131M02_DRDY_CH1_MASK) &&
		   (channels & BIT(1))) {
		*buffer = sys_get_be24(&rx_buf[6]);
	} else {
		LOG_INF("No ADC Data Available");
	}

	return 0;
}

static int ads131m02_perform_read(const struct device *dev,
				  const struct adc_sequence *sequence)
{
	int ret;
	struct ads131m02_data *data = dev->data;

	k_sem_take(&data->acq_sem, K_FOREVER);
	k_sem_reset(&data->drdy_sem);

	ret = ads131m02_wait_drdy(dev);
	if (ret != 0) {
		goto error;
	}

	ret = ads131m02_read_sample(dev, sequence->channels, data->buffer);
	if (ret != 0) {
		goto error;
	}

	data->buffer++;
	adc_context_on_sampling_done(&data->ctx, dev);

	return 0;
error:
	adc_context_complete(&data->ctx, ret);
	return ret;
}

static int ads131m02_read(const struct device *dev,
			  const struct adc_sequence *seq)
{
	int ret;
	struct ads131m02_data *data =  dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	ret = ads131m02_adc_start_read(dev, seq, false);

	while (ret == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		ret = ads131m02_perform_read(dev, seq);
	}

	adc_context_release(&data->ctx, ret);

	return ret;
}

static void ads131m02_data_ready_handler(const struct device *dev,
					 struct gpio_callback *gpio_cb,
					 uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct ads131m02_data *data = CONTAINER_OF(gpio_cb,
				      struct ads131m02_data, callback_drdy);

	k_sem_give(&data->drdy_sem);
}

static int ads131m02_configure_gpio(const struct device *dev)
{
	int ret;
	const struct ads131m02_config *cfg = dev->config;
	struct ads131m02_data *data = dev->data;

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->callback_drdy, ads131m02_data_ready_handler,
			   BIT(cfg->gpio_drdy.pin));

	return gpio_add_callback(cfg->gpio_drdy.port, &data->callback_drdy);
}

static int ads131m02_device_reset(const struct device *dev)
{
	int ret;
	uint8_t tx_buf[12] = {0};
	uint8_t rx_buf[3] = {0};

	sys_put_be16(ADS131M02_RESET_CMD, tx_buf);
	ret = ads131m02_transceive(dev, tx_buf, sizeof(tx_buf),
				   rx_buf, sizeof(rx_buf));

	if (ret != 0) {
		return ret;
	}

	if (sys_get_be16(rx_buf) != ADS131M02_RESET_RSP) {
		return -EIO;
	}

	k_msleep(ADS131M02_RESET_DELAY);

	return 0;
}

#if defined CONFIG_PM_DEVICE
static int ads131m02_pm(const struct device *dev, uint16_t cmd)
{
	int ret;
	uint8_t tx_buf[3] = {0};
	uint8_t rx_buf[3] = {0};

	sys_put_be16(cmd, tx_buf);
	ret = ads131m02_transceive(dev, tx_buf, sizeof(tx_buf),
				   rx_buf, sizeof(rx_buf));
	if (ret != 0) {
		return ret;
	}

	if (rx_buf[1] != cmd) {
		return -EIO;
	}

	return 0;
}

static int ads131m02_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return ads131m02_pm(dev, ADS131M02_WAKEUP_CMD);
	case PM_DEVICE_ACTION_SUSPEND:
		return ads131m02_pm(dev, ADS131M02_STANDBY_CMD);
	default:
		return -EINVAL;
	}
}
#endif /* CONFIG_PM_DEVICE */

int ads131m02_set_adc_mode(const struct device *dev,
			   enum ads131m02_adc_mode mode,
			   enum ads131m02_gc_delay gc_delay)
{
	uint16_t temp = 0;

	switch (mode) {
	case ADS131M02_CONTINUOUS_MODE:
		break;
	case ADS131M02_GLOBAL_CHOP_MODE:
		temp |= ADS131M02_GC_MODE_MASK;
		temp |= FIELD_PREP(ADS131M02_GC_DELAY_MASK, gc_delay);
		break;
	default:
		return -EINVAL;
	}

	return ads131m02_reg_write(dev, ADS131M02_CFG_REG, temp);
}

int ads131m02_set_power_mode(const struct device *dev,
			     enum ads131m02_adc_power_mode mode)
{
	int ret;
	uint16_t temp;
	uint8_t buf[3] = {0};

	ret = ads131m02_reg_read(dev, ADS131M02_CLOCK_REG, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	temp = sys_get_be16(buf);
	temp &= ~(ADS131M02_PWR_HR);

	switch (mode) {
	case ADS131M02_VLP:
		break;
	case ADS131M02_LP:
		temp |= ADS131M02_PWR_LP;
		break;
	case ADS131M02_HR:
		temp |= ADS131M02_PWR_HR;
		break;
	default:
		return -EINVAL;
	}

	return ads131m02_reg_write(dev, ADS131M02_CLOCK_REG, temp);
}

static const struct adc_driver_api ads131m02_api = {
	.channel_setup = ads131m02_channel_setup,
	.read = ads131m02_read,
	.ref_internal = ADS131M02_REF_INTERNAL,
};

static int ads131m02_init(const struct device *dev)
{
	int ret;
	uint8_t buf[3] = {0};
	const struct ads131m02_config *cfg = dev->config;
	struct ads131m02_data *data = dev->data;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("ADS131M02 is not ready");
		return -ENODEV;
	}

	adc_context_init(&data->ctx);
	k_sem_init(&data->acq_sem, 0, 1);
	k_sem_init(&data->drdy_sem, 0, 1);

	ret = ads131m02_configure_gpio(dev);
	if (ret != 0) {
		LOG_ERR("GPIO config failed %d", ret);
		return ret;
	}

	ret = ads131m02_reg_read(dev, ADS131M02_ID_REG, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	if (buf[0] != ADS131M02_DEVICE_ID) {
		LOG_ERR("Device ID mismatch %d", buf[0]);
		return -ENODEV;
	}

	ret = ads131m02_device_reset(dev);
	if (ret != 0) {
		LOG_WRN("Device is not reset");
	}

	/* By default, adc is configured, so disabling it */
	ret = ads131m02_reg_write(dev, ADS131M02_CLOCK_REG,
				  ADS131M02_DISABLE_ADC);
	if (ret != 0) {
		return ret;
	}

	adc_context_unlock_unconditionally(&data->ctx);

#if defined CONFIG_PM_DEVICE
	ret = ads131m02_pm(dev, ADS131M02_STANDBY_CMD);
	if (ret != 0) {
		return ret;
	}

	pm_device_init_suspended(dev);
#endif /* CONFIG_PM_DEVICE */

	LOG_INF("ADS131M02 Initialised");

	return 0;
}

#define DT_DRV_COMPAT ti_ads131m02

#define ADC_ADS131M02_INST_DEFINE(n)						\
	PM_DEVICE_DT_INST_DEFINE(n, ads131m02_pm_action);			\
	static const struct ads131m02_config config_##n = {			\
		.spi = SPI_DT_SPEC_INST_GET(					\
			n, SPI_OP_MODE_MASTER | SPI_MODE_CPHA |			\
			SPI_WORD_SET(8), 0),					\
		.gpio_drdy = GPIO_DT_SPEC_INST_GET(n, drdy_gpios),		\
	};									\
	static struct ads131m02_data data_##n;					\
	DEVICE_DT_INST_DEFINE(n, ads131m02_init,				\
			      PM_DEVICE_DT_INST_GET(n),				\
			      &data_##n, &config_##n, POST_KERNEL,		\
			      CONFIG_ADC_INIT_PRIORITY, &ads131m02_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS131M02_INST_DEFINE)
