/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adt7604

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <zephyr/drivers/sensor/adt7604.h>

#include "adt7604.h"
#include "adt7604_bus.h"

LOG_MODULE_REGISTER(adt7604, CONFIG_SENSOR_LOG_LEVEL);

/* Custom sensor data table constants */
#define ADT7604_MAX_CUSTOM_TABLE_SIZE 384 /* 64 pairs × 6 bytes */

/**
 * @brief Read raw result register
 *
 * @param dev Pointer to the device structure
 * @param reg_addr Register address to read from
 * @param raw_result Pointer to store the raw 32-bit result
 * @return 0 on success, negative error code otherwise
 */
static int adt7604_read_raw_result(const struct device *dev, uint16_t reg_addr,
				   uint32_t *raw_result)
{
	uint8_t raw_data[4];
	int ret;

	ret = adt7604_bus_read(dev, reg_addr, raw_data, 4);
	if (ret < 0) {
		return ret;
	}

	*raw_result = sys_get_be32(raw_data);

	return 0;
}

/**
 * @brief Validate and extract temperature result with fault checking
 *
 * @param raw_result Raw 32-bit temperature result register value
 * @param result Pointer to store the extracted temperature value
 * @return 0 on success, -ENODATA if result is invalid
 */
static int adt7604_validate_temperature_result(uint32_t raw_result, int32_t *result)
{
	if (!(raw_result & ADT7604_RES_VALID)) {
		if (raw_result & ADT7604_RES_HARD_SENSOR_FAULT) {
			LOG_ERR("Sensor hard fault: Bad sensor reading");
		}
		if (raw_result & ADT7604_RES_HARD_ADC_OUT_OF_RANGE) {
			LOG_ERR("Hard ADC Out-of-Range: Bad ADC reading");
		}
		return -ENODATA;
	}

	if (raw_result & ADT7604_RES_SOFT_FAULT_MASK) {
		if (raw_result & ADT7604_RES_SOFT_SENSOR_OVER_RANGE) {
			LOG_WRN("Soft sensor over range: Sensor reading above normal");
		}
		if (raw_result & ADT7604_RES_SOFT_SENSOR_UNDER_RANGE) {
			LOG_WRN("Soft sensor under range: Sensor reading below normal");
		}
		if (raw_result & ADT7604_RES_SOFT_ADC_OUT_OF_RANGE) {
			LOG_WRN("Soft ADC Out-of-Range: ADC input voltage outside normal range");
		}
	}

	*result = raw_result & ADT7604_TEMP_DATA_MASK;

	/* Sign extend if negative */
	if (*result & BIT(ADT7604_DATA_SIGN_BIT)) {
		*result = sign_extend((uint32_t)(*result), ADT7604_DATA_SIGN_BIT);
	}

	return 0;
}

static void adt7604_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct adt7604_data *data = CONTAINER_OF(cb, struct adt7604_data, int_cb);

	k_sem_give(&data->gpio_sem);
}

int adt7604_start_conversion(const struct device *dev, uint8_t chan)
{
	uint8_t status_val;

	if (chan < 1 || chan > ADT7604_MAX_CHANNELS) {
		return -EINVAL;
	}

	status_val = ADT7604_STATUS_START | (chan & ADT7604_STATUS_CHAN_SEL_MASK);

	return adt7604_bus_write(dev, ADT7604_REG_STATUS, &status_val, 1);
}

static int adt7604_read_status(const struct device *dev)
{
	uint8_t status;
	int ret;

	ret = adt7604_bus_read(dev, ADT7604_REG_STATUS, &status, 1);
	if (ret < 0) {
		return ret;
	}

	return (int)status;
}

