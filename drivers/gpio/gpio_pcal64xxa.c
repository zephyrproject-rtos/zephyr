/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pcal64xxa, CONFIG_GPIO_LOG_LEVEL);

enum pcal6408a_register {
	PCAL6408A_REG_INPUT_PORT = 0x00,
	PCAL6408A_REG_OUTPUT_PORT = 0x01,
	PCAL6408A_REG_POLARITY_INVERSION = 0x02,
	PCAL6408A_REG_CONFIGURATION = 0x03,
	PCAL6408A_REG_OUTPUT_DRIVE_STRENGTH_0 = 0x40,
	PCAL6408A_REG_OUTPUT_DRIVE_STRENGTH_1 = 0x41,
	PCAL6408A_REG_INPUT_LATCH = 0x42,
	PCAL6408A_REG_PULL_UP_DOWN_ENABLE = 0x43,
	PCAL6408A_REG_PULL_UP_DOWN_SELECT = 0x44,
	PCAL6408A_REG_INTERRUPT_MASK = 0x45,
	PCAL6408A_REG_INTERRUPT_STATUS = 0x46,
	PCAL6408A_REG_OUTPUT_PORT_CONFIGURATION = 0x4f,
};

enum pcal6416a_register {
	PCAL6416A_REG_INPUT_PORT_0 = 0x00,
	PCAL6416A_REG_INPUT_PORT_1 = 0x01,
	PCAL6416A_REG_OUTPUT_PORT_0 = 0x02,
	PCAL6416A_REG_OUTPUT_PORT_1 = 0x03,
	PCAL6416A_REG_POLARITY_INVERSION_0 = 0x04,
	PCAL6416A_REG_POLARITY_INVERSION_1 = 0x05,
	PCAL6416A_REG_CONFIGURATION_0 = 0x06,
	PCAL6416A_REG_CONFIGURATION_1 = 0x07,
	PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_0_0 = 0x40,
	PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_0_1 = 0x41,
	PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_1_0 = 0x42,
	PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_1_1 = 0x43,
	PCAL6416A_REG_INPUT_LATCH_0 = 0x44,
	PCAL6416A_REG_INPUT_LATCH_1 = 0x45,
	PCAL6416A_REG_PULL_UP_DOWN_ENABLE_0 = 0x46,
	PCAL6416A_REG_PULL_UP_DOWN_ENABLE_1 = 0x47,
	PCAL6416A_REG_PULL_UP_DOWN_SELECT_0 = 0x48,
	PCAL6416A_REG_PULL_UP_DOWN_SELECT_1 = 0x49,
	PCAL6416A_REG_INTERRUPT_MASK_0 = 0x4A,
	PCAL6416A_REG_INTERRUPT_MASK_1 = 0x4B,
	PCAL6416A_REG_INTERRUPT_STATUS_0 = 0x4C,
	PCAL6416A_REG_INTERRUPT_STATUS_1 = 0x4D,
	PCAL6416A_REG_OUTPUT_PORT_CONFIGURATION = 0x4F,
};

struct pcal64xxa_pins_cfg {
	uint16_t configured_as_inputs;
	uint16_t outputs_high;
	uint16_t pull_ups_selected;
	uint16_t pulls_enabled;
};

struct pcal64xxa_triggers {
	uint16_t masked;
	uint16_t dual_edge;
	uint16_t on_low;
};

struct pcal64xxa_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	sys_slist_t callbacks;
	struct k_sem lock;
	struct k_work work;
	const struct device *dev;
	struct gpio_callback int_gpio_cb;
	struct pcal64xxa_pins_cfg pins_cfg;
	struct pcal64xxa_triggers triggers;
	uint16_t input_port_last;
};

typedef int (*pcal64xxa_pins_cfg_apply)(const struct i2c_dt_spec *i2c,
					const struct pcal64xxa_pins_cfg *pins_cfg);
typedef int (*pcal64xxa_triggers_apply)(const struct i2c_dt_spec *i2c,
					const struct pcal64xxa_triggers *triggers);
