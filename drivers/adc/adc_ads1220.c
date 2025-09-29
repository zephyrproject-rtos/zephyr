/*
 * Copyright (c) 2025 Baumer Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc/ads1220.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

/* Enable ADC context with kernel timer support */
#define ADC_CONTEXT_USES_KERNEL_TIMER 1

#include "adc_context.h"

/* Device tree compatibility string */
#define DT_DRV_COMPAT ti_ads1220

LOG_MODULE_REGISTER(ads1220, CONFIG_ADC_LOG_LEVEL);

#define IDAC1_ROUTING_INST(n) (DT_INST_PROP_OR(n, idac1_routing, 0))
#define IDAC2_ROUTING_INST(n) (DT_INST_PROP_OR(n, idac2_routing, 0))

#define IDAC1_ROUTING(n)  (uint8_t)(IDAC1_ROUTING_INST(n) << ADS1220_REG3_I1MUX_POS)
#define IDAC2_ROUTING(n)  (uint8_t)(IDAC2_ROUTING_INST(n) << ADS1220_REG3_I2MUX_POS)
#define MAGNITUDE_UA(n)   (uint8_t)(DT_INST_PROP_OR(n, idac_magnitude_microamp, 0))
#define DATA_RATE(n)      (uint8_t)(DT_INST_PROP_OR(n, data_rate, 0))
#define OPERATING_MODE(n) (uint8_t)(DT_INST_PROP_OR(n, operating_mode, 0))

/* Configuration Registers */
#define ADS1220_REG0 0x00u /**< Configuration register 0 */
#define ADS1220_REG1 0x01u /**< Configuration register 1 */
#define ADS1220_REG2 0x02u /**< Configuration register 2 */
#define ADS1220_REG3 0x03u /**< Configuration register 3 */

/* SPI Communication Commands */
#define ADS1220_CMD_RESET      0x06u                  /**< Reset command */
#define ADS1220_CMD_START_SYNC 0x08u                  /**< Start/Sync command */
#define ADS1220_CMD_POWERDOWN  0x02u                  /**< Power-down command */
#define ADS1220_CMD_RDATA      0x10u                  /**< Read data command */
#define ADS1220_CMD_RREG(reg)  (0x20u | ((reg) << 2)) /**< Read register command */
#define ADS1220_CMD_WREG(reg)  (0x40u | ((reg) << 2)) /**< Write register command */

#define ADS1220_REF_INTERNAL 2048 /**< Internal reference voltage in mV */

#define NR_OF_OPERATING_MODES 3
#define NR_OF_DATA_RATE_MODES 7

static const uint32_t t_clk[NR_OF_DATA_RATE_MODES][NR_OF_OPERATING_MODES] = {
	{204850u, 823120u, 102434u}, {91218u, 364560u, 45618u}, {46226u, 184592u, 23122u},
	{23762u, 94736u, 11890u},    {12562u, 49936u, 6290u},   {6994u, 27664u, 3506u},
	{4242u, 16656u, 2130u},
};

#define ADC_ADS1220_STARTUP_DELAY_US    50
#define IDAC_PROGRAMMINT_TIME_US        200
#define ADC_CONTEXT_WAIT_FOR_COMPLETION K_MSEC(CONFIG_ADC_ADS1220_WAIT_FOR_COMPLETION_TIMEOUT_MS)

#define ADC_ADS1220_RESET_DELAY_US(operating_mode, data_rate)                                      \
	(50u + (32u * t_clk[data_rate][operating_mode]))
struct ads1220_config {
	/** SPI bus configuration */
	struct spi_dt_spec bus;

	/** Data ready GPIO specification (optional) */
	struct gpio_dt_spec gpio_data_ready;

	/** IDAC1/2 output routing */
	uint8_t config3;

	/** IDAC current magnitude in microamperes */
	uint16_t idac_magnitude_ua;

	/** Data rate setting */
	uint8_t data_rate;

	/** Operating mode */
	uint8_t operating_mode;

	bool pga_bypass;

	struct adc_channel_cfg dts_channel_cfg;
};

struct ads1220_data {
	/** ADC context for timing and synchronization */
	struct adc_context ctx;

	/** Current buffer pointer for conversions */
	int32_t *buffer;