int adt7604_wait_status_done(const struct device *dev)
{
	const struct adt7604_config *cfg = dev->config;
	struct adt7604_data *data = dev->data;

	if (cfg->int_gpio.port) {
		if (k_sem_take(&data->gpio_sem, K_MSEC(200)) < 0) {
			return -ETIMEDOUT;
		}
	} else {
		if (WAIT_FOR((adt7604_read_status(dev) & ADT7604_STATUS_DONE_MASK) == 0x40,
			     MSEC(200), k_msleep(10)) == false) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int adt7604_reset(const struct device *dev)
{
	const struct adt7604_config *cfg = dev->config;
	int ret;

	if (cfg->reset_gpio.port) {
		ret = gpio_pin_set_dt(&cfg->reset_gpio, 1);
		if (ret < 0) {
			return ret;
		}

		k_busy_wait(100);

		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}

		/* After RESET deassertion, device enters start-up state
		 * Phase 1: Analog circuits power up (max 100ms), status register inaccessible
		 * Wait for phase 1 to complete
		 */
		k_msleep(100);
	}

	return 0;
}

int adt7604_read_resistance_result(const struct device *dev, uint8_t chan, int32_t *result)
{
	uint32_t raw_result;
	int ret;

	if (chan < 1 || chan > ADT7604_MAX_CHANNELS) {
		return -EINVAL;
	}

	ret = adt7604_read_raw_result(dev, ADT7604_RES_RES_ADDR(chan), &raw_result);
	if (ret < 0) {
		return ret;
	}

	*result = raw_result & ADT7604_RES_DATA_MASK;

	return 0;
}

int adt7604_read_temperature_result(const struct device *dev, uint8_t chan, int32_t *result)
{
	uint32_t raw_result;
	int ret;

	if (chan < 1 || chan > ADT7604_MAX_CHANNELS) {
		return -EINVAL;
	}

	ret = adt7604_read_raw_result(dev, ADT7604_TEMP_RES_ADDR(chan), &raw_result);
	if (ret < 0) {
		return ret;
	}

	return adt7604_validate_temperature_result(raw_result, result);
}

/**
 * @brief Configure RTD sensor channel
 */
static void configure_rtd_channel(uint32_t *chan_config,
				  const struct adt7604_channel_config *ch_cfg)
{
	*chan_config |= FIELD_PREP(ADT7604_CHAN_ASSIGN_MASK, ch_cfg->rsense_chan);
	*chan_config |= FIELD_PREP(ADT7604_RTD_SENSOR_CONFIG_MASK, ch_cfg->sensor_config);
	*chan_config |= FIELD_PREP(ADT7604_EXCITATION_CURRENT_MASK, ch_cfg->excitation_current);
	*chan_config |= FIELD_PREP(ADT7604_RTD_CURVE_TYPE_MASK, ch_cfg->rtd_curve);
}

/**
 * @brief Configure copper trace sensor channel
 */
static void configure_copper_trace_channel(uint32_t *chan_config,
					   const struct adt7604_channel_config *ch_cfg)
{
	*chan_config |= FIELD_PREP(ADT7604_CHAN_ASSIGN_MASK, ch_cfg->rsense_chan);
	*chan_config |= FIELD_PREP(ADT7604_COPPER_TRACE_SENSOR_CONFIG_MASK,
				   ADT7604_COPPER_TRACE_SENSOR_CONFIG);
	if (!ch_cfg->sub_ohm) {
		*chan_config |=
			FIELD_PREP(ADT7604_CT_EXCITATION_CURRENT_MASK, ch_cfg->excitation_current);
	}
}

/**
 * @brief Configure thermistor sensor channel
 */
static void configure_thermistor_channel(uint32_t *chan_config,
					 const struct adt7604_channel_config *ch_cfg)
{
	*chan_config |= FIELD_PREP(ADT7604_CHAN_ASSIGN_MASK, ch_cfg->rsense_chan);
	*chan_config |= FIELD_PREP(ADT7604_THERMISTOR_SENSOR_CONFIG_MASK, ch_cfg->sensor_config);
	*chan_config |=
		FIELD_PREP(ADT7604_THERMISTOR_EXCITATION_CURRENT_MASK, ch_cfg->excitation_current);
}

/**
 * @brief Configure leak detector sensor channel
 */
static void configure_leak_detector_channel(uint32_t *chan_config,
					    const struct adt7604_channel_config *ch_cfg)
{
	*chan_config |= FIELD_PREP(ADT7604_CHAN_ASSIGN_MASK, ch_cfg->rsense_chan);
	*chan_config |= FIELD_PREP(ADT7604_LEAK_SENSOR_CONFIG_MASK, ADT7604_LEAK_SENSOR_CONFIG);
	*chan_config |=
		FIELD_PREP(ADT7604_LEAK_EXCITATION_CURRENT_MASK, ch_cfg->excitation_current);
}

/**
 * @brief Configure RSENSE reference resistor channel
 */
static void configure_rsense_channel(uint32_t *chan_config,
				     const struct adt7604_channel_config *ch_cfg)
{
	if (ch_cfg->rsense_mohm) {
		*chan_config |=
			FIELD_PREP(ADT7604_RSENSE_VALUE_INT_MASK, ch_cfg->rsense_int * 1000);
	} else {
		*chan_config |= FIELD_PREP(ADT7604_RSENSE_VALUE_INT_MASK, ch_cfg->rsense_int);
		*chan_config |= FIELD_PREP(ADT7604_RSENSE_VALUE_FRAC_MASK,
					   (ch_cfg->rsense_frac * 1024) / 1000);
	}
}

static int adt7604_configure_channel(const struct device *dev,
				     const struct adt7604_channel_config *ch_cfg)
{
	uint32_t chan_config = 0;
	uint8_t buf[4];
	int ret;

	/* Sensor type */
	chan_config = FIELD_PREP(ADT7604_CHAN_TYPE_MASK, ch_cfg->sensor_type);

	switch (ch_cfg->sensor_type) {
	case ADT7604_SENSOR_TYPE_RTD_PT_10:
	case ADT7604_SENSOR_TYPE_RTD_PT_50:
	case ADT7604_SENSOR_TYPE_RTD_PT_100:
	case ADT7604_SENSOR_TYPE_RTD_PT_200:
	case ADT7604_SENSOR_TYPE_RTD_PT_500:
	case ADT7604_SENSOR_TYPE_RTD_PT_1000:
	case ADT7604_SENSOR_TYPE_RTD_1000:
	case ADT7604_SENSOR_TYPE_RTD_NI_120:
		configure_rtd_channel(&chan_config, ch_cfg);
		break;
	case ADT7604_SENSOR_TYPE_COPPER_TRACE:
		configure_copper_trace_channel(&chan_config, ch_cfg);
		break;
	case ADT7604_SENSOR_TYPE_THERMISTOR_44004:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44005:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44007:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44006:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44008:
	case ADT7604_SENSOR_TYPE_THERMISTOR_YSI_400:
	case ADT7604_SENSOR_TYPE_THERMISTOR_SPECTRUM:
	case ADT7604_SENSOR_TYPE_THERMISTOR_CUSTOM:
		configure_thermistor_channel(&chan_config, ch_cfg);
		break;
	case ADT7604_SENSOR_TYPE_LEAK_DETECTOR:
		configure_leak_detector_channel(&chan_config, ch_cfg);
		break;
	case ADT7604_SENSOR_TYPE_RSENSE:
		configure_rsense_channel(&chan_config, ch_cfg);
		break;
	}

	LOG_DBG("Chan %u config: %08X", ch_cfg->chan_num, chan_config);

	sys_put_be32(chan_config, buf);

	ret = adt7604_bus_write(dev, ADT7604_CHAN_START_ADDR(ch_cfg->chan_num), buf, 4);
	if (ret < 0) {
		LOG_ERR("Failed to configure channel %d", ch_cfg->chan_num);
		return ret;
	}

	LOG_DBG("Configured channel %d, type %d", ch_cfg->chan_num, ch_cfg->sensor_type);

	return 0;
}

static const struct adt7604_channel_config *find_channel(const struct adt7604_config *cfg,
							 uint8_t chan_num)
{
	for (int i = 0; i < cfg->num_channels; i++) {
		if (cfg->channels[i].chan_num == chan_num) {
			return &cfg->channels[i];
		}
	}
	return NULL;
}

/**
 * @brief Update cached temperature unit setting from global config register
 *
 * @param data Pointer to driver data structure
 * @param global_cfg Global configuration register value
 */
static void update_temperature_unit(struct adt7604_data *data, uint8_t global_cfg)
{
	/* Bit 3: 0 = °C, 1 = °F */
	data->temp_unit_celsius = !(global_cfg & ADT7604_TEMP_UNIT_FAHRENHEIT);
}

int adt7604_write_custom_sensor_data_table(const struct device *dev, uint8_t chan_num,
					   const struct adt7604_custom_sensor_data_table *table)
{
	const struct adt7604_config *cfg = dev->config;
	const struct adt7604_channel_config *ch_cfg = find_channel(cfg, chan_num);
	uint8_t buf[ADT7604_MAX_CUSTOM_TABLE_SIZE];
	uint16_t table_addr;
	size_t entry_size, data_len;
	uint8_t custom_addr_field, custom_len_field;
	uint32_t chan_config;
	int ret;

	if (ch_cfg == NULL) {
		return -ENODEV;
	}

	/* Determine entry size and pack data based on sensor type */
	switch (ch_cfg->sensor_type) {
	case ADT7604_SENSOR_TYPE_COPPER_TRACE:
		if (ch_cfg->sub_ohm) {
			return -ENOTSUP; /* Sub-ohm doesn't use custom table */
		}
		entry_size = 6; /* 3 bytes resistance mΩ + 3 bytes temp K */
		data_len = table->num_rows * entry_size;

		/* Pack resistance (mΩ) and temperature (K) as 24-bit big-endian */
		for (uint8_t i = 0; i < table->num_rows; i++) {
			sys_put_be24(table->data[i].copper_trace.resistance_mohm, &buf[i * 6]);
			sys_put_be24(table->data[i].copper_trace.temp_kelvin, &buf[i * 6 + 3]);
		}
		/* Custom length = number of pairs - 1 */
		custom_len_field = table->num_rows - 1;
		break;

	case ADT7604_SENSOR_TYPE_THERMISTOR_CUSTOM:
		entry_size = 4; /* 4 bytes per Steinhart-Hart coefficient */
		data_len = table->num_rows * entry_size;

		/* Pack coefficients as 32-bit IEEE 754 floats (big-endian) */
		for (uint8_t i = 0; i < table->num_rows; i++) {
			uint32_t float_bits;

			memcpy(&float_bits, &table->data[i].thermistor.coeff, sizeof(float));
			sys_put_be32(float_bits, &buf[i * 4]);
		}
		/* For Steinhart-Hart: length field is 0 (6 coefficients is fixed) */
		custom_len_field = 0;
		break;

	case ADT7604_SENSOR_TYPE_LEAK_DETECTOR:
		entry_size = 6;
		data_len = table->num_rows * entry_size;

		for (uint8_t i = 0; i < table->num_rows; i++) {
			sys_put_be24(table->data[i].leak.resistance_ohm, &buf[i * 6]);
			sys_put_be24(table->data[i].leak.coverage, &buf[i * 6 + 3]);
		}
		custom_len_field = table->num_rows - 1;
		break;

	default:
		return -ENOTSUP;
	}

	/* User provides the byte offset (custom_addr_ptr) */
	table_addr = ADT7604_REG_CUST_SENS_TBL_START + table->custom_addr_ptr;

	/* Validate address range */
	if (table_addr + data_len - 1 > ADT7604_REG_CUST_SENS_TBL_END) {
		LOG_ERR("Custom data exceeds table bounds (addr=0x%04X, len=%zu)", table_addr,
			data_len);
		return -EINVAL;
	}

	/* Calculate address field: byte offset / entry_size */
	custom_addr_field = table->custom_addr_ptr / entry_size;

	LOG_DBG("Chan %u: Writing %zu bytes to 0x%04X, addr_field=%u, len_field=%u", chan_num,
		data_len, table_addr, custom_addr_field, custom_len_field);

	/* Write custom data to table memory */
	ret = adt7604_bus_write(dev, table_addr, buf, data_len);
	if (ret < 0) {
		LOG_ERR("Failed to write custom sensor data");
		return ret;
	}

	/* Build channel configuration with custom fields */
	chan_config = FIELD_PREP(ADT7604_CHAN_TYPE_MASK, ch_cfg->sensor_type);
	chan_config |= FIELD_PREP(ADT7604_CHAN_ASSIGN_MASK, ch_cfg->rsense_chan);

	/* Set sensor configuration based on type */
	if (ch_cfg->sensor_type == ADT7604_SENSOR_TYPE_COPPER_TRACE) {
		chan_config |= FIELD_PREP(ADT7604_COPPER_TRACE_SENSOR_CONFIG_MASK,
					  ADT7604_COPPER_TRACE_SENSOR_CONFIG);
		chan_config |=
			FIELD_PREP(ADT7604_CT_EXCITATION_CURRENT_MASK, ch_cfg->excitation_current);
	} else if (ch_cfg->sensor_type == ADT7604_SENSOR_TYPE_LEAK_DETECTOR) {
		chan_config |=
			FIELD_PREP(ADT7604_LEAK_SENSOR_CONFIG_MASK, ADT7604_LEAK_SENSOR_CONFIG);
		chan_config |= FIELD_PREP(ADT7604_LEAK_EXCITATION_CURRENT_MASK,
					  ch_cfg->excitation_current);
	} else if (ch_cfg->sensor_type == ADT7604_SENSOR_TYPE_THERMISTOR_CUSTOM) {
		chan_config |=
			FIELD_PREP(ADT7604_THERMISTOR_SENSOR_CONFIG_MASK, ch_cfg->sensor_config);
		chan_config |= FIELD_PREP(ADT7604_THERMISTOR_EXCITATION_CURRENT_MASK,
					  ch_cfg->excitation_current);
	}

	/* Set custom data address and length */
	chan_config |= FIELD_PREP(ADT7604_CUSTOM_DATA_OFFSET_MASK, custom_addr_field);
	chan_config |= FIELD_PREP(ADT7604_CUSTOM_DATA_LENGTH_MASK, custom_len_field);

	uint8_t chan_buf[4];

	sys_put_be32(chan_config, chan_buf);

	ret = adt7604_bus_write(dev, ADT7604_CHAN_START_ADDR(chan_num), chan_buf, 4);
	if (ret < 0) {
		LOG_ERR("Failed to update channel %u configuration", chan_num);
		return ret;
	}

	LOG_DBG("Channel %u custom data written: %u entries at byte offset %u", chan_num,
		table->num_rows, table->custom_addr_ptr);

	return 0;
}

static int adt7604_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	struct adt7604_data *data = dev->data;
	uint8_t reg_val;
	int ret;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_adt7604)attr) {
	case SENSOR_ATTR_ADT7604_TEMP_UNIT:
		/* Set temperature unit: 0 = °C, 1 = °F */
		ret = adt7604_bus_read(dev, ADT7604_REG_GLOBAL_CONFIG, &reg_val, 1);
		if (ret != 0) {
			LOG_ERR("Failed to read global config");
			return ret;
		}

		/* Clear bit 3 and set new value */
		if (val->val1 != 0) {
			reg_val |= ADT7604_TEMP_UNIT_FAHRENHEIT;
		} else {
			reg_val &= ~ADT7604_TEMP_UNIT_FAHRENHEIT;
		}

		ret = adt7604_bus_write(dev, ADT7604_REG_GLOBAL_CONFIG, &reg_val, 1);
		if (ret != 0) {
			LOG_ERR("Failed to write temperature unit");
			return ret;
		}

		/* Update cached temperature unit */
		update_temperature_unit(data, reg_val);
		LOG_INF("Temperature unit set to %s", data->temp_unit_celsius ? "°C" : "°F");
		break;

	case SENSOR_ATTR_ADT7604_FILTER_NOTCH:
		/* Set digital filter notch frequency: 0 = 50/60Hz, 1 = 60Hz, 2 = 50Hz */
		if (val->val1 > 2) {
			LOG_ERR("Invalid notch frequency value: %d", val->val1);
			return -EINVAL;
		}

		ret = adt7604_bus_read(dev, ADT7604_REG_GLOBAL_CONFIG, &reg_val, 1);
		if (ret != 0) {
			LOG_ERR("Failed to read global config");
			return ret;
		}

		/* Clear bits 1:0 and set new notch frequency */
		reg_val &= ~ADT7604_NOTCH_FREQ_MASK;
		reg_val |= FIELD_PREP(ADT7604_NOTCH_FREQ_MASK, val->val1);

		ret = adt7604_bus_write(dev, ADT7604_REG_GLOBAL_CONFIG, &reg_val, 1);
		if (ret != 0) {
			LOG_ERR("Failed to write filter notch");
			return ret;
		}

		LOG_INF("Filter notch set to: %s",
			val->val1 == 0 ? "50/60Hz" : (val->val1 == 1 ? "60Hz" : "50Hz"));
		break;

	case SENSOR_ATTR_ADT7604_MUX_DELAY_US:
		/* Set MUX delay: value in microseconds (0-25500, multiple of 100µs) */
		if (val->val1 < 0 || val->val1 > 25500) {
			LOG_ERR("Invalid MUX delay: %d µs (valid range: 0-25500)", val->val1);
			return -EINVAL;
		}

		/* Convert µs to register value (value × 100µs) */
		reg_val = val->val1 / 100;

		ret = adt7604_bus_write(dev, ADT7604_REG_MUX_CONFIG, &reg_val, 1);
		if (ret != 0) {
			LOG_ERR("Failed to write MUX delay");
			return ret;
		}

		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int adt7604_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	struct adt7604_data *data = dev->data;
	uint8_t reg_val;
	int ret;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_adt7604)attr) {
	case SENSOR_ATTR_ADT7604_TEMP_UNIT:
		/* Return cached temperature unit: 0 = °C, 1 = °F */
		val->val1 = data->temp_unit_celsius ? 0 : 1;
		val->val2 = 0;
		ret = 0;
		break;

	case SENSOR_ATTR_ADT7604_FILTER_NOTCH:
		/* Read current filter notch frequency setting */
		ret = adt7604_bus_read(dev, ADT7604_REG_GLOBAL_CONFIG, &reg_val, 1);
		if (ret == 0) {
			val->val1 = FIELD_GET(ADT7604_NOTCH_FREQ_MASK, reg_val);
			val->val2 = 0;
		}
		break;

	case SENSOR_ATTR_ADT7604_MUX_DELAY_US:
		/* Read current MUX delay in microseconds */
		ret = adt7604_bus_read(dev, ADT7604_REG_MUX_CONFIG, &reg_val, 1);
		if (ret == 0) {
			val->val1 = reg_val * 100; /* Convert to microseconds */
			val->val2 = 0;
		}
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static DEVICE_API(sensor, adt7604_driver_api) = {
	.attr_set = adt7604_attr_set,
	.attr_get = adt7604_attr_get,
	.submit = adt7604_submit,
	.get_decoder = adt7604_get_decoder,
};

static int adt7604_init(const struct device *dev)
{
	const struct adt7604_config *cfg = dev->config;
	struct adt7604_data *data = dev->data;
	uint8_t global_cfg;
	int ret;

	if (cfg->reset_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->reset_gpio.port);
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO");
			return ret;
		}
	}

	if (cfg->int_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->int_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->int_gpio.port);
		}

		ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure interrupt GPIO");
			return ret;
		}

		gpio_init_callback(&data->int_cb, adt7604_gpio_callback, BIT(cfg->int_gpio.pin));
		ret = gpio_add_callback(cfg->int_gpio.port, &data->int_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add interrupt callback");
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure interrupt GPIO");
			return ret;
		}
	}

	k_sem_init(&data->gpio_sem, 0, 1);

	ret = adt7604_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset ADT7604");
		return ret;
	}

	ret = adt7604_wait_status_done(dev);
	if (ret < 0) {
		LOG_ERR("Timeout waiting for status to be ready");
		return ret;
	}

	global_cfg = FIELD_PREP(ADT7604_NOTCH_FREQ_MASK, cfg->filter_notch);
	ret = adt7604_bus_write(dev, ADT7604_REG_GLOBAL_CONFIG, &global_cfg, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write global config");
		return ret;
	}

	/* Store temperature unit setting */
	update_temperature_unit(data, global_cfg);

	if (cfg->mux_delay_us > 0) {
		uint8_t mux_cfg = cfg->mux_delay_us / 100;

		ret = adt7604_bus_write(dev, ADT7604_REG_MUX_CONFIG, &mux_cfg, 1);
		if (ret < 0) {
			LOG_ERR("Failed to write MUX config");
			return ret;
		}
	}

	for (int i = 0; i < cfg->num_channels; i++) {
		ret = adt7604_configure_channel(dev, &cfg->channels[i]);
		if (ret < 0) {
			return ret;
		}
	}

	LOG_INF("ADT7604 initialized with %d channels", cfg->num_channels);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Power Management Support
 *
 * The ADT7604 supports sleep mode for low-power operation. To enter sleep mode:
 * 1. Write 0x97 to memory location 0x000
 * 2. On the rising edge of CS, the device enters sleep mode
 *
 * The device can be woken up by:
 * 1. Bringing CS low (happens on any SPI transaction)
 * 2. Asserting RESET
 *
 * IMPORTANT: After waking from sleep, the device enters the start-up state and
 * loses its configuration (channel assignments, filters, etc.). Therefore, the
 * RESUME action must reinitialize the device by calling adt7604_init().
 *
 * PM Actions mapping:
 * - SUSPEND: Enter sleep mode
 * - RESUME: Wake from sleep + full reconfiguration
 * - TURN_OFF/TURN_ON: Not supported (no separate deep power-down state)
 */