typedef int (*pcal64xxa_reset_state_apply)(const struct i2c_dt_spec *i2c);
typedef int (*pcal64xxa_inputs_read)(const struct i2c_dt_spec *i2c, uint16_t *int_sources,
				     uint16_t *input_port);
typedef int (*pcal64xxa_outputs_write)(const struct i2c_dt_spec *i2c, uint16_t outputs);

struct pcal64xxa_chip_api {
	pcal64xxa_pins_cfg_apply pins_cfg_apply;
	pcal64xxa_triggers_apply triggers_apply;
	pcal64xxa_inputs_read inputs_read;
	pcal64xxa_outputs_write outputs_write;
	pcal64xxa_reset_state_apply reset_state_apply;
};

struct pcal64xxa_drv_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	struct i2c_dt_spec i2c;
	uint8_t ngpios;
	const struct gpio_dt_spec gpio_reset;
	const struct gpio_dt_spec gpio_interrupt;
	const struct pcal64xxa_chip_api *chip_api;
};

static int pcal64xxa_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct pcal64xxa_drv_data *drv_data = dev->data;
	const struct pcal64xxa_drv_cfg *drv_cfg = dev->config;
	struct pcal64xxa_pins_cfg pins_cfg;
	gpio_flags_t flags_io;
	int rc;

	LOG_DBG("configure pin %i with flags 0x%08X", pin, flags);

	/* This device does not support open-source outputs, and open-drain
	 * outputs can be only configured port-wise.
	 */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* Pins in this device can be either inputs or outputs and cannot be
	 * completely disconnected.
	 */
	flags_io = (flags & (GPIO_INPUT | GPIO_OUTPUT));
	if (flags_io == (GPIO_INPUT | GPIO_OUTPUT) || flags_io == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	pins_cfg = drv_data->pins_cfg;

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		if ((flags & GPIO_PULL_UP) != 0) {
			pins_cfg.pull_ups_selected |= BIT(pin);
		} else {
			pins_cfg.pull_ups_selected &= ~BIT(pin);
		}

		pins_cfg.pulls_enabled |= BIT(pin);
	} else {
		pins_cfg.pulls_enabled &= ~BIT(pin);
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			pins_cfg.outputs_high &= ~BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			pins_cfg.outputs_high |= BIT(pin);
		}

		pins_cfg.configured_as_inputs &= ~BIT(pin);
	} else {
		pins_cfg.configured_as_inputs |= BIT(pin);
	}

	rc = drv_cfg->chip_api->pins_cfg_apply(&drv_cfg->i2c, &pins_cfg);
	if (rc == 0) {
		drv_data->pins_cfg = pins_cfg;
	} else {
		LOG_ERR("failed to apply pin config for device %s", dev->name);
	}

	k_sem_give(&drv_data->lock);

	return rc;
}

static int pcal64xxa_process_input(const struct device *dev, gpio_port_value_t *value)
{
	const struct pcal64xxa_drv_cfg *drv_cfg = dev->config;
	struct pcal64xxa_drv_data *drv_data = dev->data;
	int rc;
	uint16_t int_sources;
	uint16_t input_port;

	k_sem_take(&drv_data->lock, K_FOREVER);

	rc = drv_cfg->chip_api->inputs_read(&drv_cfg->i2c, &int_sources, &input_port);

	if (rc != 0) {
		LOG_ERR("failed to read inputs from device %s", dev->name);
		return rc;
	}

	if (value) {
		*value = input_port;
	}

	/* It may happen that some inputs change their states between above
	 * reads of the interrupt status and input port registers. Such changes
	 * will not be noted in `int_sources`, thus to correctly detect them,
	 * the current state of inputs needs to be additionally compared with
	 * the one read last time, and any differences need to be added to
	 * `int_sources`.
	 */
	int_sources |= ((input_port ^ drv_data->input_port_last) & ~drv_data->triggers.masked);

	drv_data->input_port_last = input_port;

	if (int_sources) {
		uint16_t dual_edge_triggers = drv_data->triggers.dual_edge;
		uint16_t falling_edge_triggers = (~dual_edge_triggers & drv_data->triggers.on_low);
		uint16_t fired_triggers = 0;

		/* For dual edge triggers, react to all state changes. */
		fired_triggers |= (int_sources & dual_edge_triggers);
		/* For single edge triggers, fire callbacks only for the pins
		 * that transitioned to their configured target state (0 for
		 * falling edges, 1 otherwise, hence the XOR operation below).
		 */
		fired_triggers |= ((input_port & int_sources) ^ falling_edge_triggers);

		/* Give back semaphore before the callback to make the same
		 * driver available again for the callback.
		 */
		k_sem_give(&drv_data->lock);

		gpio_fire_callbacks(&drv_data->callbacks, dev, fired_triggers);
	} else {
		k_sem_give(&drv_data->lock);
	}

	return 0;
}

