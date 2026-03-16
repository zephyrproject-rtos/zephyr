/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 *SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avago_apds9960

/* @file
 * @brief driver for APDS9960 ALS/RGB/gesture/proximity sensor
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/apds9960.h>

LOG_MODULE_REGISTER(APDS9960, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_APDS9960_FETCH_MODE_INTERRUPT
static void apds9960_handle_cb(struct apds9960_data *drv_data)
{
	apds9960_setup_int(drv_data->dev->config, false);

#ifdef CONFIG_APDS9960_TRIGGER
	k_work_submit(&drv_data->work);
#else
	k_sem_give(&drv_data->data_sem);
#endif
}

static void apds9960_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct apds9960_data *drv_data =
		CONTAINER_OF(cb, struct apds9960_data, gpio_cb);

	apds9960_handle_cb(drv_data);
}
#endif

#if CONFIG_APDS9960_ENABLE_GESTURE
static void apds9960_gesture_determine(struct apds9960_data *data,
					uint8_t *gesture_fifo, int ir_difference)
{
	int tmp_up;
	int tmp_left;
	int net_up = 0;
	int net_left = 0;

	static bool up_trig;
	static bool down_trig;
	static bool left_trig;
	static bool right_trig;

	tmp_up = (int) gesture_fifo[0] - (int) gesture_fifo[1];
	tmp_left = (int) gesture_fifo[2] - (int) gesture_fifo[3];

	if (abs(tmp_up) > ir_difference && abs(tmp_up) > abs(tmp_left)) {
		net_up = tmp_up;
	}
	if (abs(tmp_left) > ir_difference && abs(tmp_left) > abs(tmp_up)) {
		net_left = tmp_left;
	}

	if (net_up > 0) {
		if (down_trig) {
			data->gesture = APDS9960_GESTURE_DOWN;
			up_trig = false;
			down_trig = false;
			left_trig = false;
			right_trig = false;
		} else {
			up_trig = true;
		}
	} else if (net_up < 0) {
		if (up_trig) {
			data->gesture = APDS9960_GESTURE_UP;
			up_trig = false;
			down_trig = false;
			left_trig = false;
			right_trig = false;
		} else {
			down_trig = true;
		}
	} else {
		/* No movement in up down direction */
	}
	if (net_left > 0) {
		if (right_trig) {
			data->gesture = APDS9960_GESTURE_RIGHT;
			up_trig = false;
			down_trig = false;
			left_trig = false;
			right_trig = false;
		} else {
			left_trig = true;
		}
	} else if (net_left < 0) {
		if (left_trig) {
			data->gesture = APDS9960_GESTURE_LEFT;
			up_trig = false;
			down_trig = false;
			left_trig = false;
			right_trig = false;
		} else {
			right_trig = true;
		}
	} else {
		/* No movement in left right direction*/
	}
	LOG_DBG("Net up: 0x%x, Net left: 0x%x", net_up, net_left);

}
static int apds9960_gesture_fetch(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *data = dev->data;

	uint8_t gesture_fifo_cnt;
	uint8_t gstatus;
	uint8_t gesture_fifo[4];

	data->gesture = APDS9960_GESTURE_NONE;

	if (i2c_reg_read_byte_dt(&config->i2c,
			APDS9960_GSTATUS_REG, &gstatus)) {
		return -EIO;
	}

	while (gstatus & APDS9960_GSTATUS_GVALID) {
		if (i2c_reg_read_byte_dt(&config->i2c,
					APDS9960_GFLVL_REG, &gesture_fifo_cnt)) {
			return -EIO;
		}

		for (int i = 0; i < gesture_fifo_cnt; ++i) {
			/* Read up fifo and adjacent registers */
			if (i2c_burst_read_dt(&config->i2c,
					APDS9960_GFIFO_U_REG,
					(uint8_t *) gesture_fifo,
					4)) {
				return -EIO;
			}

			apds9960_gesture_determine(data, gesture_fifo,
				config->gesture_config.ir_difference);
		}

		if (i2c_reg_read_byte_dt(&config->i2c,
				APDS9960_GSTATUS_REG, &gstatus)) {
			return -EIO;
		}
	}

	return 0;
}
#endif

static int apds9960_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *data = dev->data;
	uint8_t tmp;
#ifdef CONFIG_APDS9960_FETCH_MODE_POLL
	int64_t start_time;
#endif

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

#ifdef CONFIG_APDS9960_ENABLE_GESTURE
	if (apds9960_gesture_fetch(dev)) {
		return -EIO;
	}
