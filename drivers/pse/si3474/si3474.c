#define DT_DRV_COMPAT skyworks_si3474

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pse.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "si3474.h"
#include "si3474_setup.h"
#include "si3474_reg.h"
#include "si3474_setup.h"

LOG_MODULE_REGISTER(SI3474, CONFIG_PSE_LOG_LEVEL);

int si3474_i2c_write_reg(const struct device *i2c, uint16_t dev_addr, uint8_t reg_ddr, uint8_t val)
{
	int res = i2c_reg_write_byte(i2c, dev_addr, reg_ddr, val);
	if (res != 0) {
		LOG_WRN("Si3474 device 0x%x writing 0x%x failed! [%i]", dev_addr, reg_ddr, res);
		return res;
	}
	return res;
}

int si3474_i2c_read_reg(const struct device *i2c, uint16_t dev_addr, uint8_t reg_ddr, uint8_t *buff)
{
	int res = 0;
	res = i2c_reg_read_byte(i2c, dev_addr, reg_ddr, buff);
	if (res != 0) {
		LOG_WRN("Si3474 device 0x%x reading 0x%x failed...! [%i]", dev_addr, reg_ddr, res);
		return res;
	}
	return res;
}

static int si3474_get_events(const struct device *dev, uint8_t *events)
{
	const struct si3474_config *cfg = dev->config;
	uint16_t dev_address = cfg->i2c.addr;
	uint8_t pse_interrupt_buffer;
	int res = 0;
	int mask = 0x1;

	res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, INTERRUPT_REGISTER,
				  &pse_interrupt_buffer);
	if (res != 0) {
		LOG_WRN("Si3474 reading interrupt status failed");
	}

	for (int cnt = 0; cnt <= 8; cnt++) {
		switch (pse_interrupt_buffer & mask) {
		case POWER_ENABLE_CHANGE_IT:
			mask <<= 1;
			cnt++;
		case POWER_GOOD_CHANGE_IT:
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, POWER_EVENT_REG,
						  &events[POWER_EVENT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			break;
		case DISCONNECT_IT:
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address,
						  DISCONNECT_PCUT_FAULT_REG,
						  &events[DISCONNECT_PCUT_FAULT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, SUPPLY_EVENT_REG,
						  &events[SUPPLY_EVENT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			break;

		case DETECT_CC_DONE_IT:
			mask <<= 1;
			cnt++;
		case CLASS_DONE_IT:
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, CLASS_DETECT_EVENT_REG,
						  &events[CLASS_DETECT_EVENT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			break;

		case P_I_FAULT_IT:
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address,
						  DISCONNECT_PCUT_FAULT_REG,
						  &events[DISCONNECT_PCUT_FAULT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, ILIM_START_FAULT_REG,
						  &events[ILIM_START_FAULT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, SUPPLY_EVENT_REG,
						  &events[SUPPLY_EVENT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			break;

		case START_EVENT_IT:
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, ILIM_START_FAULT_REG,
						  &events[ILIM_START_FAULT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, POWER_ON_FAULT_REG,
						  &events[POWER_ON_FAULT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			break;

		case SUPPLY_EVENT_IT:
			res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, SUPPLY_EVENT_REG,
						  &events[SUPPLY_EVENT]);
			if (res != 0) {
				LOG_WRN("Si3474 reading event status table failed");
			}
			break;
		}
		mask <<= 1;
	}
	return res;
}

static int si3474_set_events(const struct device *dev, uint8_t events)
{
	const struct si3474_config *cfg = dev->config;
	uint16_t dev_address = cfg->i2c.addr;
	int res;

	res = i2c_reg_write_byte(cfg->i2c.bus, dev_address, INTERRUPT_MASK, events);
	if (res != 0) {
		LOG_WRN("Si3474 setting events failed!");
		return res;
	}
	return res;
}

static int si3474_get_current(const struct device *dev, uint8_t channel, uint16_t *val)
{
	const struct si3474_config *cfg = dev->config;
	uint16_t dev_address = cfg->i2c.addr;
	uint8_t buff[CURRENT_SIZE_BUFFER];
	int res = 0;

	switch (channel) {
	case CHANNEL_0:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT1_CURRENT_LSB_REG, &buff[0],
				     CURRENT_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading current on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;
	case CHANNEL_1:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT2_CURRENT_LSB_REG, &buff[0],
				     CURRENT_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading current on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;

	case CHANNEL_2:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT3_CURRENT_LSB_REG, &buff[0],
				     CURRENT_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading current on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;

	case CHANNEL_3:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT4_CURRENT_LSB_REG, &buff[0],
				     CURRENT_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading current on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;
	}
	return res;
}

static int si3474_get_voltage(const struct device *dev, uint8_t channel, uint16_t *val)
{
	const struct si3474_config *cfg = dev->config;
	uint16_t dev_address = cfg->i2c.addr;
	uint8_t buff[VOLTAGE_SIZE_BUFFER];
	int res = 0;

	switch (channel) {
	case CHANNEL_0:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT1_VOLTAGE_LSB_REG, &buff[0],
				     VOLTAGE_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading voltage on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;
	case CHANNEL_1:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT2_VOLTAGE_LSB_REG, &buff[0],
				     VOLTAGE_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading voltage on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;

	case CHANNEL_2:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT3_VOLTAGE_LSB_REG, &buff[0],
				     VOLTAGE_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading voltage on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;

	case CHANNEL_3:
		res = i2c_burst_read(cfg->i2c.bus, dev_address, PORT4_VOLTAGE_LSB_REG, &buff[0],
				     VOLTAGE_SIZE_BUFFER);
		if (res != 0) {
			LOG_WRN("Si3474 reading voltage on channel %d failed!", channel);
		} else {
			*val = ((uint16_t)buff[1] << 8);
			*val |= (uint16_t)buff[0];
		}
		break;
	}
	return res;
}

int si3474_get_main_voltage(const struct device *dev, uint16_t *val)
{
	const struct si3474_config *cfg = dev->config;
	uint16_t dev_address = cfg->i2c.addr;
	uint8_t buff[VOLTAGE_SIZE_BUFFER];
	int res = 0;

	res = i2c_burst_read(cfg->i2c.bus, dev_address, VPWR_LSB_REG, &buff[0],
			     VOLTAGE_SIZE_BUFFER);
	if (res != 0) {
		LOG_WRN("Si3474 reading main voltage failed!");
	} else {
		*val = ((uint16_t)buff[1] << 8);
		*val |= (uint16_t)buff[0];
	}
	return res;
}

static int si3474_get_temperature(const struct device *dev, uint8_t *buff)
{
	const struct si3474_config *cfg = dev->config;
	uint16_t dev_address = cfg->i2c.addr;
	int res = 0;

	res = si3474_i2c_read_reg(cfg->i2c.bus, dev_address, TEMPERATURE_REG, buff);
	if (res != 0) {
		LOG_WRN("Si3474 reading temperature failed!");
		return res;
	}
	return res;
}

static int si3474_switch_on_channel(const struct device *dev, uint8_t channel)
{
	int res;
	const struct si3474_config *cfg = dev->config;
	int16_t dev_address = cfg->i2c.addr;

	res = i2c_reg_write_byte(cfg->i2c.bus, dev_address, PB_POWER_ENABLE_REG, (1 << channel));
	if (res != 0) {
		LOG_WRN("Si3474 switching channel failed");
		return res;
	}

	return res;
}

static int si3474_switch_off_channel(const struct device *dev, uint8_t channel)
{
	int res;
	const struct si3474_config *cfg = dev->config;
	int16_t dev_address = cfg->i2c.addr;

	res = i2c_reg_write_byte(cfg->i2c.bus, dev_address, PB_POWER_ENABLE_REG,
				 (1 << (channel + 4)));
	if (res != 0) {
		LOG_WRN("Si3474 switching channel failed");
		return res;
	}

	return res;
}

static int si3474_init(const struct device *dev)
{
	const struct si3474_config *cfg = dev->config;
	int res;
#if CONFIG_SI3474_TRIGGER
	res = si3474_init_interrupt(dev);
	if (res != 0) {
		LOG_ERR("Configuring Si3474 interrupt callback failed");
		return -ENODEV;
	}
#endif
	if (device_is_ready(cfg->i2c.bus) == false) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
	res = i2c_configure(cfg->i2c.bus, i2c_cfg);
	if (res != 0) {
		LOG_ERR("Configuring Si3474 speed failed");
		return -ENODEV;
	}

	res = si3474_init_ports(dev);
	if (res != 0) {
		LOG_ERR("Configuring Si3474 interrupt and oss pins failed");
		return -ENODEV;
	}

	return 0;
}

static const struct pse_driver_api si3474_driver_api = {
	.get_current = si3474_get_current,
	.get_voltage = si3474_get_voltage,
	.get_main_voltage = si3474_get_main_voltage,
	.get_temperature = si3474_get_temperature,
	.get_events = si3474_get_events,
	.set_events = si3474_set_events,
	.channel_on = si3474_switch_on_channel,
	.channel_off = si3474_switch_off_channel,
#if CONFIG_SI3474_TRIGGER
	.set_event_trigger = si3474_event_trigger_set,
#endif
};

#define SI3474_DEFINE_CONFIG(index)                                                                \
	static const struct si3474_config si3474_cfg_##index = {                                   \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.gpio_int = GPIO_DT_SPEC_INST_GET(index, int_gpios),                               \
		.gpio_rst = GPIO_DT_SPEC_INST_GET(index, rst_gpios),                               \
		.gpio_oss = GPIO_DT_SPEC_INST_GET(index, oss_gpios),                               \
	}

#define SI3474_INIT(index)                                                                         \
	SI3474_DEFINE_CONFIG(index);                                                               \
	static struct si3474_data si3474_driver_##index;                                           \
	PSE_DEVICE_DT_INST_DEFINE(index, si3474_init, NULL, &si3474_driver_##index,                \
				  &si3474_cfg_##index, POST_KERNEL, CONFIG_PSE_INIT_PRIORITY,      \
				  &si3474_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SI3474_INIT)
