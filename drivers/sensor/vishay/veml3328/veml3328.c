/*
 * Senor Driver for Vishay Veml3328
 * RGBCIR Color Sensor with I2C Interface
 *
 * Copyright (c) 2026 Christopher Ruehl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_veml3328

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/sensor/veml3328.h>

LOG_MODULE_REGISTER(VEML3328, CONFIG_SENSOR_LOG_LEVEL);

#define VEML3328_SD_ALS_MASK	BIT(14)
#define VEML3328_DG_MASK	GENMASK(13, 12)
#define VEML3328_GAIN_MASK	GENMASK(11, 12)
#define VEML3328_SENSE_MASK	BIT(6)
#define VEML3328_IT_MASK	GENMASK(5, 4)
#define VEML3328_AF_MASK	BIT(3)
#define VEML3328_TRIG_MASK	BIT(2)

/* Reserved bits always 0 */
#define RESERVED_MASK	(BIT(8)|BIT(7)|BIT(1))

/* Sensing Data: ALL or G,C and IR only */
#define VEML3328_SD_ALL		0x00
#define VEML3328_SD_ALS_ONLY	0x01

/* DG Detection Range parameter 1, 2, 4 */
#define VEML3328_DG_GAIN_1	0x00
#define VEML3328_DG_GAIN_2	0x01
#define VEML3328_DG_GAIN_4	0x02

/* Gain values */
#define VEML3328_GAIN_1_2	0x03
#define VEML3328_GAIN_1		0x00
#define VEML3328_GAIN_2		0x01
#define VEML3328_GAIN_4		0x02

/* Sensetifity */
#define VEML3328_SENSE_DEFAULT	0x00
#define VEML3328_SENSE_1_3	0x01

/* Integration times */
#define VEML3328_IT_50MS	0x00
#define VEML3328_IT_100MS	0x01
#define VEML3328_IT_200MS	0x02
#define VEML3328_IT_400MS	0x03

/* Modes the sensor support */
#define VEML3328_AUTO_MODE	0x00
#define VEML3328_ACTIVEFORCE	0x01

/* Single Measurement */
#define VEML3328_TRIGGER_OFF	0x00
#define VEML3328_TRIGGER_ON	0x01

/* Bit15 and Bit0 must set to 1 for Power Down */
#define VEML3328_PWRDOWN_BITS	(BIT(0) | BIT(15))
#define VEML3328_PWRUP_BITS	~(VEML3328_PWRDOWN_BITS)

#define VEML3328_PWROFF	0x00
#define VEML3328_PWRON	0x01

/* Veml3328 own identification read from LSB  */
#define VEML3328_DEVICE_ID	0x28

/* R/W REGISTER */
#define VEML3328_CMD_REG_CONFIG	0x00
/* RONLY REGISTER */
#define VEML3328_CMD_REG_CLEAR	0x04
#define VEML3328_CMD_REG_RED	0x05
#define VEML3328_CMD_REG_GREEN	0x06
#define VEML3328_CMD_REG_BLUE	0x07
#define VEML3328_CMD_REG_IR	0x08
#define VEML3328_CMD_REG_ID	0x0C

/* BIT MASK for internal use */
#define VEML3328_CLEAR	BIT(0)
#define VEML3328_IR	BIT(1)
#define VEML3328_RED	BIT(2)
#define VEML3328_GREEN	BIT(3)
#define VEML3328_BLUE	BIT(4)
#define VEML3328_LUX	BIT(5)

static uint16_t integration_times_ms[4] = { 50, 100, 200, 400 };

/*
 * VEML3328 CONFIG REGISTER MAP
 * SD1:SD_ALS:DG(2):GAIN(2):R(3):SENS:IT(2):AF:TRIG:R:SD0
 */

struct veml3328_config {
	struct i2c_dt_spec bus;
	union {
		uint16_t config_bits;
		struct {
			uint16_t sd1    : 1;
			uint16_t sd_als : 1;
			uint16_t dg     : 2;
			uint16_t gain   : 2;
			uint16_t res3   : 3;
			uint16_t sens   : 1;
			uint16_t itime  : 2;
			uint16_t af     : 1;
			uint16_t trig   : 1;
			uint16_t res1   : 1;
			uint16_t sd0    : 1;
		};
	};
};

