/*
 * Copyright (c) 2020 SER Consulting, LLC
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * Note, adapted from many drivers, Trigger functionality not tested yet.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <drivers/sensor.h>
#include <kernel.h>
#include <drivers/i2c.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "acs71020.h"

#define DT_DRV_COMPAT allegro_acs71020

#define ACS71020_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_REGISTER(ACS71020, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ACS71020 driver enabled without any devices"
#endif

static inline struct acs71020_data *to_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct acs71020_config *to_config(const struct device *dev)
{
	return dev->config;
}

static int acs71020_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	struct acs71020_data *data = dev->data;
	const struct acs71020_config *cfg = dev->config;

	return i2c_burst_read(data->i2c_master, cfg->i2c_addr,
			      start, buf, size);
}

/*
   static int acs71020_reg_write(const struct device *dev, uint8_t start, uint8_t *buf, int size)
   {
    struct acs71020_data *data = dev->data;
    const struct acs71020_config *cfg = dev->config;
    return i2c_burst_write(data->i2c_master, cfg->i2c_addr,
                  start, buf, size);
   }
 */
/*
   static int acs71020_attr_set(struct *dev, enum sensor_channel,  enum sensor_attribute,  const struct *sensor_value)
   {
    struct acs71020_data *data = dev->data;
    uint8_t buf[20];
    int size = 20;
    int ret;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

    ret = acs71020_reg_read(dev, ACS71020_REG_I_ADJUST, buf, size);
    if (ret < 0) {
        return ret;
    }
    data->qvo_fine =  ((buf[0]<<8) | (buf[1])) & 0x01ff;
    data->sns_fine =  ((buf[1]<<8) | (buf[1])) & 0x07fff;

    return 0;
   }
 */
static int acs71020_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct acs71020_data *data = dev->data;
	int size = 56;
	uint8_t buf[size];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
/*
    memset(buf, 0, size);
    buf[0] = 0x6E;
    buf[1] = 0x65;
    buf[2] = 0x70;
    buf[3] = 0x4F;
    ret = acs71020_reg_write(dev, 0x2f, buf, 4);
    if (ret < 0) {
        LOG_DBG("acs71020: i2c write error I2C %d", ret);
    } else {
        LOG_DBG("Wrote %02x-%02x%02x%02x%02x", 0x2f, buf[3], buf[2], buf[1], buf[0]);
    }

    memset(buf, 0, size);
    ret = acs71020_reg_read(dev, 0x30, buf, 4);
    LOG_DBG("Read %02x-%02x%02x%02x%02x", 0x30, buf[3], buf[2], buf[1], buf[0]);

    memset(buf, 0, size);
    buf[0] = 0x98;
    buf[1] = 0x03;
    ret = acs71020_reg_write(dev, 0x0f, buf, 4);
    if (ret < 0) {
        LOG_DBG("acs71020: i2c write error I2C %d", ret);
    } else {
        LOG_DBG("Wrote %02x-%02x%02x%02x%02x", 0x0f, buf[3], buf[2], buf[1], buf[0]);
    }

    memset(buf, 0, size);
    buf[0] = 0x06;
    buf[1] = 0x05;
    ret = acs71020_reg_write(dev, 0x0c, buf, 4);
    if (ret < 0) {
        LOG_DBG("acs71020: i2c write error I2C %d", ret);
    } else {
        LOG_DBG("Wrote %02x-%02x%02x%02x%02x", 0x0c, buf[3], buf[2], buf[1], buf[0]);
    }
 */

