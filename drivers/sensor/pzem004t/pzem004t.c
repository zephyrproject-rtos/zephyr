/*
 * Copyright (c) 2025 Srishtik Bhandarkar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT peacefair_pzem004t

/*
 * sensor pzem004t.c - Driver for peacefair PZEM004T sensor
 * PZEM004T product: https://en.peacefair.cn/product/772.html
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/drivers/sensor/pzem004t.h>
#include "pzem004t.h"

LOG_MODULE_REGISTER(pzem004t, CONFIG_SENSOR_LOG_LEVEL);

/* Custom function code handler */
#if CONFIG_PZEM004T_ENABLE_RESET_ENERGY

static bool custom_fc_handler(const int iface, const struct modbus_adu *rx_adu,
			      struct modbus_adu *tx_adu, uint8_t *const excep_code,
			      void *const user_data)
{
	/* Validate the received function code */
	if (rx_adu->fc != PZEM004T_RESET_ENERGY_CUSTOM_FC) {
		LOG_ERR("Unexpected function code: 0x%02X", rx_adu->fc);
		*excep_code = MODBUS_EXC_ILLEGAL_FC;
		return true;
	}

	return true;
}

MODBUS_CUSTOM_FC_DEFINE(custom_fc, custom_fc_handler, PZEM004T_RESET_ENERGY_CUSTOM_FC, NULL);

static void register_custom_fc(const int iface)
{
	int err = modbus_register_user_fc(iface, &modbus_cfg_custom_fc);

	if (err) {
		LOG_ERR("Failed to register custom function code (err %d)", err);
	} else {
		LOG_INF("Custom function code 0x42 registered successfully");
	}
}

static int pzem004t_reset_energy(int iface, uint8_t address)
{
	struct modbus_adu adu = {
		.unit_id = address,
		.fc = PZEM004T_RESET_ENERGY_CUSTOM_FC,
		.length = 0,
	};

	int err = modbus_raw_backend_txn(iface, &adu);

	if (err) {
		return err;
	}

	return (adu.fc == PZEM004T_RESET_ENERGY_CUSTOM_FC) ? 0 : -EIO;
}

#endif /* CONFIG_PZEM004T_ENABLE_RESET_ENERGY */

/**
 * @brief Check if the Modbus client is initialized
 *
 * @param iface The Modbus interface index
 * @return true if the Modbus client is initialized, false otherwise
 */
static bool is_modbus_client_initialized(int iface)
{
	struct modbus_adu adu = {0};
	int ret;

	/* Prepare a dummy ADU to test the interface */
	adu.fc = 0x03;
	adu.unit_id = 1;
	adu.length = 0;

	ret = modbus_raw_backend_txn(iface, &adu);

	return (ret == 0);
}

static int pzem004t_init(const struct device *dev)
{
	const struct pzem004t_config *config = dev->config;
	struct pzem004t_data *data = dev->data;
	int iface = modbus_iface_get_by_name(config->modbus_iface_name);

	if (iface < 0) {
		LOG_ERR("Failed to get Modbus interface: %s", config->modbus_iface_name);
		return -ENODEV;
	}

	if (!(is_modbus_client_initialized(iface))) {
		int err = modbus_init_client(iface, config->client_param);

		if (err) {
			LOG_ERR("Modbus RTU client initialization failed (err %d)", err);
			return err;
		}
	}

	data->iface = iface;

#if CONFIG_PZEM004T_ENABLE_RESET_ENERGY
	register_custom_fc(data->iface);
#endif /* CONFIG_PZEM004T_ENABLE_RESET_ENERGY */

	return 0;
}

static int pzem004t_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct pzem004t_data *sensor_data = dev->data;

	uint16_t reg_buf[MEASUREMENT_REGISTER_TOTAL_LENGTH] = {0};
	int err = modbus_read_input_regs(sensor_data->iface, sensor_data->modbus_address,
					 MEASUREMENT_REGISTER_START_ADDRESS, reg_buf,
					 MEASUREMENT_REGISTER_TOTAL_LENGTH);

	if (err != 0) {
		LOG_ERR("Failed to fetch sensor data at address 0x%02x: %d",
				sensor_data->modbus_address, err);
		return err;
	}

	sensor_data->voltage = reg_buf[0];
	sensor_data->current = ((reg_buf[2] << 16) | reg_buf[1]);
	sensor_data->power = ((reg_buf[4] << 16) | reg_buf[3]);
	sensor_data->energy = ((reg_buf[6] << 16) | reg_buf[5]);
	sensor_data->frequency = reg_buf[7];
	sensor_data->power_factor = reg_buf[8];
	sensor_data->alarm_status = reg_buf[9];

	return 0;
}