	/** Repeat buffer pointer for multi-sampling */
	int32_t *buffer_ptr;

	struct k_sem data_ready_signal;

	/** Acquisition semaphore for synchronization */
	struct k_sem acquire_signal;

	/** Current configuration registers cache */
	uint8_t config_regs[4];

	/** GPIO callback for data ready interrupt */
	struct gpio_callback callback_data_ready;
};

struct ads1220_idac_info {
	uint8_t idac_setting; /**< IDAC register setting */
	uint16_t current_ua;  /**< Current in microamperes */
};

static const struct ads1220_idac_info ads1220_idacs[] = {
	{ADS1220_REG2_IDAC_OFF, 0},       {ADS1220_REG2_IDAC_10UA, 10},
	{ADS1220_REG2_IDAC_50UA, 50},     {ADS1220_REG2_IDAC_100UA, 100},
	{ADS1220_REG2_IDAC_250UA, 250},   {ADS1220_REG2_IDAC_500UA, 500},
	{ADS1220_REG2_IDAC_1000UA, 1000}, {ADS1220_REG2_IDAC_1500UA, 1500},
};

static int ads1220_read_sample(const struct device *dev, int32_t *buffer)
{
	const struct ads1220_config *config = dev->config;
	uint8_t buffer_tx[4];
	uint8_t buffer_rx[4];

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

	buffer_tx[0] = (uint8_t)ADS1220_CMD_RDATA;

	int result = spi_transceive_dt(&config->bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("%s: spi_transceive failed with error %i", dev->name, result);
		return result;
	}

	/* Combine 3 bytes into 24-bit signed integer */
	int32_t raw_data = ((int32_t)(buffer_rx[1] << 16)) | ((int32_t)(buffer_rx[2] << 8)) |
			   ((int32_t)buffer_rx[3]);

	/* Sign extend from 24-bit to 32-bit */
	if ((raw_data & 0x800000) == 0x800000) {
		raw_data |= 0xFF000000u;
	}

	*buffer = raw_data;
	LOG_DBG("%s: Read data: 0x%06X (%d)", dev->name, raw_data & 0xFFFFFF, raw_data);

	return 0;
}

static int ads1220_send_command(const struct device *dev, uint8_t cmd)
{
	const struct ads1220_config *config = dev->config;
	uint8_t buffer_tx[1];
	int result;

	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	buffer_tx[0] = (uint8_t)cmd;

	LOG_DBG("%s: sending command 0x%02X", dev->name, cmd);
	result = spi_write_dt(&config->bus, &tx);

	if (result != 0) {
		LOG_ERR("%s: spi_write failed with error %i", dev->name, result);
		return result;
	}

	return result;
}

static int ads1220_write_register(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct ads1220_config *config = dev->config;
	struct ads1220_data *data = dev->data;
	uint8_t tx_buf[2];
	int ret;

	if (reg > ADS1220_REG3) {
		LOG_ERR("Invalid register address: %d", reg);
		return -EINVAL;
	}
	if (data->config_regs[reg] == value) {
		LOG_DBG("Register %d already written.", reg);
		return 0;
	}

	tx_buf[0] = ADS1220_CMD_WREG(reg);
	tx_buf[1] = value;

	const struct spi_buf tx_spi_buf = {
		.buf = tx_buf,
		.len = 2,
	};
	const struct spi_buf_set tx_spi_buf_set = {
		.buffers = &tx_spi_buf,
		.count = 1,
	};

	LOG_DBG("Writing register %d: 0x%02X", reg, value);

	ret = spi_write_dt(&config->bus, &tx_spi_buf_set);
	if (ret == 0) {
		/* Update local cache */
		data->config_regs[reg] = value;
	} else {
		LOG_ERR("Failed to write register %d: %d", reg, ret);
	}

	return ret;
}

static int ads1220_read_register(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const struct ads1220_config *config = dev->config;
	uint8_t buffer_tx[2];
	uint8_t buffer_rx[2];

	if (reg > ADS1220_REG3) {
		LOG_ERR("Invalid register address: %d", reg);
		return -EINVAL;
	}

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

	buffer_tx[0] = ADS1220_CMD_RREG(reg);
	/* read one register */
	buffer_tx[1] = 0x00;

	int result = spi_transceive_dt(&config->bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("%s: spi_transceive failed with error %i (MISO issue?)", dev->name, result);
		return result;
	}

	*value = buffer_rx[1];
	LOG_DBG("%s: read from register 0x%02X value 0x%02X", dev->name, reg, *value);

	return 0;
}

