/*
 * Copyright (c) 2025 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(adc_ad405x, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define AD4050_CHIP_ID 4050
#define AD4052_CHIP_ID 4052

#define AD4050_ADC_RESOLUTION 12U
#define AD4052_ADC_RESOLUTION 16U

#define AD405X_REG_INTERFACE_CONFIG_A 0x00U
#define AD405X_REG_DEVICE_CONFIG      0x02U
#define AD405X_REG_DEVICE_TYPE        0x03U
#define AD405X_REG_PRODUCT_ID_L       0x04U
#define AD405X_REG_PRODUCT_ID_H       0x05U
#define AD405X_REG_VENDOR_L           0x0CU
#define AD405X_REG_VENDOR_H           0x0DU
#define AD405X_REG_MODE_SET           0x20U
#define AD405X_REG_ADC_MODES          0x21U
#define AD405X_REG_AVG_CONFIG         0x23U
#define AD405X_REG_GP_PIN_CONF        0x24U
#define AD405X_REG_TIMER_CONFIG       0x27U

#define AD405X_REG_INTERFACE_CONFIG_A_VAL       0x10U
#define AD405X_REG_DEVICE_TYPE_VAL              0x07U
#define AD4052_REG_PRODUCT_ID_VAL               0x0072U
#define AD4050_REG_PRODUCT_ID_VAL               0x0070U
#define AD405X_REG_VENDOR_VAL                   0x0456U
#define AD405X_REG_INTERFACE_CONFIG_A_RESET_VAL 0x81U

/** AD405X_REG_ADC_MODES Bit definition */
#define AD405X_ADC_MODES_MSK        GENMASK(2, 0)
#define AD405X_BURST_AVERAGING_MODE BIT(0)
#define AD405X_AVERAGING_MODE       BIT(1)

/** AD405X_REG_MODE_SET Bit definition */
#define AD405X_ENTER_ADC_MODE_MSK BIT(0)
#define AD405X_ENTER_ADC_MODE     BIT(0)
#define AD405X_ENTER_SLEEP_MODE   BIT(1) | BIT(0)
#define AD405X_ENTER_ACTIVE_MODE  0x0U

/** AD405X_REG_AVG_CONFIG Bit Definitions */
#define AD405X_AVG_WIN_LEN_MSK GENMASK(3, 0)

#define AD405X_SINGLE_DIFFERENTIAL_MSK BIT(7)

#define AD405X_WRITE_CMD 0x0U
#define AD405X_READ_CMD  0x80U

#define AD405X_SW_RESET_MSK   BIT(7) | BIT(0)

#define AD405X_GP1_MODE_MSK GENMASK(6, 4)
#define AD405X_GP0_MODE_MSK GENMASK(2, 0)
#define AD405X_GP1          0x1U
#define AD405X_GP0          0x0U

#define AD405X_SINGLE_ENDED  0x0U
#define AD405X_DIFFERENTIAL  BIT(7)

#define AD405X_NO_GPIO      0xFFU

/** AD405X_REG_TIMER_CONFIG Bit Definitions */
#define AD405X_FS_BURST_AUTO_MSK GENMASK(7, 4)

union ad405x_bus {
	struct spi_dt_spec spi;
};

enum ad405x_gpx_mode {
	AD405X_DISABLED = 0,
	AD405X_GP0_1_INTR = 1,
	AD405X_DATA_READY = 2,
	AD405X_DEV_ENABLE = 3,
	AD405X_CHOP = 4,
	AD405X_LOGIC_LOW = 5,
	AD405X_LOGIC_HIGH = 6,
	AD405X_DEV_READY = 7
};

/** AD405X modes of operations */
enum ad405x_operation_mode {
	AD405X_SAMPLE_MODE_OP = 0,
	AD405X_BURST_AVERAGING_MODE_OP = 1,
	AD405X_AVERAGING_MODE_OP = 2,
	AD405X_MONITOR_AUTO_MODE_OP = 3,
	AD405X_CONFIG_MODE_OP = 4,
	AD405X_SLEEP_MODE_OP = 5,
	AD405X_TRIGGER_AUTO_MODE_OP = 7
};

/** AD405X sample rate for burst and autonomous modes */
enum ad405x_sample_rate {
	AD405X_2_MSPS,
	AD405X_1_MSPS,
	AD405X_333_KSPS,
	AD405X_100_KSPS,
	AD405X_33_KSPS,
	AD405X_10_KSPS,
	AD405X_3_KSPS,
	AD405X_1_KSPS,
	AD405X_500_SPS,
	AD405X_333_SPS,
	AD405X_250_SPS,
	AD405X_200_SPS,
	AD405X_166_SPS,
	AD405X_140_SPS,
	AD405X_125_SPS,
	AD405X_111_SPS
};

/** AD405X averaging filter window length */
enum ad405x_avg_filter_l {
	AD405X_LENGTH_2,
	AD405X_LENGTH_4,
	AD405X_LENGTH_8,
	AD405X_LENGTH_16,
	AD405X_LENGTH_32,
	AD405X_LENGTH_64,
	AD405X_LENGTH_128,
	AD405X_LENGTH_256,
	AD405X_LENGTH_512,
	AD405X_LENGTH_1024,
	AD405X_LENGTH_2048,
	AD405X_LENGTH_4096
};
static const int avg_filter_values[] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

struct adc_ad405x_config {
	const union ad405x_bus bus;
	enum ad405x_operation_mode active_mode;
#ifdef CONFIG_AD405X_TRIGGER
	struct gpio_dt_spec gp1_interrupt;
	struct gpio_dt_spec gp0_interrupt;
	uint8_t has_gp1;
	uint8_t has_gp0;
#endif
	struct gpio_dt_spec conversion;
	uint16_t chip_id;
	const struct adc_dt_spec spec;
#ifdef CONFIG_AD405X_STREAM
	uint32_t sampling_period;
#endif /* CONFIG_AD405X_STREAM */
};

