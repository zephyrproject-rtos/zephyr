/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pixart_pat912x

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_pat912x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(input_pat912x, CONFIG_INPUT_LOG_LEVEL);

#define PAT912X_PRODUCT_ID1	0x00
#define PAT912X_PRODUCT_ID2	0x01
#define PAT912X_MOTION_STATUS	0x02
#define PAT912X_DELTA_X_LO	0x03
#define PAT912X_DELTA_Y_LO	0x04
#define PAT912X_OPERATION_MODE	0x05
#define PAT912X_CONFIGURATION	0x06
#define PAT912X_WRITE_PROTECT	0x09
#define PAT912X_SLEEP1		0x0a
#define PAT912X_SLEEP2		0x0b
#define PAT912X_RES_X		0x0d
#define PAT912X_RES_Y		0x0e
#define PAT912X_DELTA_XY_HI	0x12
#define PAT912X_SHUTTER		0x14
#define PAT912X_FRAME_AVG	0x17
#define PAT912X_ORIENTATION	0x19

#define PRODUCT_ID_PAT9125EL 0x3191

#define CONFIGURATION_RESET 0x97
#define CONFIGURATION_CLEAR 0x17
#define CONFIGURATION_PD_ENH BIT(3)
#define WRITE_PROTECT_ENABLE 0x00
#define WRITE_PROTECT_DISABLE 0x5a
#define MOTION_STATUS_MOTION BIT(7)
#define RES_SCALING_FACTOR 5
#define RES_MAX (UINT8_MAX * RES_SCALING_FACTOR)
#define OPERATION_MODE_SLEEP_1_EN BIT(4)
#define OPERATION_MODE_SLEEP_12_EN (BIT(4) | BIT(3))

#define PAT912X_DATA_SIZE_BITS 12

#define RESET_DELAY_MS 2

struct pat912x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec motion_gpio;
	int32_t axis_x;
	int32_t axis_y;
	int16_t res_x_cpi;
	int16_t res_y_cpi;
	bool invert_x;
	bool invert_y;
	bool sleep1_enable;
	bool sleep2_enable;
};

struct pat912x_data {
	const struct device *dev;
	struct k_work motion_work;
	struct gpio_callback motion_cb;
};

static void pat912x_motion_work_handler(struct k_work *work)
{
	struct pat912x_data *data = CONTAINER_OF(
			work, struct pat912x_data, motion_work);
	const struct device *dev = data->dev;
	const struct pat912x_config *cfg = dev->config;
	int32_t x, y;
	uint8_t val;
	uint8_t xy[2];
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, PAT912X_MOTION_STATUS, &val);
	if (ret < 0) {
		return;
	}

	if ((val & MOTION_STATUS_MOTION) == 0x00) {
		return;
	}

	ret = i2c_burst_read_dt(&cfg->i2c, PAT912X_DELTA_X_LO, xy, sizeof(xy));
	if (ret < 0) {
		return;
	}
	x = xy[0];
	y = xy[1];

	ret = i2c_reg_read_byte_dt(&cfg->i2c, PAT912X_DELTA_XY_HI, &val);
	if (ret < 0) {
		return;
	}
	y |= (val << 8) & 0xf00;
	x |= (val << 4) & 0xf00;

	x = sign_extend(x, PAT912X_DATA_SIZE_BITS - 1);
	y = sign_extend(y, PAT912X_DATA_SIZE_BITS - 1);

	if (cfg->invert_x) {
		x *= -1;
	}
	if (cfg->invert_y) {
		y *= -1;
	}

	LOG_DBG("x=%4d y=%4d", x, y);

	if (cfg->axis_x >= 0) {
		bool sync = cfg->axis_y < 0;

		input_report_rel(data->dev, cfg->axis_x, x, sync, K_FOREVER);
	}

	if (cfg->axis_y >= 0) {
		input_report_rel(data->dev, cfg->axis_y, y, true, K_FOREVER);
	}

	/* Trigger one more scan in case more data is available. */
	k_work_submit(&data->motion_work);
}

static void pat912x_motion_handler(const struct device *gpio_dev,
				   struct gpio_callback *cb,
				   uint32_t pins)
{
	struct pat912x_data *data = CONTAINER_OF(
			cb, struct pat912x_data, motion_cb);

	k_work_submit(&data->motion_work);
}

