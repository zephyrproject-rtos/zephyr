/*
 * SPDX-FileCopyrightText: Copyright (c) 2026, Institute of Embedded Systems ZHAW
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tsc2007

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/util.h>

#include <errno.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tsc2007, CONFIG_INPUT_LOG_LEVEL);

#define TS_POLL_PERIOD 50 /* ms */

/* converter function */
#define TSC2007_MEASURE_TEMP_0         (0x0 << 4)
#define TSC2007_MEASURE_AUX            (0x2 << 4)
#define TSC2007_MEASURE_TEMP_1         (0x4 << 4)
#define TSC2007_ACTIVATE_X_DRIVERS     (0x8 << 4)
#define TSC2007_ACTIVATE_Y_DRIVERS     (0x9 << 4)
#define TSC2007_ACTIVATE_YP_XN_DRIVERS (0xa << 4)
#define TSC2007_SETUP_COMMAND          (0xb << 4)
#define TSC2007_MEASURE_X_POSITION     (0xc << 4)
#define TSC2007_MEASURE_Y_POSITION     (0xd << 4)
#define TSC2007_MEASURE_Z1_POSITION    (0xe << 4)
#define TSC2007_MEASURE_Z2_POSITION    (0xf << 4)

/* power modes */
#define TSC2007_POWER_DOWN_IRQ_EN    (0x0 << 2)
#define TSC2007_ADC_ON_IRQ_DISABLED0 (0x1 << 2)
#define TSC2007_ADC_OFF_IRQ_ENABLED  (0x2 << 2)
#define TSC2007_ADC_ON_IRQ_DISABLED1 (0x3 << 2)

/* resolution */
#define TSC2007_12BIT (0x0 << 1)
#define TSC2007_8BIT  (0x1 << 1)

#define MAX_12BIT ((1 << 12) - 1)

#define ADC_ON_12BIT (TSC2007_12BIT | TSC2007_ADC_ON_IRQ_DISABLED0)

#define TOUCH_STATE_ACTIVE 1

static const uint8_t cmd_read_x = (ADC_ON_12BIT | TSC2007_MEASURE_X_POSITION);
static const uint8_t cmd_read_y = (ADC_ON_12BIT | TSC2007_MEASURE_Y_POSITION);
static const uint8_t cmd_read_z1 = (ADC_ON_12BIT | TSC2007_MEASURE_Z1_POSITION);
static const uint8_t cmd_read_z2 = (ADC_ON_12BIT | TSC2007_MEASURE_Z2_POSITION);

static const uint8_t cmd_powerdown = (TSC2007_12BIT | TSC2007_POWER_DOWN_IRQ_EN);
static const uint8_t cmd_setup =
	(TSC2007_SETUP_COMMAND | TSC2007_POWER_DOWN_IRQ_EN | TSC2007_12BIT);

struct ts_event {
	uint32_t x;
	uint32_t y;
	uint32_t z1;
	uint32_t z2;
};

struct tsc2007_config {
	struct input_touchscreen_common_config common;
	struct i2c_dt_spec bus;
	struct gpio_dt_spec int_gpio;
	int raw_x_min;
	int raw_y_min;
	uint32_t raw_x_max;
	uint32_t raw_y_max;
};

struct tsc2007_data {
	const struct device *dev;
	struct k_work_delayable work;
	struct gpio_callback int_gpio_cb;
	uint32_t touch_x;
	uint32_t touch_y;
	bool pendown;
#ifdef CONFIG_PM
	struct pm_notifier pm_notifier_handle;
#endif
};

INPUT_TOUCH_STRUCT_CHECK(struct tsc2007_config);