struct adc_ad405x_data {
	struct adc_context ctx;
	const struct device *dev;
	uint8_t adc_conf;
	uint8_t diff;
	uint8_t channels;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	enum ad405x_operation_mode operation_mode;
	struct k_sem sem_devrdy;
	const struct device *gpio_dev;
	enum ad405x_gpx_mode gp1_mode;
	enum ad405x_gpx_mode gp0_mode;
	uint8_t dev_en_pol;
	enum ad405x_sample_rate rate;
	enum ad405x_avg_filter_l filter_length;
#if CONFIG_AD405X_TRIGGER
	struct gpio_callback gpio1_cb;
	struct gpio_callback gpio0_cb;
	struct k_sem sem_drdy;
	uint8_t has_drdy;
#endif /* CONFIG_AD405X_TRIGGER */
#ifdef CONFIG_AD405X_STREAM
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint64_t timestamp;
	struct rtio *r_cb;
	uint32_t adc_sample;
	uint8_t data_ready_gpio;
#if DT_HAS_CHOSEN(zephyr_adc_clock)
	const struct device *timer_dev;
#else
	struct k_timer sample_timer;
#endif
#endif /* CONFIG_AD405X_STREAM */
};

#ifdef CONFIG_AD405X_STREAM
#include <zephyr/drivers/counter.h>

#define AD405X_DEF_SAMPLING_PERIOD 10000U /* 10ms */

/** AD405X qscale modes */
enum ad405x_qscale_modes {
	AD4050_6_12B_MODE = 0,
	AD4050_6_14B_MODE = 1,
	AD4052_8_16B_MODE = 2,
	AD4052_8_20B_MODE = 3,
};

struct adc_ad405x_fifo_data {
	uint8_t is_fifo: 1;
	uint8_t ad405x_qscale_mode: 2;
	uint8_t diff_mode: 1;
	uint8_t empty: 1;
	uint8_t res: 3;
	uint16_t vref_mv;
	uint64_t timestamp;
} __attribute__((__packed__));

static void ad405x_stream_irq_handler(const struct device *dev);
static int ad405x_conv_start(const struct device *dev);

#if DT_HAS_CHOSEN(zephyr_adc_clock)
static void timer_alarm_handler(const struct device *counter_dev, uint8_t chan_id,
	uint32_t ticks, void *user_data)
{
	struct adc_ad405x_data *data = (struct adc_ad405x_data *)user_data;

	ad405x_conv_start(data->dev);
}
#else
static void sample_timer_handler(struct k_timer *timer)
{
	struct adc_ad405x_data *data = CONTAINER_OF(timer, struct adc_ad405x_data,
						sample_timer);

	ad405x_conv_start(data->dev);
}
#endif
static void ad405x_timer_init(const struct device *dev)
{
	struct adc_ad405x_data *data = dev->data;

#if DT_HAS_CHOSEN(zephyr_adc_clock)
	counter_start(data->timer_dev);
#else
	k_timer_init(&data->sample_timer, sample_timer_handler, NULL);
#endif
}

static void ad405x_timer_start(const struct device *dev)
{
	struct adc_ad405x_data *data = dev->data;
	const struct adc_ad405x_config *cfg_405 = (const struct adc_ad405x_config *)dev->config;
#if DT_HAS_CHOSEN(zephyr_adc_clock)
	struct counter_alarm_cfg alarm_cfg = {.flags = 0,
		.ticks = counter_us_to_ticks(data->timer_dev, (uint64_t)cfg_405->sampling_period),
		.callback = timer_alarm_handler,
		.user_data = data};

	counter_set_channel_alarm(data->timer_dev, 0, &alarm_cfg);
#else
	k_timer_start(&data->sample_timer, K_USEC(cfg_405->sampling_period), K_NO_WAIT);
#endif
}
#endif /* CONFIG_AD405X_STREAM */

static bool ad405x_bus_is_ready_spi(const union ad405x_bus *bus)
{
	bool ret = spi_is_ready_dt(&bus->spi);

	return ret;
}

int ad405x_reg_access_spi(const struct device *dev, uint8_t cmd, uint8_t reg_addr, uint8_t *data,
			size_t length)
{
	const struct adc_ad405x_config *cfg = dev->config;
	uint8_t access = reg_addr | cmd;
	const struct spi_buf buf[2] = {{.buf = &access, .len = 1}, {.buf = data, .len = length}};
	const struct spi_buf_set rx = {.buffers = buf, .count = ARRAY_SIZE(buf)};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};
	int ret;

	if (cmd == AD405X_READ_CMD) {
		tx.count = 1;
		ret = spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	} else {
		ret = spi_write_dt(&cfg->bus.spi, &tx);
	}
	return ret;
}

int ad405x_reset_pattern_cmd(const struct device *dev)
{
	const struct adc_ad405x_config *cfg = dev->config;
#if CONFIG_AD405X_TRIGGER
	struct adc_ad405x_data *data = dev->data;
#endif
	uint8_t access[18] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE};
	const struct spi_buf buf[1] = {{.buf = &access, .len = ARRAY_SIZE(access)}};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = 1,
	};

	int ret = spi_write_dt(&cfg->bus.spi, &tx);

	if (ret < 0) {
		return ret;
	}
#if CONFIG_AD405X_TRIGGER
	if (cfg->has_gp1 != 0) {
		k_sem_take(&data->sem_devrdy, K_FOREVER);
	} else {
		k_msleep(5);
	}
#else
	k_msleep(5);
#endif
	return ret;

}