struct veml3328_data {
	uint16_t clear_raw;
	uint16_t ir_raw;
	uint16_t red_raw;
	uint16_t green_raw;
	uint16_t blue_raw;
	uint16_t lux_high;
	uint16_t lux_low;
};

static int veml3328_write(const struct device *dev, uint8_t cmd, uint16_t data)
{
	const struct veml3328_config *conf = dev->config;
	uint8_t send_buf[3];

	send_buf[0] = cmd;
	sys_put_le16(data, &send_buf[1]);

	return i2c_write_dt(&conf->bus, send_buf, ARRAY_SIZE(send_buf));
}

static int veml3328_read(const struct device *dev, uint8_t cmd, uint16_t *data)
{
	int ret;
	const struct veml3328_config *conf = dev->config;
	uint8_t recv_buf[2];

	ret = i2c_write_read_dt(&conf->bus, &cmd, sizeof(cmd), &recv_buf, ARRAY_SIZE(recv_buf));
	if (ret < 0) {
		return ret;
	}

	*data = sys_get_le16(recv_buf);

	return 0;
}

static int veml3328_read_deviceid(const struct device *dev, uint16_t *data)
{
	return veml3328_read(dev, VEML3328_CMD_REG_ID, data);
}

static int veml3328_read_conf(const struct device *dev, uint16_t *data)
{
	return veml3328_read(dev, VEML3328_CMD_REG_CONFIG, data);
}

static int veml3328_write_conf(const struct device *dev)
{
	const struct veml3328_config *conf = dev->config;

	return veml3328_write(dev, VEML3328_CMD_REG_CONFIG, conf->config_bits);
}

static int veml3328_set_pmstate(const struct device *dev, uint8_t val)
{
	struct veml3328_config *conf = (struct veml3328_config *)dev->config;

	/* to enable the power for veml3328 the bit15 and bit0 must cleared */
	if (val > VEML3328_PWROFF) {
		conf->config_bits &= VEML3328_PWRUP_BITS;
	} else {
		conf->config_bits |= VEML3328_PWRDOWN_BITS;
	}

	return veml3328_write_conf(dev);
}

static int veml3328_calculate_lux(const struct device *dev, uint32_t *lux)
{
	const struct veml3328_config *conf = dev->config;
	struct veml3328_data *data = dev->data;
	/* lux_per_cnt based on 50ms interation time taken from Application Note
	 * gain: x1 = index 0
	 * gain: x2 = index 1
	 * gain: x4 = index 2
	 * gain: x1/2 = index 3
	 */
	float lux_per_cnt[4] = { 0.384f, 0.192f, 0.096f, 0.768f };
	/* interation time index, 50 to 400 used to adjust the lux_per_cnt */
	uint8_t itdevider[4] = { 1, 2, 4, 8 };
	/* dg index, x1, x2 and x4 used to adjust the lux_per_cnt further */
	uint8_t dgdevider[3] = { 1, 2, 4 };
	float luxcount;

	luxcount = lux_per_cnt[conf->gain] / itdevider[conf->itime];
	luxcount = luxcount / dgdevider[conf->dg];

	/* Application noted: only makes sense for very hight illumiation
	 * and DG: x1. User space application must aware of this no further
	 * checks here!
	 */
	if (conf->sens == VEML3328_SENSE_1_3) {
		luxcount *= 3;
	}

	/* Per application note: maximum lux = 150993 with sens=1,dg=1,gain=x1_2
	 * which not fit into a uint16_t, return in the uint32_t
	 */
	*lux = luxcount * data->green_raw;

	return 0;
}