static void pcal64xxa_work_handler(struct k_work *work)
{
	struct pcal64xxa_drv_data *drv_data = CONTAINER_OF(work, struct pcal64xxa_drv_data, work);

	(void)pcal64xxa_process_input(drv_data->dev, NULL);
}

static void pcal64xxa_int_gpio_handler(const struct device *dev, struct gpio_callback *gpio_cb,
				       uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct pcal64xxa_drv_data *drv_data =
		CONTAINER_OF(gpio_cb, struct pcal64xxa_drv_data, int_gpio_cb);

	k_work_submit(&drv_data->work);
}

static int pcal64xxa_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	int rc;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Reading of the input port also clears the generated interrupt,
	 * thus the configured callbacks must be fired also here if needed.
	 */
	rc = pcal64xxa_process_input(dev, value);

	return rc;
}

static int pcal64xxa_port_set_raw(const struct device *dev, uint16_t mask, uint16_t value,
				  uint16_t toggle)
{
	const struct pcal64xxa_drv_cfg *drv_cfg = dev->config;
	struct pcal64xxa_drv_data *drv_data = dev->data;
	int rc;
	uint16_t output;

	LOG_DBG("setting port with mask 0x%04X with value 0x%04X and toggle 0x%04X", mask, value,
		toggle);

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	output = (drv_data->pins_cfg.outputs_high & ~mask);
	output |= (value & mask);
	output ^= toggle;
	/*
	 * No need to limit `out` to only pins configured as outputs,
	 * as the chip anyway ignores all other bits in the register.
	 */
	rc = drv_cfg->chip_api->outputs_write(&drv_cfg->i2c, output);
	if (rc == 0) {
		drv_data->pins_cfg.outputs_high = output;
	}

	k_sem_give(&drv_data->lock);

	if (rc != 0) {
		LOG_ERR("%s: failed to write output port: %d", dev->name, rc);
		return -EIO;
	}

	return 0;
}

static int pcal64xxa_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				  gpio_port_value_t value)
{
	return pcal64xxa_port_set_raw(dev, (uint16_t)mask, (uint16_t)value, 0);
}

static int pcal64xxa_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return pcal64xxa_port_set_raw(dev, (uint16_t)pins, (uint16_t)pins, 0);
}

static int pcal64xxa_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return pcal64xxa_port_set_raw(dev, (uint16_t)pins, 0, 0);
}

static int pcal64xxa_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return pcal64xxa_port_set_raw(dev, 0, 0, (uint16_t)pins);
}

static int pcal64xxa_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
				      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct pcal64xxa_drv_cfg *drv_cfg = dev->config;
	struct pcal64xxa_drv_data *drv_data = dev->data;
	struct pcal64xxa_triggers triggers;
	int rc;

	LOG_DBG("configure interrupt for pin %i", pin);

	if (drv_cfg->gpio_interrupt.port == NULL) {
		return -ENOTSUP;
	}

	/* This device supports only edge-triggered interrupts. */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	triggers = drv_data->triggers;

	if (mode == GPIO_INT_MODE_DISABLED) {
		triggers.masked |= BIT(pin);
	} else {
		triggers.masked &= ~BIT(pin);
	}

	if (trig == GPIO_INT_TRIG_BOTH) {
		triggers.dual_edge |= BIT(pin);
	} else {
		triggers.dual_edge &= ~BIT(pin);

		if (trig == GPIO_INT_TRIG_LOW) {
			triggers.on_low |= BIT(pin);
		} else {
			triggers.on_low &= ~BIT(pin);
		}
	}

	rc = drv_cfg->chip_api->triggers_apply(&drv_cfg->i2c, &triggers);
	if (rc == 0) {
		drv_data->triggers = triggers;
	} else {
		LOG_ERR("failed to apply triggers for device %s", dev->name);
	}

	k_sem_give(&drv_data->lock);

	return rc;
}