int ad405x_read_raw(const struct device *dev, uint8_t *data, size_t len)
{
	const struct adc_ad405x_config *cfg = dev->config;
	int ret = 0;
	const struct spi_buf buf[1] = {{.buf = data, .len = len}};
	const struct spi_buf_set rx = {.buffers = buf, .count = ARRAY_SIZE(buf)};

	ret = spi_transceive_dt(&cfg->bus.spi, NULL, &rx);

	return ret;
}

int ad405x_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr, uint8_t *data,
			size_t len)
{
	return ad405x_reg_access_spi(dev, cmd, addr, data, len);
}

int ad405x_reg_write(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len)
{

	return ad405x_reg_access(dev, AD405X_WRITE_CMD, addr, data, len);
}

int ad405x_reg_read(const struct device *dev, uint8_t addr, uint8_t *data, uint8_t len)
{

	return ad405x_reg_access(dev, AD405X_READ_CMD, addr, data, len);
}

int ad405x_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val)
{
	return ad405x_reg_write(dev, addr, &val, 1);
}

int ad405x_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf)

{
	return ad405x_reg_read(dev, addr, buf, 1);
}

int ad405x_reg_update_bits(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	int ret;
	uint8_t byte = 0;

	ret = ad405x_reg_read_byte(dev, addr, &byte);
	if (ret < 0) {
		return ret;
	}

	byte &= ~mask;
	byte |= val;

	return ad405x_reg_write_byte(dev, addr, byte);
}

#if CONFIG_AD405X_TRIGGER
static void ad405x_gpio1_callback(const struct device *dev, struct gpio_callback *cb,
				uint32_t pins)
{
	struct adc_ad405x_data *drv_data = CONTAINER_OF(cb, struct adc_ad405x_data, gpio1_cb);
	const struct adc_ad405x_config *cfg = drv_data->dev->config;
	gpio_flags_t gpio_flag = GPIO_INT_EDGE_TO_ACTIVE;

	gpio_pin_interrupt_configure_dt(&cfg->gp1_interrupt, GPIO_INT_DISABLE);

	switch (drv_data->gp1_mode) {
	case AD405X_DEV_READY:
		k_sem_give(&drv_data->sem_devrdy);
		break;
	case AD405X_DATA_READY:
#ifdef CONFIG_AD405X_STREAM
		const struct adc_read_config *cfg_adc = drv_data->sqe->sqe.iodev->data;

		if (cfg_adc->is_streaming) {
			ad405x_stream_irq_handler(drv_data->dev);
		} else {
			k_sem_give(&drv_data->sem_drdy);
		}
#else
		k_sem_give(&drv_data->sem_drdy);
#endif /* CONFIG_AD405X_STREAM */
		gpio_flag = GPIO_INT_EDGE_TO_INACTIVE;
		break;
	default: /* TODO */
		break;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gp1_interrupt, gpio_flag);
}

static void ad405x_gpio0_callback(const struct device *dev, struct gpio_callback *cb,
				uint32_t pins)
{
	struct adc_ad405x_data *drv_data = CONTAINER_OF(cb, struct adc_ad405x_data, gpio0_cb);
	const struct adc_ad405x_config *cfg = drv_data->dev->config;
	gpio_flags_t gpio_flag = GPIO_INT_EDGE_TO_ACTIVE;

	gpio_pin_interrupt_configure_dt(&cfg->gp0_interrupt, GPIO_INT_DISABLE);

	switch (drv_data->gp0_mode) {
	case AD405X_DATA_READY:
#ifdef CONFIG_AD405X_STREAM
		const struct adc_read_config *cfg_adc = drv_data->sqe->sqe.iodev->data;

		if (cfg_adc->is_streaming) {
			ad405x_stream_irq_handler(drv_data->dev);
		} else {
			k_sem_give(&drv_data->sem_drdy);
		}
#else
		k_sem_give(&drv_data->sem_drdy);
#endif /* CONFIG_AD405X_STREAM */
		gpio_flag = GPIO_INT_EDGE_TO_INACTIVE;
		break;
	default:
		break;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gp0_interrupt, gpio_flag);

}
#endif