int pat912x_set_resolution(const struct device *dev,
			   int16_t res_x_cpi, int16_t res_y_cpi)
{
	const struct pat912x_config *cfg = dev->config;
	int ret;

	if (res_x_cpi >= 0) {
		if (!IN_RANGE(res_x_cpi, 0, RES_MAX)) {
			LOG_ERR("res_x_cpi out of range: %d", res_x_cpi);
			return -EINVAL;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, PAT912X_RES_X,
					    res_x_cpi / RES_SCALING_FACTOR);
		if (ret < 0) {
			return ret;
		}
	}

	if (res_y_cpi >= 0) {
		if (!IN_RANGE(res_y_cpi, 0, RES_MAX)) {
			LOG_ERR("res_y_cpi out of range: %d", res_y_cpi);
			return -EINVAL;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, PAT912X_RES_Y,
					    res_y_cpi / RES_SCALING_FACTOR);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int pat912x_configure(const struct device *dev)
{
	const struct pat912x_config *cfg = dev->config;
	uint8_t id[2];
	int ret;

	ret = i2c_burst_read_dt(&cfg->i2c, PAT912X_PRODUCT_ID1, id, sizeof(id));
	if (ret < 0) {
		return ret;
	}

	if (sys_get_be16(id) != PRODUCT_ID_PAT9125EL) {
		LOG_ERR("Invalid product id: %04x", sys_get_be16(id));
		return -ENOTSUP;
	}

	/* Software reset */

	i2c_reg_write_byte_dt(&cfg->i2c, PAT912X_CONFIGURATION, CONFIGURATION_RESET);
	/* no ret value check, the device NACKs */

	k_sleep(K_MSEC(RESET_DELAY_MS));

	ret = i2c_reg_write_byte_dt(&cfg->i2c, PAT912X_CONFIGURATION, CONFIGURATION_CLEAR);
	if (ret < 0) {
		return ret;
	}

	ret = pat912x_set_resolution(dev, cfg->res_x_cpi, cfg->res_y_cpi);
	if (ret < 0) {
		return ret;
	}

	if (cfg->sleep1_enable && cfg->sleep2_enable) {
		ret = i2c_reg_update_byte_dt(&cfg->i2c,
					     PAT912X_OPERATION_MODE,
					     OPERATION_MODE_SLEEP_12_EN,
					     OPERATION_MODE_SLEEP_12_EN);
		if (ret < 0) {
			return ret;
		}
	} else if (cfg->sleep1_enable) {
		ret = i2c_reg_update_byte_dt(&cfg->i2c,
					     PAT912X_OPERATION_MODE,
					     OPERATION_MODE_SLEEP_12_EN,
					     OPERATION_MODE_SLEEP_1_EN);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int pat912x_init(const struct device *dev)
{
	const struct pat912x_config *cfg = dev->config;
	struct pat912x_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("%s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->motion_work, pat912x_motion_work_handler);

	if (!gpio_is_ready_dt(&cfg->motion_gpio)) {
		LOG_ERR("%s is not ready", cfg->motion_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->motion_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Motion pin configuration failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->motion_gpio,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Motion interrupt configuration failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->motion_cb, pat912x_motion_handler,
			   BIT(cfg->motion_gpio.pin));

	ret = pat912x_configure(dev);
	if (ret != 0) {
		LOG_ERR("Device configuration failed: %d", ret);
		return ret;
	}

	ret = gpio_add_callback_dt(&cfg->motion_gpio, &data->motion_cb);
	if (ret < 0) {
		LOG_ERR("Could not set motion callback: %d", ret);
		return ret;
	}

	/* Trigger an initial read to clear any pending motion status.*/
	k_work_submit(&data->motion_work);

	ret = pm_device_runtime_enable(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable runtime power management");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int pat912x_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	const struct pat912x_config *cfg = dev->config;
	uint8_t val;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		val = CONFIGURATION_PD_ENH;
		break;
	case PM_DEVICE_ACTION_RESUME:
		val = 0;
		break;
	default:
		return -ENOTSUP;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, PAT912X_CONFIGURATION,
				     CONFIGURATION_PD_ENH, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
#endif

#define PAT912X_INIT(n)								\
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP_OR(n, res_x_cpi, 0), 0, RES_MAX),	\
		     "invalid res-x-cpi");					\
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP_OR(n, res_y_cpi, 0), 0, RES_MAX),	\
		     "invalid res-y-cpi");					\
	BUILD_ASSERT(DT_INST_PROP(n, sleep1_enable) ||				\
		     !DT_INST_PROP(n, sleep2_enable),				\
		     "invalid sleep configuration");				\
										\
	static const struct pat912x_config pat912x_cfg_##n = {			\
		.i2c = I2C_DT_SPEC_INST_GET(n),					\
		.motion_gpio = GPIO_DT_SPEC_INST_GET(n, motion_gpios),		\
		.axis_x = DT_INST_PROP_OR(n, zephyr_axis_x, -1),		\
		.axis_y = DT_INST_PROP_OR(n, zephyr_axis_y, -1),		\
		.res_x_cpi = DT_INST_PROP_OR(n, res_x_cpi, -1),			\
		.res_y_cpi = DT_INST_PROP_OR(n, res_y_cpi, -1),			\
		.invert_x = DT_INST_PROP(n, invert_x),				\
		.invert_y = DT_INST_PROP(n, invert_y),				\
		.sleep1_enable = DT_INST_PROP(n, sleep1_enable),		\
		.sleep2_enable = DT_INST_PROP(n, sleep2_enable),		\
	};									\
										\
	static struct pat912x_data pat912x_data_##n;				\
										\
	PM_DEVICE_DT_INST_DEFINE(n, pat912x_pm_action);				\
										\
	DEVICE_DT_INST_DEFINE(n, pat912x_init, PM_DEVICE_DT_INST_GET(n),	\
			      &pat912x_data_##n, &pat912x_cfg_##n,		\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,		\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PAT912X_INIT)