static int pcal64xxa_manage_callback(const struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct pcal64xxa_drv_data *drv_data = dev->data;

	return gpio_manage_callback(&drv_data->callbacks, callback, set);
}

static int pcal64xxa_i2c_write(const struct i2c_dt_spec *i2c, uint8_t register_address,
			       uint8_t value)
{
	int rc;

	LOG_DBG("writing to register 0x%02X value 0x%02X", register_address, value);
	rc = i2c_reg_write_byte_dt(i2c, register_address, value);

	if (rc != 0) {
		LOG_ERR("unable to write to register 0x%02X, error %i", register_address, rc);
	}

	return rc;
}

static int pcal64xxa_i2c_read(const struct i2c_dt_spec *i2c, uint8_t register_address,
			      uint8_t *value)
{
	int rc;

	rc = i2c_reg_read_byte_dt(i2c, register_address, value);
	LOG_DBG("reading from register 0x%02X value 0x%02X", register_address, *value);

	if (rc != 0) {
		LOG_ERR("unable to read from register 0x%02X, error %i", register_address, rc);
	}

	return rc;
}

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_pcal6408a)
static int pcal6408a_pins_cfg_apply(const struct i2c_dt_spec *i2c,
				    const struct pcal64xxa_pins_cfg *pins_cfg)
{
	int rc;

	rc = pcal64xxa_i2c_write(i2c, PCAL6408A_REG_PULL_UP_DOWN_SELECT,
				 (uint8_t)pins_cfg->pull_ups_selected);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6408A_REG_PULL_UP_DOWN_ENABLE,
				 (uint8_t)pins_cfg->pulls_enabled);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6408A_REG_OUTPUT_PORT, (uint8_t)pins_cfg->outputs_high);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6408A_REG_CONFIGURATION,
				 (uint8_t)pins_cfg->configured_as_inputs);
	if (rc != 0) {
		return -EIO;
	}

	return 0;
}

static int pcal6408a_inputs_read(const struct i2c_dt_spec *i2c, uint16_t *int_sources,
				 uint16_t *input_port)
{
	int rc;
	uint8_t value;

	rc = pcal64xxa_i2c_read(i2c, PCAL6408A_REG_INTERRUPT_STATUS, &value);
	if (rc != 0) {
		return -EIO;
	}

	*int_sources = value;

	/* This read also clears the generated interrupt if any. */
	rc = pcal64xxa_i2c_read(i2c, PCAL6408A_REG_INPUT_PORT, &value);
	if (rc != 0) {
		return -EIO;
	}

	*input_port = value;

	return 0;
}

static int pcal6408a_outputs_write(const struct i2c_dt_spec *i2c, uint16_t outputs)
{
	int rc;

	/*
	 * No need to limit `out` to only pins configured as outputs,
	 * as the chip anyway ignores all other bits in the register.
	 */
	rc = pcal64xxa_i2c_write(i2c, PCAL6408A_REG_OUTPUT_PORT, (uint8_t)outputs);

	if (rc != 0) {
		LOG_ERR("failed to write output port: %d", rc);
		return -EIO;
	}

	return 0;
}

static int pcal6408a_triggers_apply(const struct i2c_dt_spec *i2c,
				    const struct pcal64xxa_triggers *triggers)
{
	int rc;
	uint8_t input_latch = ~triggers->masked;
	uint8_t interrupt_mask = triggers->masked;

	rc = pcal64xxa_i2c_write(i2c, PCAL6408A_REG_INPUT_LATCH, (uint8_t)input_latch);
	if (rc != 0) {
		LOG_ERR("failed to configure input latch: %d", rc);
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6408A_REG_INTERRUPT_MASK, (uint8_t)interrupt_mask);
	if (rc != 0) {
		LOG_ERR("failed to configure interrupt mask: %d", rc);
		return -EIO;
	}

	return 0;
}

