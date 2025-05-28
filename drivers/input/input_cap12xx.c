/*
 * Copyright (c) 2022 Keiya Nobuta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_cap12xx

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/math/ilog2.h>
LOG_MODULE_REGISTER(cap12xx, CONFIG_INPUT_LOG_LEVEL);

#define REG_MAIN_CONTROL        0x00
#define MAIN_CONTROL_GAIN_MASK  GENMASK(7, 6)
#define MAIN_CONTROL_GAIN_SHIFT 6

#define CONTROL_INT 0x01

#define REG_INPUT_STATUS 0x03

#define REG_SENSITIVITY_CONTROL 0x1F
#define DELTA_SENSE_BITS        3
#define DELTA_SENSE_SHIFT       4
#define DELTA_SENSE_MASK        GENMASK(6, 4)
#define DELTA_SENSE_MAX         GENMASK(DELTA_SENSE_BITS - 1, 0)

#define REG_INTERRUPT_ENABLE 0x27
#define INTERRUPT_ENABLE     0xFF
#define INTERRUPT_DISABLE    0x00

#define REG_REPEAT_ENABLE 0x28
#define REPEAT_ENABLE     0xFF
#define REPEAT_DISABLE    0x00

#define REG_SIGNAL_GUARD_ENABLE 0x29

#define REG_CALIB_SENSITIVITY_CONFIG1 0x80
#define REG_CALIB_SENSITIVITY_CONFIG2 0x81
#define CALSENS_BITS                  2
#define NUM_CALSENS_PER_REG           4
#define MAX_CALSENS_GAIN              4

struct cap12xx_config {
	struct i2c_dt_spec i2c;
	const uint8_t input_channels;
	const uint16_t *input_codes;
	struct gpio_dt_spec *int_gpio;
	bool repeat;
	const uint16_t poll_interval_ms;
	const uint8_t sensor_gain;
	const uint8_t sensitivity_delta_sense;
	const uint8_t *signal_guard;
	const uint8_t *calib_sensitivity;
};

struct cap12xx_data {
	const struct device *dev;
	struct k_work work;
	uint8_t prev_input_state;
	struct gpio_callback int_gpio_cb;
	struct k_timer poll_timer;
};

static int cap12xx_clear_interrupt(const struct i2c_dt_spec *i2c)
{
	uint8_t ctrl;
	int ret;

	ret = i2c_reg_read_byte_dt(i2c, REG_MAIN_CONTROL, &ctrl);
	if (ret < 0) {
		return ret;
	}

	ctrl = ctrl & ~CONTROL_INT;
	return i2c_reg_write_byte_dt(i2c, REG_MAIN_CONTROL, ctrl);
}

static int cap12xx_enable_interrupt(const struct i2c_dt_spec *i2c, bool enable)
{
	uint8_t intr = enable ? INTERRUPT_ENABLE : INTERRUPT_DISABLE;

	return i2c_reg_write_byte_dt(i2c, REG_INTERRUPT_ENABLE, intr);
}

static int cap12xx_set_sensor_gain(const struct i2c_dt_spec *i2c, uint8_t gain)
{
	uint8_t regval = gain << MAIN_CONTROL_GAIN_SHIFT;

	return i2c_reg_update_byte_dt(i2c, REG_MAIN_CONTROL, MAIN_CONTROL_GAIN_MASK, regval);
}

static int cap12xx_set_sensitivity(const struct i2c_dt_spec *i2c, uint8_t sensitivity)
{
	uint8_t regval = sensitivity << DELTA_SENSE_SHIFT;

	return i2c_reg_update_byte_dt(i2c, REG_SENSITIVITY_CONTROL, DELTA_SENSE_MASK, regval);
}

static int cap12xx_set_calsens(const struct i2c_dt_spec *i2c, const uint8_t *calsens,
			       uint8_t channels)
{
	int ret;
	uint8_t regval;

	for (uint8_t i = 0; i < channels; i += NUM_CALSENS_PER_REG) {
		regval = 0;
		for (uint8_t j = 0; j < NUM_CALSENS_PER_REG && i + j < channels; j++) {
			if (calsens[i + j] > MAX_CALSENS_GAIN) {
				return -EINVAL;
			}
			/* Convert the enumerated sensitivity to the corresponding register value */
			regval |= (ilog2(calsens[i + j]) << (CALSENS_BITS * j));
		}
		if (i == 0) {
			ret = i2c_reg_write_byte_dt(i2c, REG_CALIB_SENSITIVITY_CONFIG1, regval);
		} else {
			ret = i2c_reg_write_byte_dt(i2c, REG_CALIB_SENSITIVITY_CONFIG2, regval);
		}

		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int cap12xx_process(const struct device *dev)
{
	const struct cap12xx_config *config = dev->config;
	struct cap12xx_data *data = dev->data;
	int ret;
	uint8_t input_state;

	/*
	 * Clear INT bit to clear SENSOR INPUT STATUS bits.
	 * Note that this is also required in polling mode.
	 */
	ret = cap12xx_clear_interrupt(&config->i2c);

	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_read_byte_dt(&config->i2c, REG_INPUT_STATUS, &input_state);
	if (ret < 0) {
		return ret;
	}

	if (config->int_gpio == NULL) {
		if (data->prev_input_state == input_state) {
			return 0;
		}
	}

	for (uint8_t i = 0; i < config->input_channels; i++) {
		if (input_state & BIT(i)) {
			input_report_key(dev, config->input_codes[i], input_state & BIT(i), true,
					 K_FOREVER);
		} else if (data->prev_input_state & BIT(i)) {
			input_report_key(dev, config->input_codes[i], input_state & BIT(i), true,
					 K_FOREVER);
		}
	}
	data->prev_input_state = input_state;

	return 0;
}

