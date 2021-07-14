/*
 * Copyright (c) 2021 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_st25dv

#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/eeprom.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(eeprom_st25dv);

#define ST25DV_REG_GPO 0x0000
#define ST25DV_REG_IT_TIME 0x0001
#define ST25DV_REG_EH_MODE 0x0002
#define ST25DV_REG_RF_MNGT 0x0003
#define ST25DV_REG_RFA1SS 0x0004
#define ST25DV_REG_ENDA1 0x0005
#define ST25DV_REG_RFA2SS 0x0006
#define ST25DV_REG_ENDA2 0x0007
#define ST25DV_REG_RFA3SS 0x0008
#define ST25DV_REG_ENDA3 0x0009
#define ST25DV_REG_RFA4SS 0x000a
#define ST25DV_REG_I2CSS 0x000b
#define ST25DV_REG_LOCK_CC 0x000c
#define ST25DV_REG_MB_MODE 0x000d
#define ST25DV_REG_MB_WDG 0x000e
#define ST25DV_REG_LOCK_CFG 0x000f
#define ST25DV_REG_LOCK_DSF_ID 0x0010
#define ST25DV_REG_LOCK_AFI 0x0011
#define ST25DV_REG_DSFID 0x0012
#define ST25DV_REG_AFI 0x0013
#define ST25DV_REG_MEM_SIZE 0x0014
#define ST25DV_REG_BLK_SIZE 0x0016
#define ST25DV_REG_IC_REF 0x0017
#define ST25DV_REG_UUID 0x0018
#define ST25DV_REG_IC_REV 0x0020
#define ST25DV_REG_I2C_PWD 0x0900

struct eeprom_st25dv_config {
	uint16_t addr;
	const char *i2c;
};

struct eeprom_st25dv_data {
	const struct device *i2c;
	const struct eeprom_st25dv_config *config;
	uint8_t power_state;
	uint16_t size;
	struct k_mutex lock;
};

static int _st25dv_write_user(struct eeprom_st25dv_data *self, uint16_t index, const uint8_t *pdata,
			      uint32_t count)
{
	int ret = 0;
	uint8_t buffer[count + 2];

	buffer[0] = index >> 8;
	buffer[1] = index;
	memcpy(&buffer[2], pdata, count);

	if ((ret = i2c_write(self->i2c, buffer, count + 2, self->config->addr)) < 0) {
		LOG_ERR("Failed to write user area at 0x%04x", index);
		return -EIO;
	}

	return ret;
}

static int _st25dv_read_user(struct eeprom_st25dv_data *self, uint16_t addr, uint8_t *pdata,
			     uint32_t count)
{
	int ret = 0;
	uint8_t wr_data[2] = { addr >> 8, addr & 0xff };
	if ((ret = i2c_write_read(self->i2c, self->config->addr, wr_data, 2, pdata, count)) < 0) {
		LOG_ERR("Failed to read user at 0x%04x", addr);
		return -EIO;
	}

	return ret;
}

static int _st25dv_read_conf(struct eeprom_st25dv_data *self, uint16_t addr, uint8_t *pdata,
			     uint32_t count)
{
	uint8_t wr_data[2] = { addr >> 8, addr & 0xff };
	int ret = 0;
	if ((ret = i2c_write_read(self->i2c, self->config->addr | 4, wr_data, 2, pdata, count)) <
	    0) {
		LOG_ERR("Failed to read from 0x%02x at 0x%04x", self->config->addr | 4, addr);
		return -EIO;
	}

	return ret;
}

static int eeprom_st25dv_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	struct eeprom_st25dv_data *self = dev->data;

	if (!len) {
		return 0;
	}

	if ((offset + len) > self->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&self->lock, K_FOREVER);
	int ret = _st25dv_read_user(self, offset, buf, len);
	k_mutex_unlock(&self->lock);

	return ret;
}

static int eeprom_st25dv_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	struct eeprom_st25dv_data *self = dev->data;

	if (!len) {
		return 0;
	}

	if ((offset + len) > self->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&self->lock, K_FOREVER);
	int ret = _st25dv_write_user(self, offset, buf, len);
	k_mutex_unlock(&self->lock);

	return ret;
}

static size_t eeprom_st25dv_size(const struct device *dev)
{
	const struct eeprom_st25dv_data *self = dev->data;
	return self->size;
}

static int eeprom_st25dv_init(const struct device *dev)
{
	struct eeprom_st25dv_data *self = dev->data;

	k_mutex_init(&self->lock);

	self->i2c = device_get_binding(self->config->i2c);
	if (!self->i2c) {
		LOG_ERR("Invalid i2c device");
		return -1;
	}

	uint8_t uuid[8] = { 0 };
	uint8_t rev = 0;
	uint8_t size[2] = { 0 };
	if (_st25dv_read_conf(self, ST25DV_REG_UUID, uuid, 8) < 0) {
		return -1;
	}
	if (_st25dv_read_conf(self, ST25DV_REG_IC_REV, &rev, 1) < 0) {
		return -1;
	}
	if (_st25dv_read_conf(self, ST25DV_REG_MEM_SIZE, size, 2) < 0) {
		return -1;
	}

	if (uuid[6] == 0x02 && uuid[7] == 0xe0) {
		LOG_INF("Manufacturer: STMicroelectronics");
	} else {
		return -1;
	}
	LOG_INF("UUID: %02x%02x%02x%02x%02x%02x%02x%02x", uuid[7], uuid[6], uuid[5], uuid[4],
		uuid[3], uuid[2], uuid[1], uuid[0]);
	LOG_INF("Revision 0x%02x", rev);
	LOG_INF("Size 0x%02x%02x", size[1], size[0]);

	self->size = ((uint16_t)size[1]) << 8 | size[0];

	return 0;
}

#if CONFIG_PM
static int eeprom_st25dv_pm(const struct device *dev, uint32_t ctrl_cmd, uint32_t *new_state,
			    pm_device_cb cb, void *arg)
{
	if (!dev || !dev->data)
		return -ENODEV;

	struct eeprom_st25dv_data *self = dev->data;
	// TODO: add power management support for supply-gpio
	if (ctrl_cmd == PM_DEVICE_STATE_SET) {
		uint32_t state = *new_state;
		switch (state) {
		case PM_DEVICE_STATE_ACTIVE:
			break;
		case PM_DEVICE_STATE_LOW_POWER:
			break;
		case PM_DEVICE_STATE_SUSPEND:
			break;
		case PM_DEVICE_STATE_FORCE_SUSPEND:
			break;
		case PM_DEVICE_STATE_OFF:
			break;
		}
		self->power_state = state;
	} else if (ctrl_cmd == PM_DEVICE_STATE_GET) {
		*new_state = self->power_state;
	}
	return 0;
}
#endif

static const struct eeprom_driver_api eeprom_st25dv_api = {
	.read = eeprom_st25dv_read,
	.write = eeprom_st25dv_write,
	.size = eeprom_st25dv_size,
};

#define ST25DV_DEVICE_INIT(index)                                                                  \
	static const struct eeprom_st25dv_config _st25dv_config_##index = {                        \
		.i2c = DT_INST_BUS_LABEL(index),                                                   \
		.addr = DT_INST_REG_ADDR(index),                                                   \
	};                                                                                         \
	static struct eeprom_st25dv_data _st25dv_data_##index = {                                  \
		.config = &_st25dv_config_##index                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, eeprom_st25dv_init, eeprom_st25dv_pm, &_st25dv_data_##index,  \
			      &_st25dv_config_##index, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &eeprom_st25dv_api);

DT_INST_FOREACH_STATUS_OKAY(ST25DV_DEVICE_INIT)
