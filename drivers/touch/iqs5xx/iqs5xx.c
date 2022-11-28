/*
 * Copyright (C) 2022 Ryan Walker <info@interruptlabs.ca>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT azoteq_iqs5xx

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/touch.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
LOG_MODULE_REGISTER(iqs5xx, CONFIG_TOUCH_LOG_LEVEL);

#include "iqs5xx.h"

static int iqs5xx_read_burst(const struct i2c_dt_spec *client, uint16_t reg, void *val,
				 uint16_t len)
{
	uint16_t be_reg = sys_cpu_to_be16(reg);
	int ret, i;
	bool fail = false;

	for (i = 0; i < IQS5XX_NUM_RETRIES; i++) {
		ret = i2c_write_read_dt(client, &be_reg, sizeof(be_reg), val, len);

		if (ret >= 0) {
			if (fail)
				LOG_INF("I2C Error Corrected");
			return 0;
		}

		LOG_ERR("I2C Transfer Failed, retrying");
		k_sleep(K_USEC(150));
		fail = true;
	}

	LOG_ERR("Failed to read from address 0x%04X: %d\n", reg, ret);

	return ret;
}

static int iqs5xx_read_word(const struct i2c_dt_spec *client, uint16_t reg, uint16_t *val)
{
	uint16_t val_buf;
	int error;

	error = iqs5xx_read_burst(client, reg, &val_buf, sizeof(val_buf));
	if (error) {
		return error;
	}

	*val = sys_be16_to_cpu(val_buf);

	return 0;
}

static int iqs5xx_write_burst(const struct i2c_dt_spec *client, uint16_t reg, const void *val,
				  uint16_t len)
{
	int ret, i;
	uint16_t mlen = sizeof(reg) + len;
	uint8_t mbuf[sizeof(reg) + IQS5XX_WR_BYTES_MAX];

	if (len > IQS5XX_WR_BYTES_MAX) {
		return -EINVAL;
	}

	sys_put_be16(reg, mbuf);
	memcpy(mbuf + sizeof(reg), val, len);

	/*
	 * The first addressing attempt outside of a communication window fails
	 * and must be retried, after which the device clock stretches until it
	 * is available.
	 */
	for (i = 0; i < IQS5XX_NUM_RETRIES; i++) {
		ret = i2c_write_dt(client, mbuf, mlen);
		if (ret == 0) {
			return 0;
		}

		k_sleep(K_USEC(200));
	}

	if (ret >= 0) {
		ret = -EIO;
	}

	LOG_ERR("Failed to write to address 0x%04X: %d\n", reg, ret);

	return ret;
}

static int iqs5xx_write_word(const struct i2c_dt_spec *client, uint16_t reg, uint16_t val)
{
	uint16_t val_buf = sys_cpu_to_be16(val);

	return iqs5xx_write_burst(client, reg, &val_buf, sizeof(val_buf));
}

static int iqs5xx_write_byte(const struct i2c_dt_spec *client, uint16_t reg, uint8_t val)
{
	return iqs5xx_write_burst(client, reg, &val, sizeof(val));
}

static void setup_int(const struct device *dev, bool enable)
{
	const struct iqs5xx_dev_config *cfg = dev->config;
	gpio_flags_t flags = enable ? GPIO_INT_EDGE_RISING : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->rdy_gpio, flags);
}

static void iqs5xx_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct iqs5xx_data *data = CONTAINER_OF(cb, struct iqs5xx_data, gpio_cb);

	setup_int(data->dev, false);
	k_work_submit(&data->work);
}

static void iqs5xx_work_cb(struct k_work *work)
{
	struct iqs5xx_data *data = CONTAINER_OF(work, struct iqs5xx_data, work);

	const struct iqs5xx_dev_config *cfg = data->dev->config;

	iqs5xx_read_burst(&cfg->i2c, IQS5XX_GEST_EV0, &data->regmap, sizeof(data->regmap));

	iqs5xx_write_byte(&cfg->i2c, IQS5XX_END_COMM, 0);

	/* Small delay required as the iqs twiddles the rdy pin. */
	k_sleep(K_USEC(50));

	if (data->regmap.gesture_event[0]) {
		switch (data->regmap.gesture_event[0] & 0b00111111) {
		case IQS5XX_SINGLE_TAP:
			LOG_DBG("Single Tap");
			break;
		case IQS5XX_TAP_AND_HOLD:
			LOG_DBG("Tap And Hold");
			break;
		case IQS5XX_SWIPE_X_NEG:
			LOG_DBG("Swipe X negative");
			break;
		case IQS5XX_SWIPE_X_POS:
			LOG_DBG("Swipe X positive");
			break;
		case IQS5XX_SWIPE_Y_NEG:
			LOG_DBG("Swipe Y negative");
			break;
		case IQS5XX_SWIPE_Y_POS:
			LOG_DBG("Swipe Y positive");
			break;
		default:
			LOG_DBG("%d", data->regmap.gesture_event[0]);
		}
	}

	if (data->regmap.gesture_event[1]) {
		switch (data->regmap.gesture_event[1] & 0b00000111) {
		case IQS5XX_TWO_FINGER_TAP:
			LOG_DBG("Two Finger Tap");
			break;
		case IQS5XX_SCROLL:
			LOG_DBG("Scroll");
			break;
		case IQS5XX_ZOOM:
			LOG_DBG("Zoom");
			break;
		default:
			LOG_DBG("%d", data->regmap.gesture_event[1]);
		}
	}

	setup_int(data->dev, true);
}