static void cap12xx_work_handler(struct k_work *work)
{
	struct cap12xx_data *data = CONTAINER_OF(work, struct cap12xx_data, work);

	cap12xx_process(data->dev);
}

static void cap12xx_timer_handler(struct k_timer *poll_timer)
{
	struct cap12xx_data *data = CONTAINER_OF(poll_timer, struct cap12xx_data, poll_timer);

	k_work_submit(&data->work);
}

static void cap12xx_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct cap12xx_data *data = CONTAINER_OF(cb, struct cap12xx_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static int cap12xx_init(const struct device *dev)
{
	const struct cap12xx_config *config = dev->config;
	struct cap12xx_data *data = dev->data;
	uint8_t guarded_channels = 0;
	int ret;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->work, cap12xx_work_handler);

	for (uint8_t i = 0; i < config->input_channels; i++) {
		if (config->signal_guard[i]) {
			guarded_channels |= BIT(i);
		}
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, REG_SIGNAL_GUARD_ENABLE, guarded_channels);
	if (ret < 0) {
		LOG_ERR("Could not set guarded channels");
		return ret;
	}
	ret = cap12xx_set_calsens(&config->i2c, config->calib_sensitivity, config->input_channels);
	if (ret < 0) {
		LOG_ERR("Could not set calibration sensitivities");
		return ret;
	}
	/* Convert the enumerated gain to the corresponding register value */
	ret = cap12xx_set_sensor_gain(&config->i2c, ilog2(config->sensor_gain));
	if (ret < 0) {
		LOG_ERR("Could not set analog gain");
		return ret;
	}
	/* Convert the enumerated sensitivity to the corresponding register value,
	 * which is in reverse order
	 */
	ret = cap12xx_set_sensitivity(&config->i2c,
				      DELTA_SENSE_MAX - ilog2(config->sensitivity_delta_sense));
	if (ret < 0) {
		LOG_ERR("Could not set sensitivity");
		return ret;
	}
	if (config->int_gpio == NULL) {
		LOG_DBG("cap12xx driver in polling mode");
		k_timer_init(&data->poll_timer, cap12xx_timer_handler, NULL);
		ret = cap12xx_enable_interrupt(&config->i2c, true);
		if (ret < 0) {
			LOG_ERR("Could not configure interrupt");
			return ret;
		}
		k_timer_start(&data->poll_timer, K_MSEC(config->poll_interval_ms),
			      K_MSEC(config->poll_interval_ms));
	} else {
		LOG_DBG("cap12xx driver in interrupt mode");
		if (!gpio_is_ready_dt(config->int_gpio)) {
			LOG_ERR("Interrupt GPIO controller device not ready (missing device tree "
				"node?)");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(config->int_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure interrupt GPIO pin");
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure interrupt GPIO interrupt");
			return ret;
		}

		gpio_init_callback(&data->int_gpio_cb, cap12xx_isr_handler,
				   BIT(config->int_gpio->pin));

		ret = gpio_add_callback_dt(config->int_gpio, &data->int_gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}

		ret = cap12xx_clear_interrupt(&config->i2c);
		if (ret < 0) {
			LOG_ERR("Could not clear interrupt");
			return ret;
		}
		ret = cap12xx_enable_interrupt(&config->i2c, true);
		if (ret < 0) {
			LOG_ERR("Could not configure interrupt");
			return ret;
		}
		if (config->repeat) {
			ret = i2c_reg_write_byte_dt(&config->i2c, REG_REPEAT_ENABLE, REPEAT_ENABLE);
			if (ret < 0) {
				LOG_ERR("Could not disable repeated interrupts");
				return ret;
			}
			LOG_DBG("cap12xx enabled repeated interrupts");
		} else {
			ret = i2c_reg_write_byte_dt(&config->i2c, REG_REPEAT_ENABLE,
						    REPEAT_DISABLE);
			if (ret < 0) {
				LOG_ERR("Could not enable repeated interrupts");
				return ret;
			}
			LOG_DBG("cap12xx disabled repeated interrupts");
		}
	}
	LOG_DBG("%d channels configured", config->input_channels);
	return 0;
}

#define CAP12XX_INIT(index)                                                                        \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(index, int_gpios), (                                      \
	static struct gpio_dt_spec cap12xx_int_gpio_##index =                                      \
		GPIO_DT_SPEC_INST_GET(index, int_gpios);))                                         \
	static const uint16_t cap12xx_input_codes_##index[] = DT_INST_PROP(index, input_codes);    \
	static const uint8_t cap12xx_signal_guard_##index[] =                                      \
		DT_INST_PROP(index, signal_guard);                                                 \
	static const uint8_t cap12xx_calib_sensitivity_##index[] =                                 \
		DT_INST_PROP(index, calib_sensitivity);                                            \
	static const struct cap12xx_config cap12xx_config_##index = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.input_channels = DT_INST_PROP_LEN(index, input_codes),                            \
		.input_codes = cap12xx_input_codes_##index,                                        \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(index, int_gpios), (                              \
				.int_gpio = &cap12xx_int_gpio_##index,))                           \
		.repeat = DT_INST_PROP(index, repeat),                                             \
		.poll_interval_ms = DT_INST_PROP(index, poll_interval_ms),                         \
		.sensor_gain = DT_INST_PROP(index, sensor_gain),                                   \
		.sensitivity_delta_sense = DT_INST_PROP(index, sensitivity_delta_sense),           \
		.signal_guard = cap12xx_signal_guard_##index,                                      \
		.calib_sensitivity = cap12xx_calib_sensitivity_##index};                           \
	static struct cap12xx_data cap12xx_data_##index;                                           \
	DEVICE_DT_INST_DEFINE(index, cap12xx_init, NULL, &cap12xx_data_##index,                    \
			      &cap12xx_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(CAP12XX_INIT)
