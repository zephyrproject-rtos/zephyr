/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT chipsourcetek_axs5106l

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(axs5106l, CONFIG_INPUT_LOG_LEVEL);

/*
 * The touch report starts at register 0x01: gesture, number of touch points,
 * then 6 bytes per touch point starting with the big endian X and Y positions.
 */
#define AXS5106L_REG_TOUCH_DATA 0x01U
#define AXS5106L_REPORT_LEN     14U
#define AXS5106L_COUNT_POS      1U
#define AXS5106L_X_POS          2U
#define AXS5106L_Y_POS          4U
#define AXS5106L_COUNT_MASK     GENMASK(3, 0)
#define AXS5106L_POS_MASK       GENMASK(11, 0)

/* Minimum delay between the register address write and the data read */
#define AXS5106L_READ_DELAY_US 50U

/* Reset timings from the vendor reference code */
#define AXS5106L_RESET_ASSERT_MS 200
#define AXS5106L_RESET_DELAY_MS  300

struct axs5106l_config {
	struct input_touchscreen_common_config common;
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	struct gpio_dt_spec reset_gpio;
};

INPUT_TOUCH_STRUCT_CHECK(struct axs5106l_config);

struct axs5106l_data {
	const struct device *dev;
	struct k_work_delayable work;
	struct gpio_callback int_gpio_cb;
	bool pressed;
	bool read_failed;
};

static int axs5106l_read_report(const struct device *dev, uint8_t *buf)
{
	const struct axs5106l_config *config = dev->config;
	uint8_t reg = AXS5106L_REG_TOUCH_DATA;
	int ret;

	/*
	 * The controller does not support repeated start conditions: the register
	 * address is written in its own transfer, followed by a short delay.
	 */
	ret = i2c_write_dt(&config->i2c, &reg, sizeof(reg));
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(AXS5106L_READ_DELAY_US);

	return i2c_read_dt(&config->i2c, buf, AXS5106L_REPORT_LEN);
}

static void axs5106l_report_release(const struct device *dev)
{
	struct axs5106l_data *data = dev->data;

	if (data->pressed) {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		data->pressed = false;
	}
}

static int axs5106l_process(const struct device *dev)
{
	struct axs5106l_data *data = dev->data;
	uint8_t buf[AXS5106L_REPORT_LEN];
	uint16_t x;
	uint16_t y;
	int ret;

	ret = axs5106l_read_report(dev, buf);
	if (ret < 0) {
		/*
		 * Only log the first of consecutive failures, as the read may be
		 * retried every period.
		 */
		if (!data->read_failed) {
			LOG_ERR("Could not read touch data: %d", ret);
			data->read_failed = true;
		}

		/* Do not leave the panel reported as touched */
		axs5106l_report_release(dev);

		return ret;
	}

	data->read_failed = false;

	if (FIELD_GET(AXS5106L_COUNT_MASK, buf[AXS5106L_COUNT_POS]) == 0U) {
		axs5106l_report_release(dev);

		return 0;
	}

	x = sys_get_be16(&buf[AXS5106L_X_POS]) & AXS5106L_POS_MASK;
	y = sys_get_be16(&buf[AXS5106L_Y_POS]) & AXS5106L_POS_MASK;

	LOG_DBG("x: %u, y: %u", x, y);

	input_touchscreen_report_pos(dev, x, y, K_FOREVER);
	input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	data->pressed = true;

	return 0;
}

static void axs5106l_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct axs5106l_data *data = CONTAINER_OF(dwork, struct axs5106l_data, work);
	const struct axs5106l_config *config = data->dev->config;

	(void)axs5106l_process(data->dev);

	/*
	 * Keep reading the touch data while the panel is touched, so that movement
	 * and release are reported, and while the interrupt line stays asserted, as
	 * the controller only releases it once the touch data has been read. Read
	 * it all the time without an interrupt GPIO.
	 */
	if (data->pressed || (config->int_gpio.port == NULL) ||
	    (gpio_pin_get_dt(&config->int_gpio) > 0)) {
		k_work_reschedule(&data->work, K_MSEC(CONFIG_INPUT_AXS5106L_PERIOD_MS));
	}
}

static void axs5106l_isr_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct axs5106l_data *data = CONTAINER_OF(cb, struct axs5106l_data, int_gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_reschedule(&data->work, K_NO_WAIT);
}

static int axs5106l_reset(const struct device *dev)
{
	const struct axs5106l_config *config = dev->config;
	int ret;

	if (config->reset_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->reset_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure reset GPIO: %d", ret);
		return ret;
	}

	k_msleep(AXS5106L_RESET_ASSERT_MS);

	ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Could not release reset: %d", ret);
		return ret;
	}

	return 0;
}

static int axs5106l_init_interrupt(const struct device *dev)
{
	const struct axs5106l_config *config = dev->config;
	struct axs5106l_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->int_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, axs5106l_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback_dt(&config->int_gpio, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not add interrupt GPIO callback: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt: %d", ret);
		return ret;
	}

	return 0;
}

static int axs5106l_init(const struct device *dev)
{
	const struct axs5106l_config *config = dev->config;
	struct axs5106l_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c.bus);
		return -ENODEV;
	}

	data->dev = dev;
	k_work_init_delayable(&data->work, axs5106l_work_handler);

	ret = axs5106l_reset(dev);
	if (ret < 0) {
		return ret;
	}

	if (config->int_gpio.port != NULL) {
		ret = axs5106l_init_interrupt(dev);
		if (ret < 0) {
			return ret;
		}
	}

	/*
	 * Read the touch data once the controller is ready after its reset. This
	 * also releases the interrupt line if it was asserted before the interrupt
	 * was enabled, for example when the controller was reset by the display.
	 */
	k_work_reschedule(&data->work, K_MSEC(AXS5106L_RESET_DELAY_MS));

	return 0;
}

#define AXS5106L_DEFINE(inst)                                                                      \
	static const struct axs5106l_config axs5106l_config_##inst = {                             \
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(inst),                            \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),                        \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
	};                                                                                         \
	static struct axs5106l_data axs5106l_data_##inst;                                          \
	DEVICE_DT_INST_DEFINE(inst, axs5106l_init, NULL, &axs5106l_data_##inst,                    \
			      &axs5106l_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(AXS5106L_DEFINE)