static int pzem004t_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct pzem004t_data *sensor_data = dev->data;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_VOLTAGE:
		val->val1 = sensor_data->voltage / PZEM004T_VOLTAGE_SCALE;
		val->val2 = (sensor_data->voltage % PZEM004T_VOLTAGE_SCALE);
		break;
	case SENSOR_CHAN_CURRENT:
		val->val1 = sensor_data->current / PZEM004T_CURRENT_SCALE;
		val->val2 = (sensor_data->current % PZEM004T_CURRENT_SCALE);
		break;
	case SENSOR_CHAN_POWER:
		val->val1 = sensor_data->power / PZEM004T_POWER_SCALE;
		val->val2 = (sensor_data->power % PZEM004T_POWER_SCALE);
		break;
	case (enum sensor_channel)SENSOR_CHAN_PZEM004T_ENERGY:
		val->val1 = sensor_data->energy / PZEM004T_ENERGY_SCALE;
		val->val2 = (sensor_data->energy % PZEM004T_ENERGY_SCALE);
		break;
	case SENSOR_CHAN_FREQUENCY:
		val->val1 = sensor_data->frequency / PZEM004T_FREQUENCY_SCALE;
		val->val2 = (sensor_data->frequency % PZEM004T_FREQUENCY_SCALE);
		break;
	case (enum sensor_channel)SENSOR_CHAN_PZEM004T_POWER_FACTOR:
		val->val1 = sensor_data->power_factor / PZEM004T_POWER_FACTOR_SCALE;
		val->val2 = (sensor_data->power_factor % PZEM004T_POWER_FACTOR_SCALE);
		break;
	case (enum sensor_channel)SENSOR_CHAN_PZEM004T_ALARM_STATUS:
		val->val1 = sensor_data->alarm_status;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int pzem004t_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct pzem004t_data *data = dev->data;

	int err;
	uint16_t reg_buf[1] = {0};

	if (chan != (enum sensor_channel)SENSOR_CHAN_PZEM004T_POWER_ALARM_THRESHOLD &&
	    chan != (enum sensor_channel)SENSOR_CHAN_PZEM004T_MODBUS_RTU_ADDRESS) {
		LOG_ERR("Channel not supported for setting Request");
		return -ENOTSUP;
	}

	switch ((uint32_t)attr) {
	case (enum sensor_attribute)SENSOR_ATTR_PZEM004T_POWER_ALARM_THRESHOLD:
		err = modbus_read_holding_regs(data->iface, data->modbus_address,
					       POWER_ALARM_THRESHOLD_ADDRESS, reg_buf,
					       POWER_ALARM_THRESHOLD_REGISTER_LENGTH);
		if (err != 0) {
			return err;
		}
		val->val1 = reg_buf[0];
		val->val2 = 0;
		break;

	case (enum sensor_attribute)SENSOR_ATTR_PZEM004T_MODBUS_RTU_ADDRESS:
		err = modbus_read_holding_regs(data->iface, data->modbus_address,
					       MODBUS_RTU_ADDRESS_REGISTER, reg_buf,
					       MODBUS_RTU_ADDRESS_REGISTER_LENGTH);
		if (err != 0) {
			return err;
		}
		val->val1 = reg_buf[0];
		val->val2 = 0;
		break;

	default:
		LOG_ERR("Unsupported Attribute");
		return -ENOTSUP;
	}

	return 0;
}