static int pcal6408a_reset_state_apply(const struct i2c_dt_spec *i2c)
{
	int rc;
	static const uint8_t reset_state[][2] = {
		{PCAL6408A_REG_POLARITY_INVERSION, 0},
		{PCAL6408A_REG_OUTPUT_DRIVE_STRENGTH_0, 0xff},
		{PCAL6408A_REG_OUTPUT_DRIVE_STRENGTH_1, 0xff},
		{PCAL6408A_REG_OUTPUT_PORT_CONFIGURATION, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(reset_state); ++i) {
		rc = pcal64xxa_i2c_write(i2c, reset_state[i][0], reset_state[i][1]);
		if (rc != 0) {
			LOG_ERR("failed to reset register %02x: %d", reset_state[i][0], rc);
			return -EIO;
		}
	}

	return 0;
}

static const struct pcal64xxa_chip_api pcal6408a_chip_api = {
	.pins_cfg_apply = pcal6408a_pins_cfg_apply,
	.triggers_apply = pcal6408a_triggers_apply,
	.inputs_read = pcal6408a_inputs_read,
	.outputs_write = pcal6408a_outputs_write,
	.reset_state_apply = pcal6408a_reset_state_apply,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nxp_pcal6408a) */

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_pcal6416a)
static int pcal6416a_pins_cfg_apply(const struct i2c_dt_spec *i2c,
				    const struct pcal64xxa_pins_cfg *pins_cfg)
{
	int rc;

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_PULL_UP_DOWN_SELECT_0,
				 (uint8_t)pins_cfg->pull_ups_selected);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_PULL_UP_DOWN_SELECT_1,
				 (uint8_t)(pins_cfg->pull_ups_selected >> 8));
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_PULL_UP_DOWN_ENABLE_0,
				 (uint8_t)pins_cfg->pulls_enabled);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_PULL_UP_DOWN_ENABLE_1,
				 (uint8_t)(pins_cfg->pulls_enabled >> 8));
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_OUTPUT_PORT_0, (uint8_t)pins_cfg->outputs_high);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_OUTPUT_PORT_1,
				 (uint8_t)(pins_cfg->outputs_high >> 8));
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_CONFIGURATION_0,
				 (uint8_t)pins_cfg->configured_as_inputs);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_CONFIGURATION_1,
				 (uint8_t)(pins_cfg->configured_as_inputs >> 8));
	if (rc != 0) {
		return -EIO;
	}

	return 0;
}

static int pcal6416a_inputs_read(const struct i2c_dt_spec *i2c, uint16_t *int_sources,
				 uint16_t *input_port)
{
	int rc;
	uint8_t value_low;
	uint8_t value_high;

	rc = pcal64xxa_i2c_read(i2c, PCAL6416A_REG_INTERRUPT_STATUS_0, &value_low);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_read(i2c, PCAL6416A_REG_INTERRUPT_STATUS_1, &value_high);
	if (rc != 0) {
		return -EIO;
	}

	*int_sources = value_low | (value_high << 8);

	/* This read also clears the generated interrupt if any. */
	rc = pcal64xxa_i2c_read(i2c, PCAL6416A_REG_INPUT_PORT_0, &value_low);
	if (rc != 0) {
		return -EIO;
	}

	rc = pcal64xxa_i2c_read(i2c, PCAL6416A_REG_INPUT_PORT_1, &value_high);
	if (rc != 0) {
		LOG_ERR("failed to read input port: %d", rc);
		return -EIO;
	}

	*input_port = value_low | (value_high << 8);

	return 0;
}

