/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 * Copyright (c) 2018 Aapo Vienamo
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2020 ZedBlox Ltd.
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tca9538

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include "gpio_tca9538.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(tca9538, CONFIG_GPIO_LOG_LEVEL);

#include "gpio_utils.h"

/* Number of pins supported by the device */
#define NUM_PINS 8

/* Max to select all pins supported on the device. */
#define ALL_PINS ((uint8_t)BIT_MASK(NUM_PINS))

/** Cache of the output configuration and data of the pins. */
struct tca9538_pin_state {
	uint8_t polarity;
	uint8_t dir;
	uint8_t input;
	uint8_t output;
};

struct tca9538_irq_state {
	uint8_t interrupt_mask;
	uint8_t interrupt_rising;
	uint8_t interrupt_falling;
};

/** Runtime driver data */
struct tca9538_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct tca9538_pin_state pin_state;
	struct k_sem lock;

	bool interrupt_enabled;

	const struct device *gpio_int;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct tca9538_irq_state irq_state;
	const struct device *dev;
	/* user ISR cb */
	sys_slist_t cb;
	/* Enabled INT pins generating a cb */
	uint8_t cb_pins;
};

/** Configuration data */
struct tca9538_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *i2c_dev;

	const char *gpio_int_dev_name;
	gpio_pin_t gpio_pin;
	gpio_dt_flags_t gpio_flags;

	uint8_t i2c_addr;
};

/**
 * @brief Gets the state of input pins of the TCA9538 I/O Port and
 * stores in driver data struct.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int update_input(const struct device *dev)
{
	const struct tca9538_config *cfg = dev->config;
	struct tca9538_drv_data *drv_data = dev->data;
	uint8_t cmd = TCA9538_INPUT_PORT;
	uint8_t input_states;
	int ret = 0;

	ret = i2c_write_read(cfg->i2c_dev, cfg->i2c_addr, &cmd,
			     sizeof(cmd), (uint8_t *)&input_states,
			     sizeof(input_states));
	if (ret == 0) {
		drv_data->pin_state.input = input_states;
	}
	return ret;
}

/**
 * @brief Handles interrupt triggered by the interrupt pin of TCA9538 I/O Port.
 *
 * If nint_gpios is configured in device tree then this will be triggered each
 * time a gpio configured as an input changes state. The gpio input states are
 * read in this function which clears the interrupt.
 *
 * @param arg Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int gpio_tca9538_handle_interrupt(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct tca9538_drv_data *drv_data = dev->data;
	struct tca9538_irq_state *irq_state = &drv_data->irq_state;
	int ret = 0;
	uint8_t previous_state;
	uint8_t current_state;
	uint8_t transitioned_pins;
	uint8_t interrupt_mask = 0;

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Store previous input state then read new value */
	previous_state = drv_data->pin_state.input;
	ret = update_input(dev);
	if (ret != 0) {
		goto out;
	}

	/* Find out which pins have changed state */
	current_state = drv_data->pin_state.input;
	transitioned_pins = previous_state ^ current_state;

	/* Mask gpio transactions with rising/falling edge interrupt config */
	interrupt_mask = (irq_state->interrupt_rising & transitioned_pins &
			  current_state);
	interrupt_mask |= (irq_state->interrupt_falling & transitioned_pins &
			   previous_state);

out:
	k_sem_give(&drv_data->lock);

	if (ret == 0) {
		gpio_fire_callbacks(&drv_data->cb, dev, interrupt_mask);
	}

	return ret;
}

/**
 * @brief Work handler for TCA9538 interrupt
 *
 * @param work Work struct that contains pointer to interrupt handler function
 */
static void gpio_tca9538_work_handler(struct k_work *work)
{
	struct tca9538_drv_data *drv_data =
		CONTAINER_OF(work, struct tca9538_drv_data, work);

	gpio_tca9538_handle_interrupt((struct device *)drv_data->dev);
}

/**
 * @brief ISR for intterupt pin of TCA9538
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param gpio_cb Pointer to callback function struct
 * @param pins Bitmask of pins that triggered interrupt
 */
static void gpio_tca9538_init_cb(const struct device *dev,
				 struct gpio_callback *gpio_cb, uint32_t pins)
{
	struct tca9538_drv_data *drv_data =
		CONTAINER_OF(gpio_cb, struct tca9538_drv_data, gpio_cb);