/**
 * @brief Enter sleep mode
 *
 * @param dev Pointer to the device structure
 * @return 0 on success, negative error code otherwise
 */
static int adt7604_enter_sleep(const struct device *dev)
{
	uint8_t sleep_cmd = ADT7604_SLEEP_MODE_CMD;
	int ret;

	/* Write 0x97 to memory location 0x000
	 * The device enters sleep mode on the rising edge of CS after this write
	 */
	ret = adt7604_bus_write(dev, ADT7604_REG_STATUS, &sleep_cmd, 1);
	if (ret < 0) {
		LOG_ERR("Failed to enter sleep mode");
		return ret;
	}

	/* CS rising edge happens automatically when the SPI transaction completes */
	/* The device is now in sleep mode */

	return 0;
}

/**
 * @brief Exit sleep mode
 *
 * @param dev Pointer to the device structure
 * @return 0 on success, negative error code otherwise
 */
static int adt7604_exit_sleep(const struct device *dev)
{
	const struct adt7604_config *cfg = dev->config;
	int ret;

	/* Two ways to wake up from sleep:
	 * 1. Bring CS low (happens on next SPI transaction)
	 * 2. Assert RESET
	 *
	 * We'll use RESET if available. adt7604_reset() already includes
	 * the 100ms wait for analog circuits to power up.
	 */
	if (cfg->reset_gpio.port) {
		ret = adt7604_reset(dev);
		if (ret < 0) {
			LOG_ERR("Failed to reset device");
			return ret;
		}
	} else {
		/* If no RESET pin, device wakes on next CS low (SPI transaction)
		 * Must wait 100ms for analog circuits before accessing status register
		 */
		k_msleep(100);
	}

	ret = adt7604_wait_status_done(dev);
	if (ret < 0) {
		LOG_ERR("Timeout waiting for device ready after wake-up");
		return ret;
	}

	return 0;
}