int iqs5xx_init_interrupt(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;
	const struct iqs5xx_dev_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->rdy_gpio.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->rdy_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->rdy_gpio, GPIO_INPUT | cfg->rdy_gpio.dt_flags);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, iqs5xx_gpio_callback, BIT(cfg->rdy_gpio.pin));

	ret = gpio_add_callback(cfg->rdy_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;

	data->work.handler = iqs5xx_work_cb;
	return 0;
}

static bool iqs5xx_two_finger_tap(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;

	if (data->regmap.gesture_event[0] & IQS5XX_TWO_FINGER_TAP) {
		data->regmap.gesture_event[0] &= ~IQS5XX_TWO_FINGER_TAP;
		return true;
	}

	return false;
}

static bool iqs5xx_single_tap(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;

	if (data->regmap.gesture_event[0] & IQS5XX_SINGLE_TAP) {
		data->regmap.gesture_event[0] &= ~IQS5XX_SINGLE_TAP;
		return true;
	}

	return false;
}

static int16_t iqs5xx_x_position_abs(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;

	return sys_be16_to_cpu(data->regmap.touch_data[0].abs_x);
}

static int16_t iqs5xx_y_position_abs(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;

	return sys_be16_to_cpu(data->regmap.touch_data[0].abs_y);
}

static int16_t iqs5xx_x_position_rel(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;

	return sys_be16_to_cpu(data->regmap.rel_x);
}

static int16_t iqs5xx_y_position_rel(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;

	return sys_be16_to_cpu(data->regmap.rel_y);
}

static int iqs5xx_num_fingers(const struct device *dev)
{
	struct iqs5xx_data *data = dev->data;

	return data->regmap.num_fin;
}

static const struct touch_driver_api iqs5xx_driver_api = {
	.single_tap = &iqs5xx_single_tap,
	.two_finger_tap = &iqs5xx_two_finger_tap,
	.x_pos_abs = &iqs5xx_x_position_abs,
	.y_pos_abs = &iqs5xx_y_position_abs,
	.x_pos_rel = &iqs5xx_x_position_rel,
	.y_pos_rel = &iqs5xx_y_position_rel,
	.num_fingers = &iqs5xx_num_fingers,
};

static int iqs5xx_init(const struct device *dev)
{
	const struct iqs5xx_dev_config *cfg = dev->config;
	uint8_t value = 0;
	int ret = 0;
	int timeout = 0;
	uint16_t k;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	if (cfg->rdy_gpio.port) {
		ret = iqs5xx_init_interrupt(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize interrupt!");
			return ret;
		}
	} else {
		LOG_ERR("RDY GPIO not Ready");
	}

	iqs5xx_read_word(&cfg->i2c, IQS5XX_PROD_NUM, &k);

	/* Configure trackpad size */
	iqs5xx_write_byte(&cfg->i2c, IQS5XX_TOTAL_RX, CONFIG_IQS5XX_TOTAL_RX);
	iqs5xx_write_byte(&cfg->i2c, IQS5XX_TOTAL_TX, CONFIG_IQS5XX_TOTAL_TX);

	iqs5xx_write_byte(&cfg->i2c, IQS5XX_SINGLE_FINGER_GEST, IQS5XX_SINGLE_TAP);

	iqs5xx_write_byte(&cfg->i2c, IQS5XX_MULTI_FINGER_GEST, IQS5XX_TWO_FINGER_TAP);

	iqs5xx_write_byte(&cfg->i2c, IQS5XX_SYS_CTRL0, IQS5XX_ACK_RESET);

	iqs5xx_write_byte(&cfg->i2c, IQS5XX_SYS_CFG0,
			  IQS5XX_SETUP_COMPLETE | IQS5XX_WDT | IQS5XX_ALP_REATI | IQS5XX_REATI);

	iqs5xx_write_byte(&cfg->i2c, IQS5XX_SYS_CFG1,
			  IQS5XX_GESTURE_EVENT | IQS5XX_EVENT_MODE | IQS5XX_TP_EVENT);

	iqs5xx_write_word(&cfg->i2c, IQS5XX_X_RES, CONFIG_IQS5XX_X_RES);
	iqs5xx_write_word(&cfg->i2c, IQS5XX_Y_RES, CONFIG_IQS5XX_Y_RES);
	iqs5xx_write_byte(&cfg->i2c, IQS5XX_END_COMM, 0);

	LOG_INF("IQS Driver Probed. Product Number: 0x%x", k);

	setup_int(dev, true);

	return 0;
}

#define IQS5XX_DEFINE(inst) \
	static struct iqs5xx_data iqs5xx_data_##inst;\
					\
	static const struct iqs5xx_dev_config iqs5xx_config_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst),\
		.rdy_gpio = GPIO_DT_SPEC_INST_GET(inst, rdy_gpios),\
	};\
				 \
	DEVICE_DT_INST_DEFINE(inst, iqs5xx_init, NULL, &iqs5xx_data_##inst, \
		&iqs5xx_config_##inst, POST_KERNEL, CONFIG_TOUCH_INIT_PRIORITY, \
		&iqs5xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IQS5XX_DEFINE)