static int pcal6416a_outputs_write(const struct i2c_dt_spec *i2c, uint16_t outputs)
{
	int rc;

	/*
	 * No need to limit `out` to only pins configured as outputs,
	 * as the chip anyway ignores all other bits in the register.
	 */
	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_OUTPUT_PORT_0, (uint8_t)outputs);

	if (rc != 0) {
		LOG_ERR("failed to write output port: %d", rc);
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_OUTPUT_PORT_1, (uint8_t)(outputs >> 8));

	if (rc != 0) {
		LOG_ERR("failed to write output port: %d", rc);
		return -EIO;
	}

	return 0;
}

static int pcal6416a_triggers_apply(const struct i2c_dt_spec *i2c,
				    const struct pcal64xxa_triggers *triggers)
{
	int rc;
	uint16_t input_latch = ~triggers->masked;
	uint16_t interrupt_mask = triggers->masked;

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_INPUT_LATCH_0, (uint8_t)input_latch);
	if (rc != 0) {
		LOG_ERR("failed to configure input latch: %d", rc);
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_INPUT_LATCH_1, (uint8_t)(input_latch >> 8));
	if (rc != 0) {
		LOG_ERR("failed to configure input latch: %d", rc);
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_INTERRUPT_MASK_0, (uint8_t)interrupt_mask);
	if (rc != 0) {
		LOG_ERR("failed to configure interrupt mask: %d", rc);
		return -EIO;
	}

	rc = pcal64xxa_i2c_write(i2c, PCAL6416A_REG_INTERRUPT_MASK_1,
				 (uint8_t)(interrupt_mask >> 8));
	if (rc != 0) {
		LOG_ERR("failed to configure interrupt mask: %d", rc);
		return -EIO;
	}

	return 0;
}