static int veml3328_fetch(const struct device *dev, const uint8_t chan_mask)
{
	int ret;
	struct veml3328_data *data = dev->data;

	if ((chan_mask & (VEML3328_GREEN|VEML3328_LUX)) > 0) {
		ret = veml3328_read(dev, VEML3328_CMD_REG_GREEN, &data->green_raw);
		if (ret < 0) {
			return ret;
		}
		if ((chan_mask & VEML3328_LUX) > 0) {
			uint32_t lux;

			veml3328_calculate_lux(dev, &lux);
			data->lux_high = (uint16_t)(lux >> 16);
			data->lux_low = (uint16_t)(lux & 0xffff);
		}
	}
	if ((chan_mask & VEML3328_RED) > 0) {
		ret = veml3328_read(dev, VEML3328_CMD_REG_RED, &data->red_raw);
		if (ret < 0) {
			return ret;
		}
	}
	if ((chan_mask & VEML3328_BLUE) > 0) {
		ret = veml3328_read(dev, VEML3328_CMD_REG_BLUE, &data->blue_raw);
		if (ret < 0) {
			return ret;
		}
	}
	if ((chan_mask & VEML3328_CLEAR) > 0) {
		ret = veml3328_read(dev, VEML3328_CMD_REG_CLEAR, &data->clear_raw);
		if (ret < 0) {
			return ret;
		}
	}
	if ((chan_mask & VEML3328_IR) > 0) {
		ret = veml3328_read(dev, VEML3328_CMD_REG_IR, &data->ir_raw);
		if (ret < 0) {
			return ret;
		}
	}


	return 0;
}

static int veml3328_write_attribute(const struct device *dev,
				    enum sensor_attribute_veml3328 attr,
				    const struct sensor_value *val)
{
	int ret = 0;
	struct veml3328_config *config = (struct veml3328_config *)dev->config;
	uint16_t attr_val = val->val1;

	switch (attr) {
	case SENSOR_ATTR_VEML3328_DGFACT:
		if (attr_val > VEML3328_DG_GAIN_4) {
			attr_val = VEML3328_DG_GAIN_4;
		}
		config->dg = attr_val;
		break;
	case SENSOR_ATTR_VEML3328_GAIN:
		if (attr_val > VEML3328_GAIN_1_2) {
			attr_val = VEML3328_GAIN_1_2;
		}
		config->gain = attr_val;
		break;
	case SENSOR_ATTR_VEML3328_ITIME:
		if (attr_val > VEML3328_IT_400MS) {
			attr_val = VEML3328_IT_400MS;
		}
		config->itime = attr_val;
		break;
	case SENSOR_ATTR_VEML3328_SD_ALS_MODE:
		if (attr_val > VEML3328_SD_ALS_ONLY) {
			attr_val = VEML3328_SD_ALS_ONLY;
		}
		config->sd_als = attr_val;
		break;
	case SENSOR_ATTR_VEML3328_SENS_MODE:
		if (attr_val > VEML3328_SENSE_1_3) {
			attr_val = VEML3328_SENSE_1_3;
		}
		config->sens = attr_val;
		break;
	case SENSOR_ATTR_VEML3328_AF_MODE:
		if (attr_val > VEML3328_ACTIVEFORCE) {
			attr_val = VEML3328_ACTIVEFORCE;
		}
		config->af = attr_val;
		if (attr_val == VEML3328_ACTIVEFORCE) {
			ret = veml3328_set_pmstate(dev, VEML3328_PWROFF);
		} else {
			ret = veml3328_set_pmstate(dev, VEML3328_PWRON);
		}
		if (ret) {
			LOG_WRN("Active Force, set powerstate failed");
		}
		break;
	case SENSOR_ATTR_VEML3328_TRIG:
		if (attr_val > VEML3328_TRIGGER_ON) {
			attr_val = VEML3328_TRIGGER_ON;
		}
		config->trig = attr_val;
		break;
	default:
		return -ENOTSUP;
	};

	return veml3328_write_conf(dev);
}

static int veml3328_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	enum sensor_attribute_veml3328 vemlattr;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	vemlattr = (enum sensor_attribute_veml3328) attr;
	if ((vemlattr >= SENSOR_ATTR_VEML3328_DGFACT) &&
		(vemlattr <= SENSOR_ATTR_VEML3328_TRIG)) {
		return veml3328_write_attribute(dev, vemlattr, val);
	} else {
		return -ENOTSUP;
	}
}