static int ads1220_adc_config_to_mux(uint8_t input_positive, uint8_t input_negative,
				     bool differential, uint8_t *mux_config)
{
	if (!mux_config) {
		return -EINVAL;
	}

	if (differential) {
		/* Differential measurements */
		if ((input_positive == 0u) && (input_negative == 1u)) {
			*mux_config = ADS1220_REG0_MUX_AIN0_AIN1;
		} else if ((input_positive == 0u) && (input_negative == 2u)) {
			*mux_config = ADS1220_REG0_MUX_AIN0_AIN2;
		} else if ((input_positive == 0u) && (input_negative == 3u)) {
			*mux_config = ADS1220_REG0_MUX_AIN0_AIN3;
		} else if ((input_positive == 1u) && (input_negative == 2u)) {
			*mux_config = ADS1220_REG0_MUX_AIN1_AIN2;
		} else if ((input_positive == 1u) && (input_negative == 3u)) {
			*mux_config = ADS1220_REG0_MUX_AIN1_AIN3;
		} else if ((input_positive == 2u) && (input_negative == 3u)) {
			*mux_config = ADS1220_REG0_MUX_AIN2_AIN3;
		} else if ((input_positive == 1u) && (input_negative == 0u)) {
			*mux_config = ADS1220_REG0_MUX_AIN1_AIN0;
		} else if ((input_positive == 3u) && (input_negative == 2u)) {
			*mux_config = ADS1220_REG0_MUX_AIN3_AIN2;
		} else {
			LOG_ERR("Invalid differential input pair: %d-%d", input_positive,
				input_negative);
			return -EINVAL;
		}
	} else {
		/* Single-ended or special measurements */
		switch (input_positive) {
		case 0:
			*mux_config = ADS1220_REG0_MUX_AIN0_AVSS;
			break;
		case 1:
			*mux_config = ADS1220_REG0_MUX_AIN1_AVSS;
			break;
		case 2:
			*mux_config = ADS1220_REG0_MUX_AIN2_AVSS;
			break;
		case 3:
			*mux_config = ADS1220_REG0_MUX_AIN3_AVSS;
			break;
		default:
			LOG_ERR("Invalid input pin: %d", input_positive);
			return -EINVAL;
		}
	}

	return 0;
}

static int ads1220_adc_config_gain(uint8_t gain, uint8_t *gain_config)
{

	if (!gain_config) {
		return -EINVAL;
	}

	switch (gain) {
	case ADC_GAIN_1:
		*gain_config = ADS1220_REG0_GAIN_1;
		break;
	case ADC_GAIN_2:
		*gain_config = ADS1220_REG0_GAIN_2;
		break;
	case ADC_GAIN_4:
		*gain_config = ADS1220_REG0_GAIN_4;
		break;
	case ADC_GAIN_8:
		*gain_config = ADS1220_REG0_GAIN_8;
		break;
	case ADC_GAIN_16:
		*gain_config = ADS1220_REG0_GAIN_16;
		break;
	case ADC_GAIN_32:
		*gain_config = ADS1220_REG0_GAIN_32;
		break;
	case ADC_GAIN_64:
		*gain_config = ADS1220_REG0_GAIN_64;
		break;
	case ADC_GAIN_128:
		*gain_config = ADS1220_REG0_GAIN_128;
		break;
	default:
		LOG_ERR("Invalid gain: %d", gain);
		return -EINVAL;
	}
	return 0;
}