static int tsc2007_command_setup(const struct device *dev)
{
	int ret = 0;
	uint8_t rx[2];
	const struct tsc2007_config *config = dev->config;

	ret = i2c_write_read_dt(&config->bus, &cmd_setup, 1, &rx[0], sizeof(rx));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int tsc2007_powerdown(const struct device *dev)
{
	int ret = 0;

	const struct tsc2007_config *config = dev->config;

	ret = i2c_write_dt(&config->bus, &cmd_powerdown, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void tsc2007_report_touch(const struct device *dev)
{
	const struct tsc2007_config *config = dev->config;
	const struct input_touchscreen_common_config *common = &config->common;
	struct tsc2007_data *data = dev->data;
	uint32_t x = data->touch_x;
	uint32_t y = data->touch_y;

	if (common->screen_width > 0 && common->screen_height > 0) {
		x = ((data->touch_x - config->raw_x_min) * common->screen_width) /
		    (config->raw_x_max - config->raw_x_min);
		y = ((data->touch_y - config->raw_y_min) * common->screen_height) /
		    (config->raw_y_max - config->raw_y_min);

		x = clamp(x, 0, common->screen_width - 1);
		y = clamp(y, 0, common->screen_height - 1);
	}

	input_touchscreen_report_pos(dev, x, y, K_FOREVER);
}

static int tsc2007_read_adc(const struct device *dev, uint8_t cmd, uint32_t *value)
{
	const struct tsc2007_config *config = dev->config;
	uint8_t rx[2];
	int ret = 0;

	if (!dev) {
		return -EINVAL;
	}
	if (!value) {
		return -EINVAL;
	}

	ret = i2c_write_read_dt(&config->bus, &cmd, 1, &rx[0], sizeof(rx));
	if (ret < 0) {
		return -EIO;
	}

	/*
	 * First byte of serial data (D11-D4, MSB first) followed by a second byte
	 * of serial data (D3-D0 followed by four 0 bits). Shift the values to
	 * store them in a uint16_t.
	 */
	*value = ((uint16_t)rx[0] << 4) | ((uint16_t)rx[1] >> 4);

	return 0;
}

static int tsc2007_read_values(const struct device *dev, struct ts_event *touch)
{
	int ret = 0;

	/* y- still on; turn on only y+ (and ADC) */
	ret = tsc2007_read_adc(dev, cmd_read_y, &touch->y);

	if (ret < 0) {
		LOG_ERR("Could not read y value (%d)", ret);
		return -EIO;
	}

	/* turn y- off; x+ on, then leave in lowpower */
	ret = tsc2007_read_adc(dev, cmd_read_x, &touch->x);
	if (ret < 0) {
		LOG_ERR("Could not read x value (%d)", ret);
		return -EIO;
	}

	/* turn y+ off, x- on, we'll use formula #1 */
	ret = tsc2007_read_adc(dev, cmd_read_z1, &touch->z1);
	if (ret < 0) {
		LOG_ERR("Could not read z1 values (%d)", ret);
		return -EIO;
	}

	ret = tsc2007_read_adc(dev, cmd_read_z2, &touch->z2);
	if (ret < 0) {
		LOG_ERR("Could not read z2 values (%d)", ret);
		return -EIO;
	}

	/* Prepare for next touch reading - power down ADC, enable PENIRQ */
	ret = tsc2007_powerdown(dev);
	if (ret < 0) {
		LOG_ERR("Could not read power (%d)", ret);
		return -EIO;
	}

	return 0;
}

static uint32_t tsc2007_calculate_pressure(struct ts_event *touch)
{
	uint32_t ret = 0;

	if (!touch) {
		return;
	}

	if (touch->x == MAX_12BIT) {
		touch->x = 0UL;
	}

	if (touch->x && touch->z1) {
		ret = (uint32_t)touch->z2 - (uint32_t)touch->z1;
		ret *= (uint32_t)touch->x;
		ret *= 400UL;
		ret /= (uint32_t)touch->z1;
		ret = (ret + 2047UL) >> 12;
	}

	return ret;
}

static void tsc2007_work_handler(struct k_work *work)
{
	if (!work) {
		return;
	}

	struct tsc2007_data *data = CONTAINER_OF(work, struct tsc2007_data, work.work);
	const struct tsc2007_config *config = data->dev->config;
	struct ts_event touch;
	uint32_t pressure;
	int ret = 0;

	/* Interrupt pin is set to GPIO_ACTIVE_LOW in the DTS file */
	if (!gpio_pin_get_dt(&config->int_gpio)) {
		input_report_key(data->dev, INPUT_BTN_TOUCH, !TOUCH_STATE_ACTIVE, true, K_FOREVER);
		data->pendown = false;
		goto out;
	}

	ret = tsc2007_read_values(data->dev, &touch);
	if (ret < 0) {
		LOG_ERR("touch read error %d", ret);
		return;
	}

	data->touch_x = touch.x;
	data->touch_y = touch.y;

	pressure = tsc2007_calculate_pressure(&touch);

	if (pressure > MAX_12BIT) {
		/*
		 * Sample found inconsistent by debouncing or pressure is beyond the maximum.
		 * Don't report it to user space, repeat at least once more the measurement.
		 */
		LOG_DBG("ignore pressure %d", pressure);
		goto out;
	}

	if (pressure > 0) {
		tsc2007_report_touch(data->dev);
		if (!data->pendown) {
			input_report_key(data->dev, INPUT_BTN_TOUCH, TOUCH_STATE_ACTIVE, true,
					 K_FOREVER);
			data->pendown = true;
		}
	} else if (data->pendown) {
		input_report_key(data->dev, INPUT_BTN_TOUCH, !TOUCH_STATE_ACTIVE, true, K_FOREVER);
		data->pendown = false;
	}

out:
	if (data->pendown) {
		k_work_schedule(&data->work, K_MSEC(TS_POLL_PERIOD));
	} else {
		gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
	}
}

static void tsc2007_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
				      uint32_t pins)
{
	struct tsc2007_data *data = CONTAINER_OF(cb, struct tsc2007_data, int_gpio_cb);
	const struct tsc2007_config *config = data->dev->config;

	if (!dev) {
		return;
	}
	if (!cb) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

	k_work_schedule(&data->work, K_MSEC(0));
}

#if CONFIG_PM
static void tsc2007_pm_state_exit(const struct device *dev, enum pm_state state)
{
	switch (state) {
	case PM_STATE_STANDBY:
		/* Reconfigure the GPIO interrupt pin on exit from
		 * certain low power state as we might lose the GPIO state.
		 */
		const struct tsc2007_config *config = dev->config;
		int ret = 0;

		ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure GPIO interrupt (%d)", ret);
			return;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
		if (ret < 0) {
			LOG_ERR("Could not configure GPIO interrupt (%d)", ret);
			return;
		}

		break;
	default:
		break;
	}
}
#endif /* CONFIG_PM */

static int tsc2007_init(const struct device *dev)
{
	const struct tsc2007_config *config = dev->config;
	struct tsc2007_data *data = dev->data;
	int ret = 0;

	if (!dev) {
		return -EINVAL;
	}

	data->dev = dev;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->bus.bus);
		return -ENODEV;
	}

	k_work_init_delayable(&data->work, tsc2007_work_handler);

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->int_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Could not configure GPIO interrupt (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, tsc2007_interrupt_handler,
			   (1UL << config->int_gpio.pin));

	ret = gpio_add_callback_dt(&config->int_gpio, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set GPIO callback (%d)", ret);
		return ret;
	}

#if CONFIG_PM
	/* We need to reconfigure the interrupt GPIO when waking up from
	 * certain low power modes.
	 */
	pm_notifier_register(&data->pm_notifier_handle);
#endif

	k_msleep(2);

	tsc2007_command_setup(dev);
	tsc2007_powerdown(dev);

	return 0;
}