static int pcal6416a_reset_state_apply(const struct i2c_dt_spec *i2c)
{
	int rc;
	static const uint8_t reset_state[][2] = {
		{PCAL6416A_REG_POLARITY_INVERSION_0, 0},
		{PCAL6416A_REG_POLARITY_INVERSION_1, 0},
		{PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_0_0, 0xff},
		{PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_0_1, 0xff},
		{PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_1_0, 0xff},
		{PCAL6416A_REG_OUTPUT_DRIVE_STRENGTH_1_1, 0xff},
		{PCAL6416A_REG_OUTPUT_PORT_CONFIGURATION, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(reset_state); ++i) {
		rc = pcal64xxa_i2c_write(i2c, reset_state[i][0], reset_state[i][1]);
		if (rc != 0) {
			LOG_ERR("failed to reset register %02x: %d", reset_state[i][0], rc);
			return -EIO;
		}
	}

	return 0;
}

static const struct pcal64xxa_chip_api pcal6416a_chip_api = {
	.pins_cfg_apply = pcal6416a_pins_cfg_apply,
	.triggers_apply = pcal6416a_triggers_apply,
	.inputs_read = pcal6416a_inputs_read,
	.outputs_write = pcal6416a_outputs_write,
	.reset_state_apply = pcal6416a_reset_state_apply,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nxp_pcal6416a) */

int pcal64xxa_init(const struct device *dev)
{
	const struct pcal64xxa_drv_cfg *drv_cfg = dev->config;
	struct pcal64xxa_drv_data *drv_data = dev->data;
	const struct pcal64xxa_pins_cfg initial_pins_cfg = {
		.configured_as_inputs = 0xFFFF,
		.outputs_high = 0,
		.pull_ups_selected = 0,
		.pulls_enabled = 0,
	};
	const struct pcal64xxa_triggers initial_triggers = {
		.masked = 0xFFFF,
	};
	int rc;
	uint16_t int_sources;

	LOG_DBG("initializing PCAL64XXA");

	if (drv_cfg->ngpios != 8U && drv_cfg->ngpios != 16U) {
		LOG_ERR("Invalid value ngpios=%u. Expected 8 or 16!", drv_cfg->ngpios);
		return -EINVAL;
	}

	/*
	 * executing the is ready check on i2c_bus_dev instead of on i2c.bus
	 * to avoid a const warning
	 */
	if (!i2c_is_ready_dt(&drv_cfg->i2c)) {
		LOG_ERR("%s is not ready", drv_cfg->i2c.bus->name);
		return -ENODEV;
	}

	/* If the RESET line is available, use it to reset the expander.
	 * Otherwise, write reset values to registers that are not used by
	 * this driver.
	 */
	if (drv_cfg->gpio_reset.port != NULL) {
		if (!device_is_ready(drv_cfg->gpio_reset.port)) {
			LOG_ERR("reset gpio device is not ready");
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&drv_cfg->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (rc != 0) {
			LOG_ERR("%s: failed to configure RESET line: %d", dev->name, rc);
			return -EIO;
		}

		/* RESET signal needs to be active for a minimum of 30 ns. */
		k_busy_wait(1);

		rc = gpio_pin_set_dt(&drv_cfg->gpio_reset, 0);
		if (rc != 0) {
			LOG_ERR("%s: failed to deactivate RESET line: %d", dev->name, rc);
			return -EIO;
		}

		/* Give the expander at least 200 ns to recover after reset. */
		k_busy_wait(1);
	} else {
		rc = drv_cfg->chip_api->reset_state_apply(&drv_cfg->i2c);

		if (rc != 0) {
			LOG_ERR("failed to apply reset state to device %s", dev->name);
			return rc;
		}
	}

	/* Set initial configuration of the pins. */
	rc = drv_cfg->chip_api->pins_cfg_apply(&drv_cfg->i2c, &initial_pins_cfg);
	if (rc != 0) {
		LOG_ERR("failed to apply pin config for device %s", dev->name);
		return rc;
	}

	drv_data->pins_cfg = initial_pins_cfg;

	/* Read initial state of the input port register. */
	rc = drv_cfg->chip_api->inputs_read(&drv_cfg->i2c, &int_sources,
					    &drv_data->input_port_last);
	if (rc != 0) {
		LOG_ERR("failed to read inputs for device %s", dev->name);
		return rc;
	}

	/* Set initial state of the interrupt related registers. */
	rc = drv_cfg->chip_api->triggers_apply(&drv_cfg->i2c, &initial_triggers);
	if (rc != 0) {
		LOG_ERR("failed to apply triggers for device %s", dev->name);
		return rc;
	}

	drv_data->triggers = initial_triggers;

	/* If the INT line is available, configure the callback for it. */
	if (drv_cfg->gpio_interrupt.port != NULL) {
		if (!device_is_ready(drv_cfg->gpio_interrupt.port)) {
			LOG_ERR("interrupt gpio device is not ready");
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&drv_cfg->gpio_interrupt, GPIO_INPUT);
		if (rc != 0) {
			LOG_ERR("%s: failed to configure INT line: %d", dev->name, rc);
			return -EIO;
		}

		rc = gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt,
						     GPIO_INT_EDGE_TO_ACTIVE);
		if (rc != 0) {
			LOG_ERR("%s: failed to configure INT interrupt: %d", dev->name, rc);
			return -EIO;
		}

		gpio_init_callback(&drv_data->int_gpio_cb, pcal64xxa_int_gpio_handler,
				   BIT(drv_cfg->gpio_interrupt.pin));
		rc = gpio_add_callback(drv_cfg->gpio_interrupt.port, &drv_data->int_gpio_cb);
		if (rc != 0) {
			LOG_ERR("%s: failed to add INT callback: %d", dev->name, rc);
			return -EIO;
		}
	}

	/* Device configured, unlock it so that it can be used. */
	k_sem_give(&drv_data->lock);

	return 0;
}

#define PCAL64XXA_INIT_INT_GPIO_FIELDS(idx)                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, int_gpios),                                         \
		    (GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(idx), int_gpios, 0)),                     \
		    ({0}))

#define PCAL64XXA_INIT_RESET_GPIO_FIELDS(idx)                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, reset_gpios),                                       \
		    (GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(idx), reset_gpios, 0)),                   \
		    ({0}))