static uint16_t veml3328_read_attribute(const struct device *dev,
				    enum sensor_attribute_veml3328 attr)
{
	uint16_t retval = 0;
	const struct veml3328_config *config = dev->config;

	switch (attr) {
	case SENSOR_ATTR_VEML3328_DGFACT:
		retval = config->dg;
		break;
	case SENSOR_ATTR_VEML3328_GAIN:
		retval = config->gain;
		break;
	case SENSOR_ATTR_VEML3328_ITIME:
		retval = config->itime;
		break;
	case SENSOR_ATTR_VEML3328_SD_ALS_MODE:
		retval = config->sd_als;
		break;
	case SENSOR_ATTR_VEML3328_SENS_MODE:
		retval = config->sens;
		break;
	case SENSOR_ATTR_VEML3328_AF_MODE:
		retval = config->af;
		break;
	case SENSOR_ATTR_VEML3328_TRIG:
		retval = config->trig;
		break;
	default:
		break;
	};

	return retval;
}

static int veml3328_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	enum sensor_attribute_veml3328 vemlattr;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	vemlattr = (enum sensor_attribute_veml3328) attr;
	if ((vemlattr >= SENSOR_ATTR_VEML3328_DGFACT) &&
		(vemlattr <= SENSOR_ATTR_VEML3328_TRIG)) {
		val->val1 = veml3328_read_attribute(dev, vemlattr);
	} else {
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static int veml3328_af_measurement(const struct device *dev)
{
	int ret;
	uint8_t retry = 3;
	uint16_t confdata;
	struct veml3328_config *conf = (struct veml3328_config *)dev->config;

	if ((conf->config_bits & VEML3328_PWRDOWN_BITS) > 0) {
		ret = veml3328_set_pmstate(dev, VEML3328_PWRON);
		if (ret) {
			return ret;
		}
	}

	/* trigger the meassurement, wait for integration */
	conf->trig = VEML3328_TRIGGER_ON;
	ret = veml3328_write_conf(dev);
	if (ret) {
		return ret;
	}
	k_msleep(integration_times_ms[conf->itime]);

	/* integration time is over, the trigger bit should be
	 * cleared, wait additional 15ms maximum
	 */
	do {
		k_msleep(5);
		veml3328_read_conf(dev, &confdata);
		--retry;
	} while (retry > 0 && ((confdata & BIT(2)) > 0));

	/* in AF mode shutdown */
	return veml3328_set_pmstate(dev, VEML3328_PWROFF);
}

static int veml3328_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	bool sd_als_only;
	uint8_t chan_mask = 0;
	enum sensor_channel_veml3328 priv_chan;
	const struct veml3328_config *conf = dev->config;

	/* if AF mode is set, trigger single measurement */
	if (conf->af == VEML3328_ACTIVEFORCE) {
		int ret = veml3328_af_measurement(dev);

		if (ret < 0) {
			return ret;
		}
	}

	/* cast private channels, and check sd_als_only flag */
	priv_chan = (enum sensor_channel_veml3328) chan;
	sd_als_only = conf->sd_als > 0 ? true : false;

	if (chan == SENSOR_CHAN_LIGHT || priv_chan == SENSOR_CHAN_VEML3328_LUX_SENSING) {
		chan_mask = VEML3328_LUX;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_CLEAR_RAW) {
		chan_mask = VEML3328_CLEAR;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_IR_RAW) {
		chan_mask = VEML3328_IR;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_RED_RAW) {
		if (!sd_als_only) {
			chan_mask = VEML3328_RED;
		}
	} else if (priv_chan == SENSOR_CHAN_VEML3328_GREEN_RAW) {
		chan_mask = VEML3328_GREEN;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_BLUE_RAW) {
		if (!sd_als_only) {
			chan_mask = VEML3328_BLUE;
		}
	} else if (chan == SENSOR_CHAN_ALL) {
		chan_mask = VEML3328_CLEAR | VEML3328_IR | VEML3328_GREEN | VEML3328_LUX;
		if (!sd_als_only) {
			chan_mask |= (VEML3328_RED | VEML3328_BLUE);
		}
	} else {
		return -ENOTSUP;
	}

	return veml3328_fetch(dev, chan_mask);
}