int ad405x_init_conv(const struct device *dev)
{
	const struct adc_ad405x_config *cfg = dev->config;
	struct adc_ad405x_data *drv_data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&cfg->conversion)) {
		LOG_ERR("GPIO port %s not ready", cfg->conversion.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&cfg->conversion, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_set_dt(&cfg->conversion, 0);
	drv_data->dev = dev;
	return ret;
}

#if CONFIG_AD405X_TRIGGER
int ad405x_init_interrupt(const struct device *dev)
{
	const struct adc_ad405x_config *cfg = dev->config;
	struct adc_ad405x_data *drv_data = dev->data;
	int ret;

	if (cfg->has_gp1 != 0) {

		if (!gpio_is_ready_dt(&cfg->gp1_interrupt)) {
			LOG_ERR("GPIO port %s not ready", cfg->gp1_interrupt.port->name);
			return -EINVAL;
		}

		ret = gpio_pin_configure_dt(&cfg->gp1_interrupt, GPIO_INPUT);
		if (ret != 0) {
			return ret;
		}

		gpio_init_callback(&drv_data->gpio1_cb, ad405x_gpio1_callback,
					BIT(cfg->gp1_interrupt.pin));

		ret = gpio_add_callback(cfg->gp1_interrupt.port, &drv_data->gpio1_cb);
		if (ret != 0) {
			LOG_ERR("Failed to set gpio callback!");
			return ret;
		}

		gpio_pin_interrupt_configure_dt(&cfg->gp1_interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	}

	if (cfg->has_gp0 != 0) {

		if (!gpio_is_ready_dt(&cfg->gp0_interrupt)) {
			LOG_ERR("GPIO port %s not ready", cfg->gp0_interrupt.port->name);
			return -EINVAL;
		}

		ret = gpio_pin_configure_dt(&cfg->gp0_interrupt, GPIO_INPUT);
		if (ret != 0) {
			return ret;
		}

		gpio_init_callback(&drv_data->gpio0_cb, ad405x_gpio0_callback,
					BIT(cfg->gp0_interrupt.pin));

		ret = gpio_add_callback(cfg->gp0_interrupt.port, &drv_data->gpio0_cb);
		if (ret != 0) {
			LOG_ERR("Failed to set gpio callback!");
			return ret;
		}
	}

	return 0;
}
#endif

static int ad405x_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_ad405x_config *cfg = dev->config;
	if (channel_cfg->channel_id != 0) {
		LOG_ERR("invalid channel id %d", channel_cfg->channel_id);
		return -EINVAL;
	}
	if (channel_cfg->differential != cfg->spec.channel_cfg.differential) {
		LOG_ERR("invalid mode %d", channel_cfg->differential);
		return -EINVAL;
	}

	uint8_t diff_mode = AD405X_SINGLE_ENDED;

	if (channel_cfg->differential) {
		diff_mode = AD405X_DIFFERENTIAL;
	}
	return ad405x_reg_update_bits(dev, AD405X_REG_ADC_MODES,
				AD405X_SINGLE_DIFFERENTIAL_MSK, diff_mode);
}

static int adc_ad405x_validate_buffer_size(const struct device *dev,
					const struct adc_sequence *sequence)
{
	uint8_t channels;
	size_t needed;

	channels = POPCOUNT(sequence->channels);
	needed = channels * sizeof(uint16_t);

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ad405x_conv_start(const struct device *dev)
{
	const struct adc_ad405x_config *cfg = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&cfg->conversion, 1);
	if (ret != 0) {
		return ret;
	}

	/** CNV High Time min 10 ns */
	k_busy_wait(1);
	ret = gpio_pin_set_dt(&cfg->conversion, 0);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{

	struct adc_ad405x_data *data = CONTAINER_OF(ctx, struct adc_ad405x_data, ctx);
	const struct adc_ad405x_config *cfg = (struct adc_ad405x_config *)data->dev->config;

	uint8_t reg_val_x[3] = {0};
	uint32_t data_be;
	size_t len = 0;

	switch (data->operation_mode) {
	case AD405X_SAMPLE_MODE_OP:
		len = 2;
		ad405x_conv_start(data->dev);
		break;
	case AD405X_BURST_AVERAGING_MODE_OP:
		if (cfg->chip_id == AD4052_CHIP_ID) {
			len = 3;
		}
		ad405x_conv_start(data->dev);
		break;
	case AD405X_AVERAGING_MODE_OP:
		if (cfg->chip_id == AD4052_CHIP_ID) {
			len = 3;
		}
		uint16_t conv_avg = avg_filter_values[data->filter_length];

		for (int i = 0; i < conv_avg; i++) {
			ad405x_conv_start(data->dev);
		}
		break;
	default:
		len = 2;
		break;
	}

#if CONFIG_AD405X_TRIGGER
	if (data->has_drdy != 0) {
		k_sem_take(&data->sem_drdy, K_FOREVER);
	}
#endif
	ad405x_read_raw(data->dev, reg_val_x, len);
	switch (len) {
	case 2:
		/* translate valid bytes to BE format */
		data_be = sys_get_be16(reg_val_x);
		break;
	case 3:
		data_be = sys_get_be24(reg_val_x);
		break;
	default:
		data_be = sys_get_be16(reg_val_x);
		break;
	}
	memcpy(ctx->sequence.buffer, &data_be, len);

	adc_context_on_sampling_done(&data->ctx, data->dev);
}

int ad405x_set_sample_rate(const struct device *dev, enum ad405x_sample_rate rate)
{
	struct adc_ad405x_data *data = dev->data;
	int ret;

	ret = ad405x_reg_update_bits(dev, AD405X_REG_TIMER_CONFIG, AD405X_FS_BURST_AUTO_MSK, rate);
	if (ret != 0) {
		return ret;
	}

	data->rate = rate;

	return 0;
}

int ad405x_set_averaging_filter_length(const struct device *dev, enum ad405x_avg_filter_l length)
{
	const struct adc_ad405x_config *cfg = dev->config;
	struct adc_ad405x_data *data = dev->data;
	int ret;

	/* Restrict filter length depending on active device selected */
	if (cfg->chip_id == AD4050_CHIP_ID) {
		if (length > AD405X_LENGTH_256) {
			return -EINVAL;
		}
	}
	ret = ad405x_reg_update_bits(dev, AD405X_REG_AVG_CONFIG, AD405X_AVG_WIN_LEN_MSK, length);
	if (ret) {
		return ret;
	}

	data->filter_length = length;

	return 0;
}

int ad405x_exit_command(const struct device *dev)
{
	int ret;

	const struct adc_ad405x_config *cfg = dev->config;
	struct adc_ad405x_data *data = dev->data;
	uint8_t access[1] = {0xA8};
	const struct spi_buf buf[1] = {{.buf = &access, .len = ARRAY_SIZE(access)}};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = 1,
	};

	ret = spi_write_dt(&cfg->bus.spi, &tx);
	if (!ret) {
		data->operation_mode = AD405X_CONFIG_MODE_OP;
	}

	return ret;
}

int ad405x_set_operation_mode(const struct device *dev, enum ad405x_operation_mode operation_mode)
{
	struct adc_ad405x_data *data = dev->data;
	int ret;

	if (data->operation_mode == AD405X_SLEEP_MODE_OP) {
		if (operation_mode != AD405X_SLEEP_MODE_OP) {
			ret = ad405x_reg_write_byte(dev, AD405X_REG_DEVICE_CONFIG,
						AD405X_ENTER_ACTIVE_MODE);
			if (ret != 0) {
				return ret;
			}
			if (operation_mode != AD405X_CONFIG_MODE_OP) {
				/* Set Operation mode. */
				ret = ad405x_reg_update_bits(dev, AD405X_REG_ADC_MODES,
							  AD405X_ADC_MODES_MSK, operation_mode);
				if (ret != 0) {
					return ret;
				}
				/* Enter ADC_MODE. */
				ret = ad405x_reg_update_bits(dev, AD405X_REG_MODE_SET,
							     AD405X_ENTER_ADC_MODE_MSK,
							     AD405X_ENTER_ADC_MODE);
				if (ret != 0) {
					return ret;
				}
			}
		}
	} else if (data->operation_mode == AD405X_CONFIG_MODE_OP) {
		if (operation_mode == AD405X_SLEEP_MODE_OP) {
			ret = ad405x_reg_write_byte(dev, AD405X_REG_DEVICE_CONFIG,
						AD405X_ENTER_SLEEP_MODE);
			if (ret != 0) {
				return ret;
			}
		} else {
			/* Set Operation mode. */
			ret = ad405x_reg_update_bits(dev, AD405X_REG_ADC_MODES,
						  AD405X_ADC_MODES_MSK, operation_mode);
			if (ret != 0) {
				return ret;
			}
			/* Enter ADC_MODE. */
			ret = ad405x_reg_update_bits(dev, AD405X_REG_MODE_SET,
						     AD405X_ENTER_ADC_MODE_MSK,
						     AD405X_ENTER_ADC_MODE);
			if (ret != 0) {
				return ret;
			}
		}
	} else {
		ret = ad405x_exit_command(dev);
		if (ret != 0) {
			return ret;
		};
		if (operation_mode == AD405X_SLEEP_MODE_OP) {
			ret = ad405x_reg_write_byte(dev, AD405X_REG_DEVICE_CONFIG,
						AD405X_ENTER_SLEEP_MODE);
			if (ret != 0) {
				return ret;
			}
		} else if (operation_mode != AD405X_CONFIG_MODE_OP) {
			ret = ad405x_reg_update_bits(dev, AD405X_REG_ADC_MODES,
						AD405X_ADC_MODES_MSK, operation_mode);
			if (ret != 0) {
				return ret;
			}
			/* Enter ADC_MODE. */
			ret = ad405x_reg_update_bits(dev, AD405X_REG_MODE_SET,
						     AD405X_ENTER_ADC_MODE_MSK,
						     AD405X_ENTER_ADC_MODE);
			if (ret != 0) {
				return ret;
			}
		}
	}
	data->operation_mode = operation_mode;
	return 0;
}

#if CONFIG_AD405X_TRIGGER
int ad405x_set_gpx_mode(const struct device *dev, uint8_t gp0_1, enum ad405x_gpx_mode gpx_mode)
{
	const struct adc_ad405x_config *cfg = dev->config;
	struct adc_ad405x_data *data = dev->data;
	uint8_t mask = AD405X_GP1_MODE_MSK;
	int ret;
	enum ad405x_gpx_mode gpx_mode_tmp = gpx_mode;

	if (gp0_1 == AD405X_GP0) {
		mask = AD405X_GP0_MODE_MSK;
		if (gpx_mode == AD405X_DEV_READY) {
			return -EINVAL;
		}
	} else {
		gpx_mode_tmp = gpx_mode_tmp << 4;
	}

	ret = ad405x_reg_update_bits(dev, AD405X_REG_GP_PIN_CONF, mask, gpx_mode_tmp);

	if (ret == 0) {
		if (gp0_1 == AD405X_GP0) {
			if (gpx_mode == AD405X_DATA_READY) {
				gpio_pin_interrupt_configure_dt(&cfg->gp0_interrupt,
								GPIO_INT_EDGE_TO_INACTIVE);
#ifdef CONFIG_AD405X_STREAM
				data->data_ready_gpio = AD405X_GP0;
#endif /* CONFIG_AD405X_STREAM */
			}
			data->gp0_mode = gpx_mode;
		} else {
			if (gpx_mode == AD405X_DATA_READY) {
				gpio_pin_interrupt_configure_dt(&cfg->gp1_interrupt,
								GPIO_INT_EDGE_TO_INACTIVE);
#ifdef CONFIG_AD405X_STREAM
				data->data_ready_gpio = AD405X_GP1;
#endif /* CONFIG_AD405X_STREAM */
			}
			data->gp1_mode = gpx_mode;
		}
	}
	return ret;
}
#endif

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ad405x_data *data = CONTAINER_OF(ctx, struct adc_ad405x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_ad405x_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_ad405x_config *cfg = dev->config;
	struct adc_ad405x_data *data = dev->data;
	int ret;

	if (cfg->chip_id == AD4050_CHIP_ID) {
		if (sequence->resolution != cfg->spec.resolution) {
			LOG_ERR("invalid resolution %d", sequence->resolution);
			return -EINVAL;
		}
	}
	if (cfg->chip_id == AD4052_CHIP_ID) {
		if (sequence->resolution != cfg->spec.resolution) {
			LOG_ERR("invalid resolution %d", sequence->resolution);
			return -EINVAL;
		}
	}

	ret = adc_ad405x_validate_buffer_size(dev, sequence);
	if (ret < 0) {
		LOG_ERR("insufficient buffer size");
		return ret;
	}

	ad405x_set_operation_mode(dev, cfg->active_mode);

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_ad405x_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct adc_ad405x_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = adc_ad405x_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int ad405x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_ad405x_read_async(dev, sequence, NULL);
}

static inline bool adc_ad405x_bus_is_ready(const struct device *dev)
{
	const struct adc_ad405x_config *cfg = dev->config;

	return ad405x_bus_is_ready_spi(&cfg->bus);
}

int ad405x_soft_reset(const struct device *dev)
{
	int ret;

	ret = ad405x_reg_update_bits(dev, AD405X_REG_INTERFACE_CONFIG_A, AD405X_SW_RESET_MSK,
					AD405X_REG_INTERFACE_CONFIG_A_RESET_VAL);
	if (ret < 0) {
		return ret;
	}

	return ad405x_reg_update_bits(dev, AD405X_REG_INTERFACE_CONFIG_A, AD405X_SW_RESET_MSK, 0);
}

static int adc_ad405x_init(const struct device *dev)
{
	const struct adc_ad405x_config *cfg = dev->config;
	struct adc_ad405x_data *data = dev->data;
	int ret;
	uint8_t reg_val;
	uint8_t reg_val_hi;
	uint16_t reg_val_res;

	if (!adc_ad405x_bus_is_ready(dev)) {
		LOG_ERR("bus not ready");
		return -ENODEV;
	}

#if CONFIG_AD405X_TRIGGER
	ret = ad405x_init_interrupt(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#else
	data->gp0_mode = AD405X_DISABLED;
#endif
	ad405x_init_conv(dev);
	data->gp1_mode = AD405X_DEV_READY;
	data->operation_mode = AD405X_CONFIG_MODE_OP;
	data->filter_length = AD405X_LENGTH_2;
	k_sem_init(&data->sem_devrdy, 0, 1);
	adc_context_init(&data->ctx);

	/* Reset */
	ret = ad405x_reset_pattern_cmd(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ad405x_reg_read_byte(dev, AD405X_REG_PRODUCT_ID_L, &reg_val);
	if (ret < 0) {
		return ret;
	}

	ret = ad405x_reg_read_byte(dev, AD405X_REG_PRODUCT_ID_H, &reg_val_hi);
	if (ret < 0) {
		return ret;
	}

	reg_val_res = (reg_val_hi << 8) | reg_val;
	if ((reg_val_res != AD4052_REG_PRODUCT_ID_VAL) &
	    (reg_val_res != AD4050_REG_PRODUCT_ID_VAL)) {
		LOG_ERR("Invalid product id");
		return -ENODEV;
	}

	ret = ad405x_reg_read_byte(dev, AD405X_REG_DEVICE_TYPE, &reg_val);
	if (ret < 0) {
		return ret;
	}

	if (reg_val != AD405X_REG_DEVICE_TYPE_VAL) {
		LOG_ERR("Invalid device type");
		return -ENODEV;
	}

	ret = ad405x_reg_read_byte(dev, AD405X_REG_VENDOR_L, &reg_val);
	if (ret < 0) {
		return ret;
	}

	ret = ad405x_reg_read_byte(dev, AD405X_REG_VENDOR_H, &reg_val_hi);
	if (ret < 0) {
		return ret;
	}

	reg_val_res = (reg_val_hi << 8) | reg_val;
	if (reg_val_res != AD405X_REG_VENDOR_VAL) {
		LOG_ERR("Invalid vendor value");
		return -ENODEV;
	}

	if (cfg->chip_id == AD4050_CHIP_ID) {
		if (cfg->spec.resolution != AD4050_ADC_RESOLUTION) {
			LOG_ERR("Invalid resolution %d", cfg->spec.resolution);
			return -EINVAL;
		}
	} else {
		if (cfg->spec.resolution != AD4052_ADC_RESOLUTION) {
			LOG_ERR("Invalid resolution %d", cfg->spec.resolution);
			return -EINVAL;
		}
	}

#if CONFIG_AD405X_TRIGGER
	if (cfg->has_gp1 != 0) {
		ad405x_set_gpx_mode(dev, AD405X_GP1, AD405X_DATA_READY);
		k_sem_init(&data->sem_drdy, 0, 1);
		data->has_drdy = 1;
	}
#endif

	adc_context_unlock_unconditionally(&data->ctx);
	data->dev = dev;
	return ret;
}

#ifdef CONFIG_AD405X_STREAM
void ad405x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct adc_ad405x_data *data = (struct adc_ad405x_data *)dev->data;
	const struct adc_ad405x_config *cfg_405 = (const struct adc_ad405x_config *)dev->config;

	if (data->data_ready_gpio > AD405X_GP1)	{
		LOG_ERR("DATA_READY irq is not enabled!");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	int rc;

	if (data->data_ready_gpio == AD405X_GP0) {
		rc = gpio_pin_interrupt_configure_dt(&cfg_405->gp0_interrupt,
					      GPIO_INT_DISABLE);
	} else {
		rc = gpio_pin_interrupt_configure_dt(&cfg_405->gp1_interrupt,
					      GPIO_INT_DISABLE);
	}

	if (rc < 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	if (data->operation_mode == AD405X_CONFIG_MODE_OP) {
		rc = ad405x_set_operation_mode(dev, cfg_405->active_mode);

		if (rc < 0) {
			LOG_ERR("Set operation mode failed!");
			return;
		}
		ad405x_timer_init(dev);
	}

	if (data->data_ready_gpio == AD405X_GP0) {
		rc = gpio_pin_interrupt_configure_dt(&cfg_405->gp0_interrupt,
					      GPIO_INT_EDGE_TO_INACTIVE);
	} else {
		rc = gpio_pin_interrupt_configure_dt(&cfg_405->gp1_interrupt,
					      GPIO_INT_EDGE_TO_INACTIVE);
	}

	if (rc < 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	data->sqe = iodev_sqe;
	ad405x_timer_start(dev);
}

static const uint32_t adc_ad405x_resolution[] = {
	[AD4050_6_12B_MODE] = 12,
	[AD4050_6_14B_MODE] = 14,
	[AD4052_8_16B_MODE] = 16,
	[AD4052_8_20B_MODE] = 20,
};

static inline void adc_ad405x_convert_q31(q31_t *out, const uint8_t *buff,
				enum ad405x_qscale_modes mode, uint8_t diff_mode,
				uint16_t vref_mv, uint8_t adc_shift)
{
	int32_t data_in = 0;
	uint32_t scale = BIT(adc_ad405x_resolution[mode]);

	/* In Differential mode, 1 bit is used for sign */
	if (diff_mode) {
		scale = BIT(adc_ad405x_resolution[mode] - 1);
	}

	uint32_t sensitivity = (vref_mv * (scale - 1)) / scale
			 * 1000 / scale; /* uV / LSB */

	switch (mode) {
	case AD4050_6_12B_MODE:
	case AD4050_6_14B_MODE:
	case AD4052_8_16B_MODE:
		data_in = sys_get_be16(buff);
		if (diff_mode && (data_in & (BIT(adc_ad405x_resolution[mode] - 1)))) {
			data_in |= ~BIT_MASK(adc_ad405x_resolution[mode]);
		}
		break;

	case AD4052_8_20B_MODE:
		data_in = sys_get_be24(buff);
		if (diff_mode && (data_in & (BIT(adc_ad405x_resolution[mode] - 1)))) {
			data_in |= ~BIT_MASK(adc_ad405x_resolution[mode]);
		}
		break;

	default:
		data_in = sys_get_be16(buff);
		break;
	}

	*out = BIT(31 - adc_shift)/* scaling to q_31*/ * sensitivity / 1000000/*uV to V*/ * data_in;
}

static int ad405x_decoder_get_frame_count(const uint8_t *buffer, uint32_t channel,
			       uint16_t *frame_count)
{
	const struct adc_ad405x_fifo_data *enc_data = (const struct adc_ad405x_fifo_data *)buffer;

	if (enc_data->empty) {
		return -ENODATA;
	}

	/* This adc does not have FIFO so it will stream one sample at a time */
	*frame_count = 1;

	return 0;
}

static int ad405x_decoder_decode(const uint8_t *buffer, uint32_t channel, uint32_t *fit,
		      uint16_t max_count, void *data_out)
{
	const struct adc_ad405x_fifo_data *enc_data = (const struct adc_ad405x_fifo_data *)buffer;

	if (*fit > 0) {
		return -ENOTSUP;
	}

	struct adc_data *data = (struct adc_data *)data_out;

	memset(data, 0, sizeof(struct adc_data));

	if (enc_data->empty) {
		data->header.base_timestamp_ns = 0;
		data->header.reading_count = 0;
		return -ENODATA;
	}

	data->header.base_timestamp_ns = enc_data->timestamp;
	data->header.reading_count = 1;

	/* 32 is used because input parameter for __builtin_clz func is
	 * unsigneg int (32 bits) and func will consider any input value
	 * as 32 bit.
	 */
	data->shift = 32 - __builtin_clz(enc_data->vref_mv);

	buffer += sizeof(struct adc_ad405x_fifo_data);

	data->readings[0].timestamp_delta = 0;
	adc_ad405x_convert_q31(&data->readings[0].value, buffer, enc_data->ad405x_qscale_mode,
				enc_data->diff_mode, enc_data->vref_mv, data->shift);

	*fit = 1;

	return 0;
}

static void ad405x_process_sample_cb(struct rtio *r, const struct rtio_sqe *sqe, int res, void *arg)
{
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void ad405x_stream_irq_handler(const struct device *dev)
{
	struct adc_ad405x_data *data = (struct adc_ad405x_data *)dev->data;
	const struct adc_ad405x_config *cfg = (const struct adc_ad405x_config *)dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint32_t sample_size = 2;
	enum ad405x_qscale_modes qscale_mode = AD4050_6_12B_MODE;
	struct adc_read_config *read_config = (struct adc_read_config *)data->sqe->sqe.iodev->data;

	if (read_config == NULL) {
		return;
	}

	if (current_sqe == NULL) {
		return;
	}

	if (cfg->chip_id == AD4050_CHIP_ID) {
		if ((cfg->active_mode == AD405X_BURST_AVERAGING_MODE_OP)
					|| (cfg->active_mode == AD405X_AVERAGING_MODE_OP)) {
			qscale_mode = AD4050_6_14B_MODE;
		}
	} else { /* AD4052_CHIP_ID */
		if ((cfg->active_mode == AD405X_BURST_AVERAGING_MODE_OP)
					|| (cfg->active_mode == AD405X_AVERAGING_MODE_OP)) {
			sample_size = 3;
			qscale_mode = AD4052_8_20B_MODE;
		} else {
			qscale_mode = AD4052_8_16B_MODE;
		}
	}

#if DT_HAS_CHOSEN(zephyr_adc_clock)
	uint32_t ticks;
	int ret = counter_get_value(data->timer_dev, &ticks);

	if (ret != 0) {
		LOG_ERR("Failed to get timer value");
		data->timestamp = 0;
		return;
	}

	data->timestamp = (uint64_t)(counter_ticks_to_us(data->timer_dev, ticks))
				* 1000 /* uS to nS */;
#else
	data->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
#endif
	data->sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		return;
	}

	const size_t min_read_size = sizeof(struct adc_ad405x_fifo_data) + sample_size;

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, min_read_size, &buf, &buf_len) != 0) {
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		return;
	}

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	struct adc_ad405x_fifo_data *hdr = (struct adc_ad405x_fifo_data *)buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->diff_mode = cfg->spec.channel_cfg.differential;
	hdr->vref_mv = cfg->spec.vref_mv;
	hdr->ad405x_qscale_mode = qscale_mode;

	uint8_t *read_buf = buf + sizeof(*hdr);

	if (read_config->trigger_cnt != 0) {
		enum adc_stream_data_opt data_opt = read_config->triggers[0].opt;

		for (int i = 1; i < read_config->trigger_cnt; i++) {
			data_opt = MIN(data_opt, read_config->triggers[i].opt);
		}

		if (data_opt == ADC_STREAM_DATA_NOP || data_opt == ADC_STREAM_DATA_DROP) {
			hdr->empty = 1;
		}
	}

	/* Setup new rtio chain to read the data and report then check the result */
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, read_buf, sample_size,
				current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, ad405x_process_sample_cb, (void *)dev, current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

static bool ad405x_decoder_has_trigger(const uint8_t *buffer, enum adc_trigger_type trigger)
{
	const struct adc_ad405x_fifo_data *data = (const struct adc_ad405x_fifo_data *)buffer;

	if (!data->is_fifo) {
		return false;
	}

	switch (trigger) {
	/* This family of chips doesn't have FIFO so if there is a buffer trigger has happen. */
	case ADC_TRIG_DATA_READY:
	case ADC_TRIG_FIFO_WATERMARK:
	case ADC_TRIG_FIFO_FULL:
		return true;
	default:
		return false;
	}
}

ADC_DECODER_API_DT_DEFINE() = {
	.get_frame_count = ad405x_decoder_get_frame_count,
	.decode = ad405x_decoder_decode,
	.has_trigger = ad405x_decoder_has_trigger,
};

int ad405x_get_decoder(const struct device *dev, const struct adc_decoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &ADC_DECODER_NAME();

	return 0;
}

#endif /* CONFIG_AD405X_STREAM */

static DEVICE_API(adc, ad405x_api_funcs) = {
	.channel_setup = ad405x_channel_setup,
	.read = ad405x_read,
	.ref_internal = 2500,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ad405x_adc_read_async,
#endif /* CONFIG_ADC_ASYNC */
#ifdef CONFIG_AD405X_STREAM
	.submit = ad405x_submit_stream,
	.get_decoder = ad405x_get_decoder,
#endif /* CONFIG_AD405X_STREAM */
};

#define AD405X_SPI_CFG SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define AD405X_RTIO_DEFINE(inst)                                                                  \
	SPI_DT_IODEV_DEFINE(ad405x_iodev_##inst, DT_DRV_INST(inst), AD405X_SPI_CFG, 0U);         \
	RTIO_DEFINE(ad405x_rtio_ctx_##inst, 16, 16);

#define DT_INST_AD405X(inst, t) DT_INST(inst, adi_ad##t##_adc)

#define AD405X_GPIO_PROPS1(n)                                                                    \
	.gp1_interrupt = GPIO_DT_SPEC_INST_GET_OR(n, gp1_gpios, {0}),                            \
	.has_gp1 = 1,                                                                            \

#define AD405X_GPIO_PROPS0(n)                                                                    \
	.gp0_interrupt = GPIO_DT_SPEC_INST_GET_OR(n, gp0_gpios, {0}),                            \
	.has_gp0 = 1,                                                                            \

#define AD405X_GPIO(t, n)                                                                        \
	IF_ENABLED(DT_NODE_HAS_PROP(DT_INST_AD405X(n, t), gp1_gpios),                            \
		   (AD405X_GPIO_PROPS1(n)))                                                      \
	IF_ENABLED(DT_NODE_HAS_PROP(DT_INST_AD405X(n, t), gp0_gpios),                            \
		   (AD405X_GPIO_PROPS0(n)))

#define AD405X_INIT(t, n) \
	IF_ENABLED(CONFIG_AD405X_STREAM, (AD405X_RTIO_DEFINE(n)));				\
	static struct adc_ad405x_data ad##t##_data_##n = {					\
	IF_ENABLED(CONFIG_AD405X_STREAM, (.rtio_ctx = &ad405x_rtio_ctx_##n,			\
				.iodev = &ad405x_iodev_##n, .data_ready_gpio = AD405X_NO_GPIO,	\
				COND_CODE_1(DT_HAS_CHOSEN(zephyr_adc_clock),			\
				(.timer_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_adc_clock)),),	\
				())))								\
	};											\
	static const struct adc_ad405x_config ad##t##_config_##n =  {				\
		.bus = {.spi = SPI_DT_SPEC_GET(DT_INST_AD405X(n, t), AD405X_SPI_CFG, 0)},	\
		.conversion = GPIO_DT_SPEC_GET_BY_IDX(DT_INST_AD405X(n, t),			\
							conversion_gpios, 0),			\
		IF_ENABLED(CONFIG_AD405X_TRIGGER, (AD405X_GPIO(t, n)))				\
		.chip_id = t,									\
		.active_mode = AD405X_SAMPLE_MODE_OP,						\
		.spec = ADC_DT_SPEC_STRUCT(DT_INST(n, DT_DRV_COMPAT), 0),			\
		IF_ENABLED(CONFIG_AD405X_STREAM, (.sampling_period =				\
			DT_INST_PROP_OR(n, sampling_period, AD405X_DEF_SAMPLING_PERIOD),))	\
		};										\
	DEVICE_DT_DEFINE(DT_INST_AD405X(n, t), adc_ad405x_init, NULL, &ad##t##_data_##n,	\
			&ad##t##_config_##n, POST_KERNEL,					\
			CONFIG_ADC_INIT_PRIORITY, &ad405x_api_funcs);

/*
 * AD4052: 16 bit
 */
#define AD4052_INIT(n) AD405X_INIT(AD4052_CHIP_ID, n)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_ad4052_adc
DT_INST_FOREACH_STATUS_OKAY(AD4052_INIT)

/*
 * AD4050: 12 bit
 */
#define AD4050_INIT(n) AD405X_INIT(AD4050_CHIP_ID, n)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_ad4050_adc
DT_INST_FOREACH_STATUS_OKAY(AD4050_INIT)