	ARG_UNUSED(pins);

	k_work_submit(&drv_data->work);
}

/**
 * @brief Initialization function of TCA9538
 *
 * This sets initial input/ output configuration, output states and
 * input polarity. The interrupt is configured if this is enabled.
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_tca9538_init(const struct device *dev)
{
	const struct tca9538_config *cfg = dev->config;
	struct tca9538_drv_data *drv_data = dev->data;
	int rc = 0;
	char tx_dat[2];

	if (!device_is_ready(cfg->i2c_dev)) {
		LOG_ERR("I2C device not found");
		return -EINVAL;
	}

	if ((drv_data->interrupt_enabled) && (rc == 0)) {
		drv_data->dev = dev;
		drv_data->gpio_int = device_get_binding(cfg->gpio_int_dev_name);

		if (!drv_data->gpio_int) {
			rc = -ENOTSUP;
		} else {
			k_work_init(&drv_data->work, gpio_tca9538_work_handler);

			gpio_pin_configure(drv_data->gpio_int, cfg->gpio_pin,
					   GPIO_INPUT | cfg->gpio_flags);
			gpio_pin_interrupt_configure(drv_data->gpio_int,
						     cfg->gpio_pin,
						     GPIO_INT_EDGE_TO_ACTIVE);

			gpio_init_callback(&drv_data->gpio_cb,
					   gpio_tca9538_init_cb,
					   BIT(cfg->gpio_pin));

			gpio_add_callback(drv_data->gpio_int,
					  &drv_data->gpio_cb);

			drv_data->irq_state = (struct tca9538_irq_state){
				.interrupt_mask = ALL_PINS,
			};
		}
	}

	if (rc == 0) {
		tx_dat[0] = TCA9538_OUTPUT_PORT;
		tx_dat[1] = drv_data->pin_state.output;
		rc = i2c_write(cfg->i2c_dev, tx_dat, sizeof(tx_dat),
			       cfg->i2c_addr);
	}

	if (rc == 0) {
		tx_dat[0] = TCA9538_CONFIGURATION;
		tx_dat[1] = drv_data->pin_state.dir;
		rc = i2c_write(cfg->i2c_dev, tx_dat, sizeof(tx_dat),
			       cfg->i2c_addr);
	}

	/* Do an initial read, this also clears the interrupt pin */
	if (rc == 0) {
		rc = update_input(dev);
	}

	if (rc != 0) {
		LOG_ERR("%s init failed: %d", dev->name, rc);
	} else {
		LOG_INF("%s init ok", dev->name);
	}
	k_sem_give(&drv_data->lock);
	return rc;
}

/**
 * @brief Configures pins of TCA9538 I/O Port.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number (0 - 7)
 * @param flags Configuration flags, see gpio.h
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int gpio_tca9538_config(const struct device *dev, gpio_pin_t pin,
			       gpio_flags_t flags)
{
	const struct tca9538_config *cfg = dev->config;
	struct tca9538_drv_data *drv_data = dev->data;
	struct tca9538_pin_state *pins = &drv_data->pin_state;
	int rc = 0;
	bool data_first = false;
	char tx_dat[2];

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Zephyr currently defines drive strength support based on
	 * the behavior and capabilities of the Nordic GPIO
	 * peripheral: strength defaults to low but can be set high,
	 * and is controlled independently for output levels.
	 *
	 * The TCA9538 supports only high strength, and does not
	 * support different strengths for different levels.
	 *
	 * Until something more general is available reject any
	 * attempt to set a non-default drive strength.
	 */
	if ((flags & (GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH)) != 0) {
		rc = -ENOTSUP;
		goto out;
	}

	if ((flags & GPIO_OPEN_DRAIN) != 0 || (flags & GPIO_OPEN_SOURCE) != 0) {
		/* Open drain not supported */
		/* Open source not supported */
		rc = -ENOTSUP;
		goto out;
	}

	/* The TCA9538 has no internal pull up support */
	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		rc = -ENOTSUP;
		goto out;
	}

	/* The TCA9538 does not support internal debounce */
	if ((flags & GPIO_INT_DEBOUNCE) != 0) {
		rc = -ENOTSUP;
		goto out;
	}

	/* Ensure either Output or Input is specified */
	if ((flags & GPIO_OUTPUT) != 0) {
		pins->dir &= ~BIT(pin);
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			pins->output &= ~BIT(pin);
			data_first = true;
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			pins->output |= BIT(pin);
			data_first = true;
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		pins->dir |= BIT(pin);
	} else {
		rc = -ENOTSUP;
		goto out;
	}

	/* Set output values */
	if (data_first) {
		tx_dat[0] = TCA9538_OUTPUT_PORT;
		tx_dat[1] = pins->output;
		rc = i2c_write(cfg->i2c_dev, tx_dat, sizeof(tx_dat),
			       cfg->i2c_addr);
	}

	/* Set input polarity inversion */
	if (rc == 0) {
		tx_dat[0] = TCA9538_POLARITY_INVERSION;
		tx_dat[1] = pins->polarity;
		rc = i2c_write(cfg->i2c_dev, tx_dat, sizeof(tx_dat),
			       cfg->i2c_addr);
	}

	if (rc == 0) {
		/* Set pin directions */
		tx_dat[0] = TCA9538_CONFIGURATION;
		tx_dat[1] = pins->dir;
		rc = i2c_write(cfg->i2c_dev, tx_dat, sizeof(tx_dat),
			       cfg->i2c_addr);
	}