#endif

#ifndef CONFIG_APDS9960_TRIGGER
#ifdef CONFIG_APDS9960_FETCH_MODE_INTERRUPT
	apds9960_setup_int(config, true);

#ifdef CONFIG_APDS9960_ENABLE_ALS
	tmp = APDS9960_ENABLE_PON | APDS9960_ENABLE_AIEN;
#else
	tmp = APDS9960_ENABLE_PON | APDS9960_ENABLE_PIEN;
#endif
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG, tmp, tmp)) {
		LOG_ERR("Power on bit not set.");
		return -EIO;
	}
	k_sem_take(&data->data_sem, K_FOREVER);
#endif
#endif

	if (i2c_reg_read_byte_dt(&config->i2c,
			      APDS9960_STATUS_REG, &tmp)) {
		return -EIO;
	}

#ifdef CONFIG_APDS9960_FETCH_MODE_POLL
	start_time = k_uptime_get();
#ifdef CONFIG_APDS9960_ENABLE_ALS
	while (!(tmp & APDS9960_STATUS_AINT)) {
#else
	while (!(tmp & APDS9960_STATUS_PINT)) {
#endif
		k_sleep(K_MSEC(APDS9960_DEFAULT_WAIT_TIME));
		if (i2c_reg_read_byte_dt(&config->i2c, APDS9960_STATUS_REG, &tmp)) {
			return -EIO;
		}
		if ((k_uptime_get() - start_time) > APDS9960_MAX_WAIT_TIME) {
			return -ETIMEDOUT;
		}
	}
#endif

	LOG_DBG("status: 0x%x", tmp);
	if (tmp & APDS9960_STATUS_PINT) {
		if (i2c_reg_read_byte_dt(&config->i2c,
				      APDS9960_PDATA_REG, &data->pdata)) {
			return -EIO;
		}
	}

	if (tmp & APDS9960_STATUS_AINT) {
		if (i2c_burst_read_dt(&config->i2c,
				   APDS9960_CDATAL_REG,
				   (uint8_t *)&data->sample_crgb,
				   sizeof(data->sample_crgb))) {
			return -EIO;
		}

	}

#ifndef CONFIG_APDS9960_TRIGGER
#ifdef CONFIG_APDS9960_FETCH_MODE_INTERRUPT
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG,
				APDS9960_ENABLE_PON,
				0)) {
		return -EIO;
	}
#endif
#endif

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_AICLEAR_REG, 0)) {
		return -EIO;
	}

	return 0;
}

static int apds9960_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct apds9960_data *data = dev->data;

	switch (chan) {
#ifdef CONFIG_APDS9960_ENABLE_ALS
	case SENSOR_CHAN_LIGHT:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[0]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_RED:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[1]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[2]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[3]);
		val->val2 = 0;
		break;
#endif
#ifdef CONFIG_APDS9960_ENABLE_GESTURE
	case SENSOR_CHAN_APDS9960_GESTURE:
		val->val1 = data->gesture;
		val->val2 = 0;
		break;