static int ads1220_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct ads1220_config *config = dev->config;
	uint8_t config0 = 0, config1 = 0, config2 = 0;
	int ret;

	/* Validate channel ID */
	if (channel_cfg->channel_id != 0) {
		LOG_ERR("Only channel 0 supported (multiplexed inputs)");
		return -EINVAL;
	}

	/* Configure input multiplexer using improved mapping */
	uint8_t mux_config;

	ret = ads1220_adc_config_to_mux(channel_cfg->input_positive, channel_cfg->input_negative,
					channel_cfg->differential, &mux_config);
	if (ret < 0) {
		LOG_ERR("Failed to convert ADC config to MUX setting");
		return ret;
	}

	LOG_DBG("Configured MUX: %d (differential=%s, pos=%d, neg=%d)", mux_config,
		channel_cfg->differential ? "true" : "false", channel_cfg->input_positive,
		channel_cfg->input_negative);

	config0 |= (mux_config);

	/* Configure gain */

	uint8_t gain_config;

	ret = ads1220_adc_config_gain(channel_cfg->gain, &gain_config);

	if (ret < 0) {
		LOG_ERR("Failed to convert ADC gain setting");
		return ret;
	}

	config0 |= (gain_config);

	/* Configure PGA bypass if enabled */
	if (config->pga_bypass) {
		config0 |= ADS1220_REG0_PGA_BYPASS;
	}

	/* Configure data rate from acquisition time */
	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		/* For now, use default data rate - acquisition time mapping can be implemented
		 * later
		 */
		LOG_WRN("Custom acquisition time not fully implemented, using default data rate");
	}

	/* Use configured data rate */
	config1 |= (config->data_rate << ADS1220_REG1_DR_POS);
	config1 |= (config->operating_mode << ADS1220_REG1_MODE_POS);

	/* Use single-shot mode (default) */
	/* CM bit = 0 for single-shot, so we don't set ADS1220_REG1_CM */

	/* Configure reference */
	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		config2 |= (ADS1220_REG2_VREF_INT);
		break;
	case ADC_REF_EXTERNAL0:
		config2 |= (ADS1220_REG2_VREF_EXT_0);
		break;
	case ADC_REF_EXTERNAL1:
		config2 |= (ADS1220_REG2_VREF_EXT_1);
		break;
	case ADC_REF_VDD_1:
		config2 |= (ADS1220_REG2_VREF_SUPPLY);
		break;
	default:
		LOG_ERR("Invalid reference: %d", channel_cfg->reference);
		return -EINVAL;
	}

	/* Configure IDAC */
	bool idac_found = false;

	for (size_t i = 0; i < ARRAY_SIZE(ads1220_idacs); i++) {
		if (ads1220_idacs[i].current_ua == config->idac_magnitude_ua) {
			idac_found = true;
			config2 |= ads1220_idacs[i].idac_setting;
			break;
		}
	}

	if (!idac_found) {
		LOG_ERR("Invalid IDAC magnitude: %d", config->idac_magnitude_ua);
		return -EINVAL;
	}

	/* Write configuration registers */
	ret = ads1220_write_register(dev, ADS1220_REG0, config0);
	if (ret < 0) {
		LOG_ERR("Failed to write CONFIG0: %d", ret);
		return ret;
	}

	ret = ads1220_write_register(dev, ADS1220_REG1, config1);
	if (ret < 0) {
		LOG_ERR("Failed to write CONFIG1: %d", ret);
		return ret;
	}

	ret = ads1220_write_register(dev, ADS1220_REG2, config2);
	if (ret < 0) {
		LOG_ERR("Failed to write CONFIG2: %d", ret);
		return ret;
	}

	return 0;
}

static void ads1220_data_ready_handler(const struct device *dev, struct gpio_callback *gpio_cb,
				       uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct ads1220_data *data = (struct ads1220_data *)((char *)gpio_cb -
							    offsetof(struct ads1220_data,
								     callback_data_ready));

	k_sem_give(&data->data_ready_signal);
}

static int ads1220_wait_data_ready(const struct device *dev)
{
	struct ads1220_data *data = dev->data;

	return k_sem_take(&data->data_ready_signal, ADC_CONTEXT_WAIT_FOR_COMPLETION);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads1220_data *data =
		(struct ads1220_data *)((char *)ctx - offsetof(struct ads1220_data, ctx));

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads1220_data *data =
		(struct ads1220_data *)((char *)ctx - offsetof(struct ads1220_data, ctx));

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acquire_signal);
}