out:
	k_sem_give(&drv_data->lock);
	return rc;
}

/**
 * @brief Gets the state of the TCA9538 I/O Port.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Value at the I/O Port.
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int gpio_tca9538_port_get(const struct device *dev,
				 gpio_port_value_t *value)
{
	const struct tca9538_config *cfg = dev->config;
	struct tca9538_drv_data *drv_data = dev->data;
	uint8_t input_pin_data;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Read Input Register */
	uint8_t cmd = TCA9538_INPUT_PORT;

	rc = i2c_write_read(cfg->i2c_dev, cfg->i2c_addr, &cmd,
			    sizeof(cmd), &input_pin_data,
			    sizeof(input_pin_data));
	LOG_DBG("read %x got %d", input_pin_data, rc);

	if (rc == 0) {
		drv_data->pin_state.input = input_pin_data;
		*value = (gpio_port_value_t)(drv_data->pin_state.input);
	}

	k_sem_give(&drv_data->lock);
	return rc;
}

/**
 * @brief Write to TCA9538 I/O output pin(s).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask Mask of which pins to write to.
 * @param value Value of pins (bit x sets state for pin x)
 * @param toggle Set Bit(x) to toggle pin x
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int gpio_tca9538_port_write(const struct device *dev,
				   gpio_port_pins_t mask,
				   gpio_port_value_t value,
				   gpio_port_value_t toggle)
{
	const struct tca9538_config *cfg = dev->config;
	struct tca9538_drv_data *drv_data = dev->data;
	uint8_t *outp = &drv_data->pin_state.output;
	int rc;
	char tx_dat[2];
	uint8_t orig_out;
	uint8_t out;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	orig_out = *outp;
	out = ((orig_out & ~mask) | (value & mask)) ^ toggle;

	tx_dat[0] = TCA9538_OUTPUT_PORT;
	tx_dat[1] = out;
	rc = i2c_write(cfg->i2c_dev, tx_dat, sizeof(tx_dat),
		       cfg->i2c_addr);

	if (rc == 0) {
		*outp = out;
	}

	k_sem_give(&drv_data->lock);

	LOG_DBG("write %x msk %08x val %08x => %x: %d", orig_out, mask,
		value, out, rc);

	return rc;
}

static int gpio_tca9538_port_set_masked(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	return gpio_tca9538_port_write(dev, mask, value, 0);
}

static int gpio_tca9538_port_set_bits(const struct device *dev,
				      gpio_port_pins_t pins)
{
	return gpio_tca9538_port_write(dev, pins, pins, 0);
}

static int gpio_tca9538_port_clear_bits(const struct device *dev,
					gpio_port_pins_t pins)
{
	return gpio_tca9538_port_write(dev, pins, 0, 0);
}

static int gpio_tca9538_port_toggle_bits(const struct device *dev,
					 gpio_port_pins_t pins)
{
	return gpio_tca9538_port_write(dev, 0, 0, pins);
}

/**
 * @brief Write to TCA9538 I/O output pin(s).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number to apply interrupt configuration (0 - 7)
 * @param mode Interrupt mode. Disabled or edge triggered.
 * @param trig Interrupt trigger. Rising edge, falling edge or both.
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int gpio_tca9538_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	struct tca9538_drv_data *drv_data = dev->data;

	if (drv_data->interrupt_enabled) {
		/* Device does not support level-triggered interrupts. */
		if (mode == GPIO_INT_MODE_LEVEL) {
			return -ENOTSUP;
		}

		struct tca9538_drv_data *drv_data = dev->data;
		struct tca9538_irq_state *irq = &drv_data->irq_state;

		k_sem_take(&drv_data->lock, K_FOREVER);

		if (mode == GPIO_INT_MODE_DISABLED) {
			drv_data->cb_pins &= ~BIT(pin);
			irq->interrupt_mask |= BIT(pin);
		} else { /* GPIO_INT_MODE_EDGE */
			drv_data->cb_pins |= BIT(pin);
			irq->interrupt_mask &= ~BIT(pin);
			if (trig == GPIO_INT_TRIG_BOTH) {
				irq->interrupt_falling |= BIT(pin);
				irq->interrupt_rising |= BIT(pin);
			} else if (trig == GPIO_INT_TRIG_LOW) {
				irq->interrupt_falling |= BIT(pin);
				irq->interrupt_rising &= ~BIT(pin);
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				irq->interrupt_falling &= ~BIT(pin);
				irq->interrupt_rising |= BIT(pin);
			}
		}
		k_sem_give(&drv_data->lock);
	}

	return 0;
}