static int adt7604_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Enter sleep mode */
		return adt7604_enter_sleep(dev);

	case PM_DEVICE_ACTION_RESUME:
		/* Wake from sleep and reconfigure
		 * Device goes through start-up cycle after sleep, losing configuration
		 */
		ret = adt7604_exit_sleep(dev);
		if (ret != 0) {
			return ret;
		}
		/* Reinitialize device configuration (channels, filters, etc.) */
		return adt7604_init(dev);

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		/* ADT7604 only has one low-power state (sleep mode)
		 * Use SUSPEND/RESUME for runtime power management
		 */
		return -ENOTSUP;

	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define RSENSE_OHM_INT(value)  ((uint32_t)((value) * 1000) / 1000)
#define RSENSE_OHM_FRAC(value) ((uint32_t)((value) * 1000) % 1000)

#define ADT7604_RSENSE_OHM(node_id)                                                                \
	.rsense_int = RSENSE_OHM_INT(DT_STRING_UNQUOTED(node_id, adi_rsense_val)),                 \
	.rsense_frac = RSENSE_OHM_FRAC(DT_STRING_UNQUOTED(node_id, adi_rsense_val)),

#define ADT7604_RTD_CURVE(node_id)                                                                 \
	CONCAT(ADT7604_RTD_CURVE_, DT_STRING_UPPER_TOKEN(node_id, adi_rtd_curve))