#endif
	case SENSOR_CHAN_PROX:
		val->val1 = data->pdata;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int apds9960_proxy_setup(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_POFFSET_UR_REG,
			       APDS9960_DEFAULT_POFFSET_UR)) {
		LOG_ERR("Default offset UR not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_POFFSET_DL_REG,
			       APDS9960_DEFAULT_POFFSET_DL)) {
		LOG_ERR("Default offset DL not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PPULSE_REG,
			       config->ppcount)) {
		LOG_ERR("Default pulse count not set ");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONTROL_REG,
				APDS9960_CONTROL_LDRIVE,
				APDS9960_DEFAULT_LDRIVE)) {
		LOG_ERR("LED Drive Strength not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONFIG2_REG,
				APDS9960_PLED_BOOST_300,
				config->pled_boost)) {
		LOG_ERR("LED Drive Strength not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONTROL_REG, APDS9960_CONTROL_PGAIN,
				(config->pgain & APDS9960_PGAIN_8X))) {
		LOG_ERR("Gain is not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PILT_REG, APDS9960_DEFAULT_PILT)) {
		LOG_ERR("Low threshold not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PIHT_REG, APDS9960_DEFAULT_PIHT)) {
		LOG_ERR("High threshold not set");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG, APDS9960_ENABLE_PEN,
				APDS9960_ENABLE_PEN)) {
		LOG_ERR("Proximity mode is not enabled");
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_APDS9960_ENABLE_ALS
static int apds9960_ambient_setup(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	uint16_t th;

	/* ADC value */
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_ATIME_REG, APDS9960_DEFAULT_ATIME)) {
		LOG_ERR("Default integration time not set for ADC");
		return -EIO;
	}

	/* ALS Gain */
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_CONTROL_REG,
				APDS9960_CONTROL_AGAIN,
				(config->again & APDS9960_AGAIN_64X))) {
		LOG_ERR("Ambient Gain is not set");
		return -EIO;
	}

	th = sys_cpu_to_le16(APDS9960_DEFAULT_AILT);
	if (i2c_burst_write_dt(&config->i2c,
			    APDS9960_INT_AILTL_REG,
			    (uint8_t *)&th, sizeof(th))) {
		LOG_ERR("ALS low threshold not set");
		return -EIO;
	}

	th = sys_cpu_to_le16(APDS9960_DEFAULT_AIHT);
	if (i2c_burst_write_dt(&config->i2c,
			    APDS9960_INT_AIHTL_REG,
			    (uint8_t *)&th, sizeof(th))) {
		LOG_ERR("ALS low threshold not set");
		return -EIO;
	}

	/* Enable ALS */
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG, APDS9960_ENABLE_AEN,
				APDS9960_ENABLE_AEN)) {
		LOG_ERR("ALS is not enabled");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef CONFIG_APDS9960_ENABLE_GESTURE
static int apds9960_gesture_setup(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;

	if (i2c_reg_write_byte_dt(&config->i2c,
				APDS9960_GPENTH_REG, config->gesture_config.proximity)) {
		LOG_ERR("Gesture proximity enter not set.");
		return -EIO;
	}
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_GEXTH_REG, config->gesture_config.proximity)) {
		LOG_ERR("Gesture proximity exit not set.");
		return -EIO;
	}
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_GCONFIG1_REG, 0)) {
		LOG_ERR("Gesture config 1 not set.");
		return -EIO;
	}
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_GCONFIG2_REG, APDS9960_GGAIN_4X)) {
		LOG_ERR("Gesture config 2 not set.");
		return -EIO;
	}
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_GCONFIG4_REG, 0)) {
		LOG_ERR("Gesture config 4 not set.");
		return -EIO;
	}
	if (i2c_reg_update_byte_dt(&config->i2c, APDS9960_ENABLE_REG, APDS9960_ENABLE_GEN,
				   APDS9960_ENABLE_GEN)) {
		LOG_ERR("Gesture on bit not set.");
		return -EIO;
	}
	return 0;
}
#endif

static int apds9960_sensor_setup(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	uint8_t chip_id;

	if (i2c_reg_read_byte_dt(&config->i2c,
			      APDS9960_ID_REG, &chip_id)) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	if (!((chip_id == APDS9960_ID_1) || (chip_id == APDS9960_ID_2))) {
		LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* Disable all functions and interrupts */
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_ENABLE_REG, 0)) {
		LOG_ERR("ENABLE register is not cleared");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_AICLEAR_REG, 0)) {
		return -EIO;
	}

	/* Disable gesture interrupt */
	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_GCONFIG4_REG, 0)) {
		LOG_ERR("GCONFIG4 register is not cleared");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_WTIME_REG, APDS9960_DEFAULT_WTIME)) {
		LOG_ERR("Default wait time not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_CONFIG1_REG,
			       APDS9960_DEFAULT_CONFIG1)) {
		LOG_ERR("Default WLONG not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_CONFIG2_REG,
			       APDS9960_DEFAULT_CONFIG2)) {
		LOG_ERR("Configuration Register Two not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_CONFIG3_REG,
			       APDS9960_DEFAULT_CONFIG3)) {
		LOG_ERR("Configuration Register Three not set");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       APDS9960_PERS_REG,
			       APDS9960_DEFAULT_PERS)) {
		LOG_ERR("Interrupt persistence not set");
		return -EIO;
	}

	if (apds9960_proxy_setup(dev)) {
		LOG_ERR("Failed to setup proximity functionality");
		return -EIO;
	}

#ifdef CONFIG_APDS9960_ENABLE_ALS
	if (apds9960_ambient_setup(dev)) {
		LOG_ERR("Failed to setup ambient light functionality");
		return -EIO;
	}
#endif

#ifdef CONFIG_APDS9960_ENABLE_GESTURE
	if (apds9960_gesture_setup(dev)) {
		LOG_ERR("Failed to setup gesture functionality");
		return -EIO;
	}