static int pzem004t_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct pzem004t_data *data = dev->data;
	int err;

	if (chan != (enum sensor_channel)SENSOR_CHAN_PZEM004T_POWER_ALARM_THRESHOLD &&
	    chan != (enum sensor_channel)SENSOR_CHAN_PZEM004T_MODBUS_RTU_ADDRESS &&
		chan != (enum sensor_channel)SENSOR_CHAN_PZEM004T_ADDRESS_INST_SET &&
	    chan != (enum sensor_channel)SENSOR_CHAN_PZEM004T_RESET_ENERGY) {
		LOG_ERR("Channel not supported for setting attribute");
		return -ENOTSUP;
	}

	switch ((uint32_t)attr) {
	case (enum sensor_attribute)SENSOR_ATTR_PZEM004T_POWER_ALARM_THRESHOLD:
		if (val->val1 < 0 || val->val1 > PZEM004T_MAX_POWER_ALARM_THRESHOLD) {
			LOG_ERR("Power alarm threshold out of range");
			return -EINVAL;
		}

		err = modbus_write_holding_reg(data->iface, data->modbus_address,
					       POWER_ALARM_THRESHOLD_ADDRESS, val->val1);
		if (err != 0) {
			return err;
		}
		break;

	case (enum sensor_attribute)SENSOR_ATTR_PZEM004T_MODBUS_RTU_ADDRESS:
		if (val->val1 < 0 || val->val1 > PZEM004T_MAX_MODBUS_RTU_ADDRESS) {
			LOG_ERR("Address out of range");
			return -EINVAL;
		}

		err = modbus_write_holding_reg(data->iface, data->modbus_address,
					       MODBUS_RTU_ADDRESS_REGISTER, val->val1);

		if (err != 0) {
			return err;
		}
		break;

	case (enum sensor_attribute)SENSOR_ATTR_PZEM004T_ADDRESS_INST_SET:
		if (val->val1 < 0 || val->val1 > PZEM004T_MAX_MODBUS_RTU_ADDRESS) {
			LOG_ERR("Address out of range");
			return -EINVAL;
		}

		data->modbus_address = val->val1;
		break;

#if CONFIG_PZEM004T_ENABLE_RESET_ENERGY
	case (enum sensor_attribute)SENSOR_ATTR_PZEM004T_RESET_ENERGY:
		err = pzem004t_reset_energy(data->iface, data->modbus_address);
		if (err != 0) {
			LOG_ERR("Failed to reset energy");
			return err;
		}
		break;
#else
	case (enum sensor_attribute)SENSOR_ATTR_PZEM004T_RESET_ENERGY:
		LOG_ERR("Reset energy is not enabled by default. Enable "
			"CONFIG_PZEM004T_ENABLE_RESET_ENERGY in prj.conf.");
		return -ENOTSUP;
#endif /* CONFIG_PZEM004T_ENABLE_RESET_ENERGY */

	default:
		LOG_ERR("Unsupported Attribute");
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, pzem004t_api) = {
	.sample_fetch = pzem004t_sample_fetch,
	.channel_get = pzem004t_channel_get,
	.attr_get = pzem004t_attr_get,
	.attr_set = pzem004t_attr_set,
};

#define PZEM004T_DEFINE(inst)                                                              \
static const struct pzem004t_config pzem004t_config_##inst = {                             \
	.modbus_iface_name = DEVICE_DT_NAME(DT_PARENT(DT_INST(inst, peacefair_pzem004t))), \
	.client_param =                                                                    \
		{                                                                          \
			.mode = MODBUS_MODE_RTU,                                           \
			.rx_timeout = 100000,                                              \
			.serial =                                                          \
				{                                                          \
					.baud = 9600,                                      \
					.parity = UART_CFG_PARITY_NONE,                    \
					.stop_bits = UART_CFG_STOP_BITS_1,                 \
				},                                                         \
		},                                                                         \
};                                                                                         \
											   \
static struct pzem004t_data pzem004t_data_##inst = {                                       \
	.modbus_address = PZEM004T_DEFAULT_MODBUS_ADDRESS,                                 \
};                                                                                         \
											   \
SENSOR_DEVICE_DT_INST_DEFINE(inst, &pzem004t_init, NULL, &pzem004t_data_##inst,            \
				 &pzem004t_config_##inst, POST_KERNEL,                     \
				 CONFIG_SENSOR_INIT_PRIORITY, &pzem004t_api);

DT_INST_FOREACH_STATUS_OKAY(PZEM004T_DEFINE)