static int gpio_tca9538_manage_callback(const struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	struct tca9538_drv_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static const struct gpio_driver_api api_table = {
	.pin_configure = gpio_tca9538_config,
	.port_get_raw = gpio_tca9538_port_get,
	.port_set_masked_raw = gpio_tca9538_port_set_masked,
	.port_set_bits_raw = gpio_tca9538_port_set_bits,
	.port_clear_bits_raw = gpio_tca9538_port_clear_bits,
	.port_toggle_bits = gpio_tca9538_port_toggle_bits,
	.pin_interrupt_configure = gpio_tca9538_pin_interrupt_configure,
	.manage_callback = gpio_tca9538_manage_callback,
};

#define INST_DT_TCA9538(inst, t) DT_INST(inst, ti_tca9538##t)

#define GPIO_TCA9538_INIT(n)						\
	static const struct tca9538_config tca9538_cfg_##n = {		\
	.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(n)),			\
	.common = {							\
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
	},								\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, nint_gpios), (		\
		.gpio_int_dev_name = DT_INST_GPIO_LABEL(n, nint_gpios),	\
		.gpio_pin = DT_INST_GPIO_PIN(n, nint_gpios),		\
		.gpio_flags = DT_INST_GPIO_FLAGS(n, nint_gpios),	\
	))								\
	.i2c_addr = DT_INST_REG_ADDR(n),				\
	};								\
									\
	static struct tca9538_drv_data tca9538_drvdata_##n = {		\
	.lock = Z_SEM_INITIALIZER(tca9538_drvdata_##n.lock, 1, 1),	\
	.interrupt_enabled = DT_INST_NODE_HAS_PROP(n, nint_gpios),	\
	.pin_state = {							\
		.polarity = (ALL_PINS & DT_INST_PROP(			\
				      n, init_input_inversion)),	\
		.dir = (ALL_PINS & ~(DT_INST_PROP(n, init_out_low) |	\
				     DT_INST_PROP(n, init_out_high))),	\
		.output = (ALL_PINS & ~DT_INST_PROP(n, init_out_low)),	\
		},							\
	};								\
	DEVICE_DT_INST_DEFINE(n,					\
		gpio_tca9538_init,					\
		NULL,							\
		&tca9538_drvdata_##n,					\
		&tca9538_cfg_##n,					\
		POST_KERNEL,						\
		CONFIG_GPIO_TCA9538_INIT_PRIORITY,			\
		&api_table						\
		);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TCA9538_INIT)