#endif

#ifdef CONFIG_APDS9960_FETCH_MODE_POLL
	if (i2c_reg_update_byte_dt(&config->i2c, APDS9960_ENABLE_REG, APDS9960_ENABLE_PON,
				   APDS9960_ENABLE_PON)) {
		LOG_ERR("Power on bit not set.");
		return -EIO;
	}
#endif

	return 0;
}

#ifdef CONFIG_APDS9960_FETCH_MODE_INTERRUPT
static int apds9960_init_interrupt(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *drv_data = dev->data;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
			config->int_gpio.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | config->int_gpio.dt_flags);

	gpio_init_callback(&drv_data->gpio_cb,
			   apds9960_gpio_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

	drv_data->dev = dev;

#ifdef CONFIG_APDS9960_TRIGGER
	drv_data->work.handler = apds9960_work_cb;
	if (i2c_reg_update_byte_dt(&config->i2c,
				APDS9960_ENABLE_REG,
				APDS9960_ENABLE_PON,
				APDS9960_ENABLE_PON)) {
		LOG_ERR("Power on bit not set.");
		return -EIO;
	}

#else
	k_sem_init(&drv_data->data_sem, 0, K_SEM_MAX_LIMIT);
#endif
	apds9960_setup_int(config, true);

	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		apds9960_handle_cb(drv_data);
	}

	return 0;
}
#endif

#ifdef CONFIG_PM_DEVICE
static int apds9960_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	const struct apds9960_config *config = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (i2c_reg_update_byte_dt(&config->i2c,
					APDS9960_ENABLE_REG,
					APDS9960_ENABLE_PON,
					APDS9960_ENABLE_PON)) {
			ret = -EIO;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (i2c_reg_update_byte_dt(&config->i2c,
					APDS9960_ENABLE_REG,
					APDS9960_ENABLE_PON, 0)) {
			ret = -EIO;
		}

		if (i2c_reg_write_byte_dt(&config->i2c,
				       APDS9960_AICLEAR_REG, 0)) {
			ret = -EIO;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int apds9960_init(const struct device *dev)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *data = dev->data;

	/* Initialize time 5.7ms */
	k_sleep(K_MSEC(6));

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	(void)memset(data->sample_crgb, 0, sizeof(data->sample_crgb));
	data->pdata = 0U;

	if (apds9960_sensor_setup(dev) < 0) {
		LOG_ERR("Failed to setup device!");
		return -EIO;
	}

#ifdef CONFIG_APDS9960_FETCH_MODE_INTERRUPT
	if (apds9960_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

static DEVICE_API(sensor, apds9960_driver_api) = {
	.sample_fetch = &apds9960_sample_fetch,
	.channel_get = &apds9960_channel_get,
#ifdef CONFIG_APDS9960_TRIGGER
	.attr_set = apds9960_attr_set,
	.trigger_set = apds9960_trigger_set,
#endif
};

#if CONFIG_APDS9960_FETCH_MODE_INTERRUPT
#define APDS9960_CONFIG_INTERRUPT(inst) \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),
#else
#define APDS9960_CONFIG_INTERRUPT(inst)
#endif

#if CONFIG_APDS9960_ENABLE_GESTURE
#define APDS9960_CONFIG_GESTURE(inst)                                                              \
		.gesture_config = {                                                                \
			.proximity = DT_INST_PROP(inst, proximity),                                \
			.ir_difference = DT_INST_PROP(inst, ir_difference),                        \
		},
#else
#define APDS9960_CONFIG_GESTURE(inst)
#endif

#define APDS9960_INIT(i)                                                                           \
	static struct apds9960_data apds9960_data_##i;                                             \
	static const struct apds9960_config apds9960_config_##i = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(i),                                                    \
		APDS9960_CONFIG_INTERRUPT(i)                                                       \
		.pgain = DT_INST_PROP(i, pgain) << 1,                                              \
		.again = DT_INST_PROP(i, again),                                                   \
		.ppcount = DT_INST_PROP(i, ppulse_length) | (DT_INST_PROP(i, ppulse_count) - 1),   \
		.pled_boost = DT_INST_PROP(i, pled_boost) << 4,                                    \
		APDS9960_CONFIG_GESTURE(i)                                                         \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(i, apds9960_pm_action);                                           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(i, apds9960_init,                                             \
		PM_DEVICE_DT_INST_GET(i), &apds9960_data_##i, &apds9960_config_##i,                \
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &apds9960_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APDS9960_INIT)
