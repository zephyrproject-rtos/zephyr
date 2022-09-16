/* bh1750.c - Driver for BH1750 light sensor */

/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1750

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(BH1750, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BH1750 driver enabled without any devices"
#endif

#define BH1750_POWER_DOWN                          0x00
#define BH1750_POWER_ON                            0x01
#define BH1750_RESET                               0x07
#define BH1750_CONTINUOUSLY_HIGH_RESOLUTION_MODE   0x10
#define BH1750_CONTINUOUSLY_HIGH_RESOLUTION_MODE_2 0x11
#define BH1750_CONTINUOUSLY_LOW_RESOLUTION_MODE    0x13
#define BH1750_ONE_TIME_HIGH_RESOLUTION_MODE       0x20
#define BH1750_ONE_TIME_HIGH_RESOLUTION_MODE_2     0x21
#define BH1750_ONE_TIME_LOW_RESOLUTION_MODE        0x23
#define BH1750_CHANGE_MEASUREMENT_TIME_HIGH        0x40
#define BH1750_CHANGE_MEASUREMENT_TIME_LOW         0x60

struct bh1750_data {
	uint16_t raw_val;
};

struct bh1750_config {
	struct i2c_dt_spec i2c;
};

static inline int bh1750_is_ready(const struct device *dev)
{
	const struct bh1750_config *cfg = dev->config;

	return device_is_ready(cfg->i2c.bus) ? 0 : -ENODEV;
}

static inline int bh1750_read(const struct device *dev, uint8_t *buf, int size)
{
	const struct bh1750_config *cfg = dev->config;

	return i2c_read_dt(&cfg->i2c, buf, size);
}

static inline int bh1750_write(const struct device *dev, uint8_t val)
{
	const struct bh1750_config *cfg = dev->config;
	uint8_t buf = val;

	return i2c_write_dt(&cfg->i2c, &buf, sizeof(buf));
}

static int bh1750_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bh1750_data *data = dev->data;
	uint8_t buf[2];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;
	(void)pm_device_state_get(dev, &state);
	/* Do not allow sample fetching from suspended state */
	if (state == PM_DEVICE_STATE_SUSPENDED) {
		return -EIO;
	}
#endif

	ret = bh1750_write(dev, BH1750_ONE_TIME_HIGH_RESOLUTION_MODE);
	if (ret < 0) {
		LOG_DBG("One time high resolution mode failed: %d", ret);
		return ret;
	}

	/* Wait for the measure to be ready */
	k_sleep(K_MSEC(180)); /* Max for H-Resolution Mode */

	ret = bh1750_read(dev, buf, sizeof(buf));
	if (ret < 0) {
		LOG_DBG("Read failed: %d", ret);
		return ret;
	}

	data->raw_val = sys_get_be16(&buf[0]);

	return 0;
}

static int bh1750_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bh1750_data *data = dev->data;
	double tmp;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		/*
		 * The documentation says the following about the calculation
		 * at section Measurement sequence example from "Write
		 * instruction" to "Read measurement result" p.17:
		 *
		 * How to calculate when the data High Byte is "10000011" and
		 * Low Byte is "10010000"
		 *
		 * (2^15 + 2^9 + 2^8 + 2^7 + 2^4) / 1.2 = 28067 [lx]
		 *
		 * Which means:
		 *
		 * lx = Slx / 1.2
		 */
		tmp = data->raw_val / 1.2;
		ret = sensor_value_from_double(val, tmp);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static const struct sensor_driver_api bh1750_api_funcs = {
	.sample_fetch = bh1750_sample_fetch,
	.channel_get = bh1750_channel_get,
};

static int bh1750_chip_init(const struct device *dev)
{
	int ret;

	ret = bh1750_is_ready(dev);
	if (ret < 0) {
		LOG_DBG("I2C bus check failed: %d", ret);
		return ret;
	}

	/*
	 * Make sure chip is in power on mode before reset.
	 *
	 * The documentation says the following about the reset command:
	 *
	 * Reset command is for only reset Illuminance data register (reset
	 * value is '0'). It is not necessary even power supply sequence. It is
	 * used for removing previous measurement result.  This command is not
	 * working in power down mode, so that please set the power on mode
	 * before input this command.
	 */
	ret = bh1750_write(dev, BH1750_POWER_ON);
	if (ret < 0) {
		LOG_DBG("Power on failed: %d", ret);
		return ret;
	}

	ret = bh1750_write(dev, BH1750_RESET);
	if (ret < 0) {
		LOG_DBG("Reset failed: %d", ret);
		return ret;
	}

	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int bh1750_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Put the chip into power up mode and reset */
		ret = bh1750_chip_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Put the chip into power down mode */
		ret = bh1750_write(dev, BH1750_POWER_DOWN);
		if (ret < 0) {
			LOG_DBG("Power down failed: %d", ret);
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#define BH1750_DEFINE(inst)						\
	static struct bh1750_data bh1750_data_##inst;			\
	static const struct bh1750_config bh1750_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(inst, bh1750_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			 bh1750_chip_init,				\
			 PM_DEVICE_DT_INST_GET(inst),			\
			 &bh1750_data_##inst,				\
			 &bh1750_config_##inst,				\
			 POST_KERNEL,					\
			 CONFIG_SENSOR_INIT_PRIORITY,			\
			 &bh1750_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(BH1750_DEFINE)