#define GPIO_PCAL6408A_INST(idx)                                                                   \
	static const struct gpio_driver_api pcal6408a_drv_api##idx = {                             \
		.pin_configure = pcal64xxa_pin_configure,                                          \
		.port_get_raw = pcal64xxa_port_get_raw,                                            \
		.port_set_masked_raw = pcal64xxa_port_set_masked_raw,                              \
		.port_set_bits_raw = pcal64xxa_port_set_bits_raw,                                  \
		.port_clear_bits_raw = pcal64xxa_port_clear_bits_raw,                              \
		.port_toggle_bits = pcal64xxa_port_toggle_bits,                                    \
		.pin_interrupt_configure = pcal64xxa_pin_interrupt_configure,                      \
		.manage_callback = pcal64xxa_manage_callback,                                      \
	};                                                                                         \
	static const struct pcal64xxa_drv_cfg pcal6408a_cfg##idx = {                               \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),             \
			},                                                                         \
		.i2c = I2C_DT_SPEC_INST_GET(idx),                                                  \
		.ngpios = DT_INST_PROP(idx, ngpios),                                               \
		.gpio_interrupt = PCAL64XXA_INIT_INT_GPIO_FIELDS(idx),                             \
		.gpio_reset = PCAL64XXA_INIT_RESET_GPIO_FIELDS(idx),                               \
		.chip_api = &pcal6408a_chip_api,                                                   \
	};                                                                                         \
	static struct pcal64xxa_drv_data pcal6408a_data##idx = {                                   \
		.lock = Z_SEM_INITIALIZER(pcal6408a_data##idx.lock, 1, 1),                         \
		.work = Z_WORK_INITIALIZER(pcal64xxa_work_handler),                                \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, pcal64xxa_init, NULL, &pcal6408a_data##idx,                     \
			      &pcal6408a_cfg##idx, POST_KERNEL,                                    \
			      CONFIG_GPIO_PCAL64XXA_INIT_PRIORITY, &pcal6408a_drv_api##idx);

#define DT_DRV_COMPAT nxp_pcal6408a
DT_INST_FOREACH_STATUS_OKAY(GPIO_PCAL6408A_INST)

#define GPIO_PCAL6416A_INST(idx)                                                                   \
	static const struct gpio_driver_api pcal6416a_drv_api##idx = {                             \
		.pin_configure = pcal64xxa_pin_configure,                                          \
		.port_get_raw = pcal64xxa_port_get_raw,                                            \
		.port_set_masked_raw = pcal64xxa_port_set_masked_raw,                              \
		.port_set_bits_raw = pcal64xxa_port_set_bits_raw,                                  \
		.port_clear_bits_raw = pcal64xxa_port_clear_bits_raw,                              \
		.port_toggle_bits = pcal64xxa_port_toggle_bits,                                    \
		.pin_interrupt_configure = pcal64xxa_pin_interrupt_configure,                      \
		.manage_callback = pcal64xxa_manage_callback,                                      \
	};                                                                                         \
	static const struct pcal64xxa_drv_cfg pcal6416a_cfg##idx = {                               \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),             \
			},                                                                         \
		.i2c = I2C_DT_SPEC_INST_GET(idx),                                                  \
		.ngpios = DT_INST_PROP(idx, ngpios),                                               \
		.gpio_interrupt = PCAL64XXA_INIT_INT_GPIO_FIELDS(idx),                             \
		.gpio_reset = PCAL64XXA_INIT_RESET_GPIO_FIELDS(idx),                               \
		.chip_api = &pcal6416a_chip_api,                                                   \
	};                                                                                         \
	static struct pcal64xxa_drv_data pcal6416a_data##idx = {                                   \
		.lock = Z_SEM_INITIALIZER(pcal6416a_data##idx.lock, 1, 1),                         \
		.work = Z_WORK_INITIALIZER(pcal64xxa_work_handler),                                \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, pcal64xxa_init, NULL, &pcal6416a_data##idx,                     \
			      &pcal6416a_cfg##idx, POST_KERNEL,                                    \
			      CONFIG_GPIO_PCAL64XXA_INIT_PRIORITY, &pcal6416a_drv_api##idx);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pcal6416a
DT_INST_FOREACH_STATUS_OKAY(GPIO_PCAL6416A_INST)
