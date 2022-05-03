/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lm75

#include <device.h>
#include <devicetree.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(LM75, CONFIG_SENSOR_LOG_LEVEL);


#define LM75_REG_TEMP   0x00
#define LM75_REG_CONFIG 0x01
#define LM75_REG_T_HYST 0x02
#define LM75_REG_T_OS   0x03

#define ATMEL_AT30TS75A_CONFIG_SHUTDOWN_MODE_BIT 0
#define ATMEL_AT30TS75A_CONFIG_ALARM_THERMO_MODE_BIT 1
#define ATMEL_AT30TS75A_CONFIG_ALERT_PIN_POL_BIT 2
#define ATMEL_AT30TS75A_CONFIG_FAULT_TOL_QUEUE_BITS 3
#define ATMEL_AT30TS75A_CONFIG_RESOLUTION_BITS 5
#define ATMEL_AT30TS75A_CONFIG_ONE_SHOT_MODE_BIT 7

struct lm75_data {
	int16_t temp; /*temp in 0.1°C*/
};

struct lm75_config {
	const struct device *i2c_dev;
	uint8_t i2c_addr;
	uint8_t temp_res;
};

static inline int lm75_reg_read(const struct lm75_config *cfg, uint8_t reg,
				uint8_t *buf, uint32_t size)
{
	return i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr, reg, buf, size);
}

static inline int lm75_reg_write(const struct lm75_config *cfg, uint8_t reg,
				uint8_t *buf, uint32_t size)
{
	return i2c_burst_write(cfg->i2c_dev, cfg->i2c_addr, reg, buf, size);
}

static inline int lm75_fetch_temp(const struct lm75_config *cfg, struct lm75_data *data)
{
	int ret;
	uint8_t temp_read[2];
	int16_t temp;

	ret = lm75_reg_read(cfg, LM75_REG_TEMP, temp_read, sizeof(temp_read));
	if (ret) {
		LOG_ERR("Could not fetch temperature [%d]", ret);
		return -EIO;
	}

	/* temp is in two's complement.
	 *  bit 7 in the lower part corresponds to 0.5
	 */
	temp = temp_read[0] << 8 | temp_read[1];

	/* shift right by number of not used bits
	 * multiply by 1000 and shift right by 1,2,3 or 4 
	 * to store as fixed point int16_t */
	data->temp = (((temp >> (16 - cfg->temp_res)) * 1000) >> (cfg->temp_res - 8));

	return 0;
}

static int lm75_sample_fetch(const struct device *dev,
			     enum sensor_channel chan)
{
	struct lm75_data *data = dev->data;
	const struct lm75_config *cfg = dev->config;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
		return lm75_fetch_temp(cfg, data);
	default:
		return -ENOTSUP;
	}
}

static int lm75_channel_get(const struct device *dev,
			    enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct lm75_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Convert fixed point value to sensor value*/
		val->val1 = data->temp / 1000;
		val->val2 = (data->temp - val->val1 * 1000) * 1000U;
		return 0;
	default:
		return -ENOTSUP;
	}
}

/* Function to set the config register*/
static int lm75_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct lm75_data *drv_data = dev->data;
	const struct lm75_config *cfg = dev->config;
	int ret;
	if(attr == SENSOR_ATTR_CONFIGURATION){
		uint8_t config_buf[3] = {LM75_REG_CONFIG, (uint8_t) val->val1, (uint8_t) val->val2};
		ret = i2c_write(cfg->i2c_dev, config_buf, 
                        2, cfg->i2c_addr);
		if (ret) {
			printf("Could not set attribute\n");
			return -ENOTSUP;
		}
	}

	return 0;
}

uint8_t set_bits(uint8_t *byte, uint8_t bit_num,
                 uint8_t bit_mask_val, uint8_t bit_val)
{
	uint8_t bit_mask = (bit_mask_val << bit_num);

	*byte = (*byte & (~bit_mask)) | (bit_val << bit_num);
}

static int lm75_set_resolution(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr)
{
	const struct lm75_config *cfg = dev->config;
	int ret;

	struct sensor_value config_val = {
		.val1 = 0,
		.val2 = 0
	};

	/* Depending on the temperature resolution of 9, 10, 11 or 12-bit, value is set to 0, 1, 2 or 3 */
	uint8_t res_bit_set = cfg->temp_res - 9;

	set_bits(&config_val.val1, ATMEL_AT30TS75A_CONFIG_RESOLUTION_BITS, 
             res_bit_set, 0b11);

	/* MSB of ATMEL AT30TS75A config register are reserved for future use */
	config_val.val2 = 0x0;  

	/* Additional configurations can be added here */

	ret = lm75_attr_set(dev, chan, attr, &config_val);
	if (ret) {
		printf("sensor_attr_set failed ret %d\n", ret);
	}

	return ret;
}

static const struct sensor_driver_api lm75_driver_api = {
	.sample_fetch = lm75_sample_fetch,
	.channel_get = lm75_channel_get,
	.attr_set = lm75_attr_set,
};

int lm75_init(const struct device *dev)
{
	const struct lm75_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->i2c_dev)) {
		LOG_ERR("I2C dev not ready");
		return -ENODEV;
	}

	if (dev->name == "AT30TS75A")
	{
		ret = lm75_set_resolution(dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_CONFIGURATION);
	}
	if (ret) {
		printf("failed to set resolution %d\n", ret);
	}

	return 0;
}

#define LM75_INST(inst)                                             \
static struct lm75_data lm75_data_##inst;                           \
static const struct lm75_config lm75_config_##inst = {              \
	.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                \
	.i2c_addr = DT_INST_REG_ADDR(inst),                         \
	.temp_res = DT_INST_PROP(inst, temp_res),					\
};                                                                  \
DEVICE_DT_INST_DEFINE(inst, lm75_init, NULL, &lm75_data_##inst,     \
		      &lm75_config_##inst, POST_KERNEL,             \
		      CONFIG_SENSOR_INIT_PRIORITY, &lm75_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LM75_INST)