static int ads1220_validate_sequence(const struct adc_sequence *sequence)
{
	/* Validate resolution */
	if (sequence->resolution != ADS1220_RESOLUTION) {
		LOG_ERR("Invalid resolution %d, must be %d", sequence->resolution,
			ADS1220_RESOLUTION);
		return -EINVAL;
	}

	/* Validate channels - only single channel (0) supported */
	if (sequence->channels != BIT(0)) {
		LOG_ERR("Invalid channels 0x%08X, only channel 0 supported", sequence->channels);
		return -EINVAL;
	}

	/* Validate oversampling */
	if (sequence->oversampling != 0) {
		LOG_ERR("Oversampling not supported");
		return -ENOTSUP;
	}

	/* Validate buffer size */
	size_t needed_size = sizeof(int32_t);

	if (sequence->options != 0) {
		needed_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_size) {
		LOG_ERR("Buffer size %d too small, need %d", sequence->buffer_size, needed_size);
		return -ENOMEM;
	}

	return 0;
}

static int ads1220_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ads1220_data *data = dev->data;
	int ret;

	/* Validate sequence parameters */
	ret = ads1220_validate_sequence(sequence);
	if (ret < 0) {
		return ret;
	}

	/* Set buffer pointer */
	data->buffer = (int32_t *)sequence->buffer;

	/* Start ADC context */
	adc_context_start_read(&data->ctx, sequence);
	return ret;
}

static int ads1220_adc_perform_read(const struct device *dev)
{
	int result;
	struct ads1220_data *data = dev->data;

	k_sem_take(&data->acquire_signal, K_FOREVER);
	k_sem_take(&data->data_ready_signal, K_NO_WAIT);

	result = ads1220_send_command(dev, ADS1220_CMD_START_SYNC);
	if (result != 0) {
		LOG_ERR("%s: unable to send START/SYNC command", dev->name);
		adc_context_complete(&data->ctx, result);
		return result;
	}

	result = ads1220_wait_data_ready(dev);
	if (result != 0) {
		LOG_ERR("%s: waiting for data to be ready failed", dev->name);
		adc_context_complete(&data->ctx, result);
		return result;
	}

	result = ads1220_read_sample(dev, data->buffer);
	if (result != 0) {
		LOG_ERR("%s: reading sample failed", dev->name);
		adc_context_complete(&data->ctx, result);
		return result;
	}

	data->buffer++;

	adc_context_on_sampling_done(&data->ctx, dev);

	return result;
}

static int ads1220_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ads1220_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);

	ret = ads1220_start_read(dev, sequence);

	while ((ret == 0) && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		ret = ads1220_adc_perform_read(dev);
	}

	adc_context_release(&data->ctx, ret);
	return ret;
}

static int ads1220_reset(const struct device *dev)
{
	int ret;
	const struct ads1220_config *config = dev->config;

	/* Software reset via SPI command */
	LOG_DBG("Performing software reset");
	ret = ads1220_send_command(dev, ADS1220_CMD_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to send reset command: %d", ret);
		return ret;
	}

	/* Wait for device to be ready after reset */

	uint32_t reset_delay =
		(uint32_t)ADC_ADS1220_RESET_DELAY_US(config->operating_mode, config->data_rate);

	k_usleep(reset_delay);
	return 0;
}