/*
    memset(buf, 0, size);
    ret = acs71020_reg_read(dev, 0x0b, buf, 20);
    for (int i=0;i<5;i++) {
        LOG_DBG("%02x-%02x%02x%02x%02x", i+0x0b, buf[(i*4)+3], buf[(i*4)+2], buf[(i*4)+1], buf[(i*4)+0]);
    }
    memset(buf, 0, size);
    ret = acs71020_reg_read(dev, 0x1b, buf, 20);
    for (int i=0;i<5;i++) {
        LOG_DBG("%02x-%02x%02x%02x%02x", i+0x1b, buf[(i*4)+3], buf[(i*4)+2], buf[(i*4)+1], buf[(i*4)+0]);
    }
 */
	memset(buf, 0, size);
	ret = acs71020_reg_read(dev, ACS71020_REG_IV, buf, 4);
	if (ret < 0) {
		LOG_DBG("acs71020: i2c error IV %d", ret);
		data->vrms = 0;
		data->irms = 0;
		return ret;
	}
	// LOG_DBG("acs71020 IV= 0.%02x 1.%02x 2.%02x 3.%02x", buf[0], buf[1], buf[2], buf[3]);

	// 0 1 2 3
	data->vrms = ((buf[1] << 8) | (buf[0])) & 0x07fff;
	data->irms = ((buf[3] << 8) | (buf[2])) & 0x07fff;
	// LOG_DBG("acs71020: vrms %x irms %x",data->vrms,data->irms);

	ret = acs71020_reg_read(dev, ACS71020_REG_P_ACT, buf, 4);
	// LOG_DBG("acs71020 PA= 0.%02x 1.%02x 2.%02x 3.%02x", buf[0], buf[1], buf[2], buf[3]);
	if (ret < 0) {
		LOG_DBG("acs71020: i2c error PA %d", ret);
		data->pactive = 0;
		return ret;
	}

	// 4 5 6 7
	data->pactive = ((buf[2] << 16) | (buf[1] << 8) | buf[0]) & 0x01ffff;
	if (data->pactive & 0x010000) {
		data->pactive = data->pactive | 0xffff0000;
	}
	// LOG_DBG("acs71020: pactive %x %d",data->pactive,data->pactive);
	// 8 9 10 11
	// data->papparent = ((buf[10]<<8) | (buf[11])) & 0x07fff;
	// 12 13 14 15
	// data->pimag = ((buf[14]<<8) | (buf[15])) & 0x07fff;
	// 16 17 18 19
	// data->pfactor = ((buf[18]<<8) | (buf[19])) & 0x03ff;
	// 20 21 22 23
	// data->numptsout = ((buf[21]<<8) | (buf[22])) & 0x01f;
	// 24 25 26 27
	// data->vrmsavgonesec = ((buf[26]<<8) | (buf[27])) & 0x07fff;
	// data->irmsavgonesec = ((buf[24]<<8) | (buf[25])) & 0x07fff;
	// 28 29 30 31
	// data->vrmsavgonemin = ((buf[30]<<8) | (buf[31])) & 0x07fff;
	// data->irmsavgonemin = ((buf[28]<<8) | (buf[29])) & 0x07fff;
	// 32 33 34 35
	// data->pactavgonesec = ((buf[33]<<16) | (buf[34]<<8) | buf[35])&0x1ffff;
	// 36 37 38 39
	// data->pactavgonemin = ((buf[37]<<16) | (buf[38]<<8) | buf[39])&0x1ffff;
	// 40 41 42 43
	// data->vcodes = ((buf[41]<<16) | (buf[42]<<8) | buf[43])&0x1ffff;
	// 44 45 46 47
	// data->icodes = ((buf[45]<<16) | (buf[46]<<8) | buf[47])&0x1ffff;
	// 48 49 50 51
	// data->pinstant = ((buf[48]<<24) | (buf[49]<<16) | (buf[50]<<8) | buf[51]);
	// 52 53 54 55
	// data->vzerocrossout = buf[55]&&0x01;
	// data->faultout = buf[55]&&0x02;
	// data->faultlatched = buf[55]&&0x04;
	// data->overvoltage = buf[55]&&0x08;
	// data->undervoltage = buf[55]&&0x10;
	// data->posangle = buf[55]&&0x20;
	// data->pospf = buf[55]&&0x40;

	return 0;
}

static int acs71020_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct acs71020_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		val->val1 = ((data->vrms) * ACS71020_VOLTAGE) / 0x07fff;
		val->val2 = (((data->vrms) * ACS71020_VOLTAGE) % 0x07fff);
		break;
	case SENSOR_CHAN_CURRENT:
		val->val1 = ((data->irms) * ACS71020_CURRENT * 2) / 0x06fff;
		val->val2 = ((data->irms) * ACS71020_CURRENT * 2) % 0x06fff;
		break;
	case SENSOR_CHAN_POWER:
		val->val1 = ((data->pactive) * ACS71020_VOLTAGE * ACS71020_CURRENT * 2) / 0x0ffff;
		val->val2 = ((data->pactive) * ACS71020_VOLTAGE * ACS71020_CURRENT * 2) % 0x0ffff;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api acs71020_api_funcs = {
	.sample_fetch = acs71020_sample_fetch,
	.channel_get = acs71020_channel_get,
    #ifdef CONFIG_ACS71020_TRIGGER
	.attr_set = acs71020_attr_set,
	.trigger_set = acs71020_trigger_set,
#endif /* CONFIG_ACS71020_TRIGGER */
};

int acs71020_init(const struct device *dev)
{
	const char *name = dev->name;
	struct acs71020_data *data = to_data(dev);
	const struct acs71020_config *config = to_config(dev);
	int rc = 0;

	LOG_DBG("initializing %s", name);

	data->i2c_master = device_get_binding(config->i2c_bus);
	if (!data->i2c_master) {
		LOG_DBG("%s: i2c master not found: %s", name, config->i2c_bus);
		return -EINVAL;
	}

	uint8_t buf[4];

	rc = acs71020_reg_read(dev, ACS71020_REG_IV, buf, 4);

#ifdef CONFIG_ACS71020_TRIGGER
	rc = acs71020_setup_interrupt(dev);
#endif /* CONFIG_ACS71020_TRIGGER */

	return rc;
}

static struct acs71020_data acs71020_data;
static const struct acs71020_config acs71020_cfg = {
	.i2c_bus = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
#ifdef CONFIG_ACS71020_TRIGGER
	.alert_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.alert_flags = DT_INST_GPIO_FLAGS(0, int_gpios),
	.alert_controller = DT_INST_GPIO_LABEL(0, int_gpios),
#endif /* CONFIG_ACS71020_TRIGGER */
};

DEVICE_AND_API_INIT(acs71020, DT_INST_LABEL(0), acs71020_init,
		    &acs71020_data, &acs71020_cfg, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &acs71020_api_funcs);