static int veml3328_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	enum sensor_channel_veml3328 priv_chan;
	struct veml3328_data *data = dev->data;

	val->val2 = 0;
	priv_chan = (enum sensor_channel_veml3328) chan;
	if (chan == SENSOR_CHAN_LIGHT || priv_chan == SENSOR_CHAN_VEML3328_LUX_SENSING) {
		val->val2 = data->lux_high;
		val->val1 = data->lux_low;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_CLEAR_RAW) {
		val->val1 = data->clear_raw;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_IR_RAW) {
		val->val1 = data->ir_raw;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_RED_RAW) {
		val->val1 = data->red_raw;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_GREEN_RAW) {
		val->val1 = data->green_raw;
	} else if (priv_chan == SENSOR_CHAN_VEML3328_BLUE_RAW) {
		val->val1 = data->green_raw;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int veml3328_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return veml3328_set_pmstate(dev, VEML3328_PWROFF);

	case PM_DEVICE_ACTION_RESUME:
		return veml3328_set_pmstate(dev, VEML3328_PWRON);

	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif /* CONFIG_PM_DEVICE */

static int veml3328_init(const struct device *dev)
{
	int ret;
	uint16_t device_id;
	struct veml3328_config *conf = (struct veml3328_config *)dev->config;
	struct veml3328_data *data = dev->data;

	if (!i2c_is_ready_dt(&conf->bus)) {
		LOG_ERR_DEVICE_NOT_READY(conf->bus.bus);
		return -ENODEV;
	}

	ret = veml3328_read_deviceid(dev, &device_id);
	if (ret) {
		return ret;
	}

	device_id &= 0x00ff; /* lsb only */

	if (device_id != VEML3328_DEVICE_ID) {
		LOG_ERR("Unknown Device ID: 0x%x", device_id);
		return -ENODEV;
	}

	/* clean startup data values */
	memset(data, 0, sizeof(struct veml3328_data));

	/* power on, reserves = 0, trigger = 0 */
	conf->sd0 = 0;
	conf->sd1 = 0;
	conf->res3 = 0;
	conf->res1 = 0;
	conf->trig = 0;

	return veml3328_write_conf(dev);
}

static DEVICE_API(sensor, veml3328_api) = {
	.sample_fetch = veml3328_sample_fetch,
	.channel_get = veml3328_channel_get,
	.attr_set = veml3328_attr_set,
	.attr_get = veml3328_attr_get,
};

#define VEML3328_INIT(n)                                                                           \
	static struct veml3328_data veml3328_data_##n;                                             \
	                                                                                           \
	static const struct veml3328_config veml3328_config_##n = {                                \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.sd_als = DT_INST_PROP_HAS_NAME(n, sd-als-only, 0),                                \
		.dg = DT_INST_ENUM_IDX_OR(n, dg, 0),                                               \
		.sens = DT_INST_PROP_HAS_NAME(n, sens, 0),                                         \
		.gain = DT_INST_ENUM_IDX_OR(n, gain, 0),                                           \
		.itime = DT_INST_ENUM_IDX_OR(n, it, 0),                                            \
		.af = DT_INST_PROP_HAS_NAME(n, active-force-mode, 0),                              \
		};                                                                                 \
		                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, veml3328_pm_action);                                           \
	                                                                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(n, veml3328_init, PM_DEVICE_DT_INST_GET(n),                   \
				     &veml3328_data_##n, &veml3328_config_##n, POST_KERNEL,        \
				     CONFIG_SENSOR_INIT_PRIORITY, &veml3328_api);

DT_INST_FOREACH_STATUS_OKAY(VEML3328_INIT)