#define ADT7604_CHANNEL_CFG(node_id)                                                               \
	{                                                                                          \
		.chan_num = DT_REG_ADDR(node_id),                                                  \
		.sensor_type = DT_PROP(node_id, adi_sensor_type),                                  \
		.rsense_chan = DT_PROP(node_id, adi_rsense_channel),                               \
		ADT7604_RSENSE_OHM(node_id).rsense_mohm = DT_PROP(node_id, adi_rsense_mohm),       \
		.sub_ohm = DT_PROP(node_id, adi_sub_ohm),                                          \
		.sensor_config = DT_PROP(node_id, adi_sensor_configuration),                       \
		.excitation_current = DT_PROP(node_id, adi_excitation_current),                    \
		.rtd_curve = ADT7604_RTD_CURVE(node_id),                                           \
	},

#define ADT7604_DEFINE(inst)                                                                       \
	static const struct adt7604_channel_config adt7604_channels_##inst[] = {                   \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, ADT7604_CHANNEL_CFG)};                     \
                                                                                                   \
	static const struct adt7604_config adt7604_config_##inst = {                               \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		.mux_delay_us = DT_INST_PROP(inst, adi_mux_delay_config_us),                       \
		.filter_notch = DT_INST_ENUM_IDX(inst, adi_filter_notch_freq),                     \
		.num_channels = ARRAY_SIZE(adt7604_channels_##inst),                               \
		.channels = adt7604_channels_##inst,                                               \
	};                                                                                         \
                                                                                                   \
	SPI_DT_IODEV_DEFINE(adt7604_iodev_##inst, DT_DRV_INST(inst),                               \
			    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB);              \
	RTIO_DEFINE(adt7604_rtio_##inst, 16, 16);                                                  \
	static int32_t sensor_data_##inst[ARRAY_SIZE(adt7604_channels_##inst)];                    \
	static struct adt7604_data adt7604_data_##inst = {                                         \
		.iodev = &adt7604_iodev_##inst,                                                    \
		.ctx = &adt7604_rtio_##inst,                                                       \
		.sensor_data = sensor_data_##inst,                                                 \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, adt7604_pm_action);                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adt7604_init, PM_DEVICE_DT_INST_GET(inst),              \
				     &adt7604_data_##inst, &adt7604_config_##inst, POST_KERNEL,    \
				     CONFIG_SENSOR_INIT_PRIORITY, &adt7604_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADT7604_DEFINE)