#if CONFIG_PM
#define TSC2007_PM_NOTIFIER_FUNCS(n)                                                               \
	static void tsc2007_##n##_pm_state_exit(enum pm_state state)                               \
	{                                                                                          \
		tsc2007_pm_state_exit(DEVICE_DT_INST_GET(n), state);                               \
	}

#define TSC2007_PM_NOTIFIER(n)                                                                     \
	.pm_notifier_handle = {                                                                    \
		.state_exit = tsc2007_##n##_pm_state_exit,                                         \
	},
#else
#define TSC2007_PM_NOTIFIER_FUNCS(n)
#define TSC2007_PM_NOTIFIER(n)
#endif /* CONFIG_PM */

#define TSC2007_DEFINE(index)                                                                      \
	BUILD_ASSERT(DT_INST_PROP_OR(index, raw_x_max, 4096) >                                     \
			     DT_INST_PROP_OR(index, raw_x_min, 0),                                 \
		     "raw-x-max should be larger than raw-x-min");                                 \
	BUILD_ASSERT(DT_INST_PROP_OR(index, raw_y_max, 4096) >                                     \
			     DT_INST_PROP_OR(index, raw_y_min, 0),                                 \
		     "raw-y-max should be larger than raw-y-min");                                 \
	static const struct tsc2007_config tsc2007_config_##index = {                              \
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(index),                           \
		.bus = I2C_DT_SPEC_INST_GET(index),                                                \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),                               \
		.raw_x_min = DT_INST_PROP_OR(index, raw_x_min, 0),                                 \
		.raw_x_max = DT_INST_PROP_OR(index, raw_x_max, 4096),                              \
		.raw_y_min = DT_INST_PROP_OR(index, raw_y_min, 0),                                 \
		.raw_y_max = DT_INST_PROP_OR(index, raw_y_max, 4096),                              \
	};                                                                                         \
	TSC2007_PM_NOTIFIER_FUNCS(index)                                                           \
	static struct tsc2007_data tsc2007_data_##index;                                           \
	DEVICE_DT_INST_DEFINE(index, tsc2007_init, NULL, &tsc2007_data_##index,                    \
			      &tsc2007_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TSC2007_DEFINE)