static int ads1220_init(const struct device *dev)
{
	const struct ads1220_config *config = dev->config;
	struct ads1220_data *data = dev->data;
	int ret;

	LOG_INF("Initializing ADS1220 ADC");

#ifdef CONFIG_ADC_ASYNC
	if (data->ctx.asynchronous != 0) {
		LOG_ERR("The driver does currently not support asynchronous access!");
		return -ENODEV;
	}
#endif /* CONFIG_ADC_ASYNC */

	/* Initialize ADC context */
	adc_context_init(&data->ctx);

	/* Initialize semaphore */
	k_sem_init(&data->acquire_signal, 0, 1);
	k_sem_init(&data->data_ready_signal, 0, 1);

	/* Check SPI bus readiness */
	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: SPI device is not ready", dev->name);
		return -ENODEV;
	}

	/* Configure DRDY GPIO */
	if (!device_is_ready(config->gpio_data_ready.port)) {
		LOG_ERR("DRDY GPIO port not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->gpio_data_ready, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure DRDY GPIO: %d", ret);
		return ret;
	}

	/* Setup interrupt callback */
	gpio_init_callback(&data->callback_data_ready, ads1220_data_ready_handler,
			   BIT(config->gpio_data_ready.pin));

	ret = gpio_add_callback(config->gpio_data_ready.port, &data->callback_data_ready);
	if (ret < 0) {
		LOG_ERR("Failed to add DRDY callback: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->gpio_data_ready, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure DRDY interrupt: %d", ret);
		return ret;
	}

	/* Wait for device startup */
	k_usleep(ADC_ADS1220_STARTUP_DELAY_US);

	/* Reset device */
	ret = ads1220_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset device: %d", ret);
		return ret;
	}

	LOG_INF("ADS1220 initialization completed successfully");

	/* Verify device communication by reading the registers */
	if (ret == 0) {
		ret = ads1220_read_register(dev, ADS1220_REG0, &data->config_regs[0]);
	}
	if (ret == 0) {
		ret = ads1220_read_register(dev, ADS1220_REG1, &data->config_regs[1]);
	}
	if (ret == 0) {
		ret = ads1220_read_register(dev, ADS1220_REG2, &data->config_regs[2]);
	}
	if (ret == 0) {
		ret = ads1220_read_register(dev, ADS1220_REG3, &data->config_regs[3]);
	}
	if (ret < 0) {
		LOG_ERR("Failed to read CONFIG config_regs: %d", ret);
		return ret;
	}

	/* Configure IDAC routing */
	ret = ads1220_write_register(dev, ADS1220_REG3, config->config3);
	if (ret < 0) {
		LOG_ERR("Failed to write CONFIG3: %d", ret);
		return ret;
	}

	/* initialize adc with channel_0 devicetree node */
	ret = ads1220_channel_setup(dev, &config->dts_channel_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to setup default channel: %d", ret);
		return ret;
	}

	k_usleep(IDAC_PROGRAMMINT_TIME_US);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api ads1220_driver_api = {
	.channel_setup = ads1220_channel_setup,
	.read = ads1220_read,
	.ref_internal = ADS1220_REF_INTERNAL,
};

#define CHECK_1220_CONFIGURATION(n)                                                                \
	!(((IDAC1_ROUTING(n) & ADS1220_REG3_I1MUX_MSK) != IDAC1_ROUTING(n)) ||                     \
	  ((IDAC2_ROUTING(n) & ADS1220_REG3_I2MUX_MSK) != IDAC2_ROUTING(n)) ||                     \
	  ((DATA_RATE(n) & (ADS1220_REG1_DR_MSK >> ADS1220_REG1_DR_POS)) != DATA_RATE(n)) ||       \
	  ((OPERATING_MODE(n) & (ADS1220_REG1_MODE_MSK >> ADS1220_REG1_MODE_POS)) !=               \
	   OPERATING_MODE(n)))

/* Device instantiation macro */
#define ADS1220_INIT(n)                                                                            \
	static const struct ads1220_config ads1220_config_##n = {                                  \
		.bus = SPI_DT_SPEC_INST_GET(n,                                                     \
					    SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8)), \
		.gpio_data_ready = GPIO_DT_SPEC_INST_GET(n, drdy_gpios),                           \
		.config3 = IDAC1_ROUTING(n) | IDAC2_ROUTING(n),                                    \
		.idac_magnitude_ua = MAGNITUDE_UA(n),                                              \
		.data_rate = DATA_RATE(n),                                                         \
		.operating_mode = OPERATING_MODE(n),                                               \
		.pga_bypass = DT_INST_PROP(n, pga_bypass),                                         \
		.dts_channel_cfg = ADC_CHANNEL_CFG_DT(DT_CHILD(DT_DRV_INST(n), channel_0)),        \
	};                                                                                         \
                                                                                                   \
	BUILD_ASSERT(CHECK_1220_CONFIGURATION(n), "ADS1220 configuration invalid");                \
	BUILD_ASSERT(DT_INST_PROP(n, continuous_convert) == false,                                 \
		     "ADS1220 does currently not support continuous conversion");                  \
	static struct ads1220_data ads1220_data_##n;                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ads1220_init, NULL, &ads1220_data_##n, &ads1220_config_##n,       \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &ads1220_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADS1220_INIT)
