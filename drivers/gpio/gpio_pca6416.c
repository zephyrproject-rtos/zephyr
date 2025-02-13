/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca6416

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pca6416, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/gpio/gpio_utils.h>

/* PCA6416 Register addresses */
#define PCA6416_INPUT_PORT0		0x00
#define PCA6416_INPUT_PORT1		0x01
#define PCA6416_OUTPUT_PORT0		0x02
#define PCA6416_OUTPUT_PORT1		0x03
#define PCA6416_POL_INV_PORT0		0x04
#define PCA6416_POL_INV_PORT1		0x05
#define PCA6416_CONFIG_PORT0		0x06
#define PCA6416_CONFIG_PORT1		0x07

/* Number of pins supported by the device */
#define NUM_PINS 16

/* Max to select all pins supported on the device. */
#define ALL_PINS ((uint16_t)BIT_MASK(NUM_PINS))

/** Cache of the output configuration and data of the pins. */
struct pca6416_pin_state {
	uint16_t dir;
	uint16_t input;
	uint16_t output;
};

struct pca6416_irq_state {
	uint16_t rising;
	uint16_t falling;
};

/** Runtime driver data */
struct pca6416_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct pca6416_pin_state pin_state;
	struct k_sem lock;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct pca6416_irq_state irq_state;
	const struct device *dev;
	/* user ISR cb */
	sys_slist_t cb;
};

/** Configuration data */
struct pca6416_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec gpio_int;
	bool interrupt_enabled;
};

/**
 * @brief Gets the state of input pins of the PCA6416 I/O Port and
 * stores in driver data struct.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int update_input(const struct device *dev)
{
	const struct pca6416_config *cfg = dev->config;
	struct pca6416_drv_data *drv_data = dev->data;
	uint16_t input_states;
	int rc = 0;

	rc = i2c_burst_read_dt(&cfg->i2c, PCA6416_INPUT_PORT0, (uint8_t *)&input_states, 2);
	if (rc == 0) {
		drv_data->pin_state.input = input_states;
	}
	return rc;
}

/**
 * @brief Handles interrupt triggered by the interrupt pin of PCA6416 I/O Port.
 *
 * If interrupt-gpios is configured in device tree then this will be triggered each
 * time a gpio configured as an input changes state. The gpio input states are
 * read in this function which clears the interrupt.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static void gpio_pca6416_handle_interrupt(const struct device *dev)
{
	struct pca6416_drv_data *drv_data = dev->data;
	struct pca6416_irq_state *irq_state = &drv_data->irq_state;
	int rc;
	uint16_t previous_state;
	uint16_t current_state;
	uint16_t transitioned_pins;
	uint16_t interrupt_status = 0;

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Any interrupts enabled? */
	if (!irq_state->rising && !irq_state->falling) {
		rc = -EINVAL;
		goto out;
	}

	/* Store previous input state then read new value */
	previous_state = drv_data->pin_state.input;
	rc = update_input(dev);
	if (rc != 0) {
		goto out;
	}

	/* Find out which input pins have changed state */
	current_state = drv_data->pin_state.input;
	transitioned_pins = previous_state ^ current_state;

	/* Mask gpio transactions with rising/falling edge interrupt config */
	interrupt_status = (irq_state->rising & transitioned_pins &
			  current_state);
	interrupt_status |= (irq_state->falling & transitioned_pins &
			   previous_state);

out:
	k_sem_give(&drv_data->lock);

	if ((rc == 0) && (interrupt_status)) {
		gpio_fire_callbacks(&drv_data->cb, dev, interrupt_status);
	}
}

/**
 * @brief Work handler for PCA6416 interrupt
 *
 * @param work Work struct that contains pointer to interrupt handler function
 */
static void gpio_pca6416_work_handler(struct k_work *work)
{
	struct pca6416_drv_data *drv_data =
		CONTAINER_OF(work, struct pca6416_drv_data, work);

	gpio_pca6416_handle_interrupt(drv_data->dev);
}

/**
 * @brief ISR for interrupt pin of PCA6416
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param gpio_cb Pointer to callback function struct
 * @param pins Bitmask of pins that triggered interrupt
 */
static void gpio_pca6416_init_cb(const struct device *dev,
				 struct gpio_callback *gpio_cb, uint32_t pins)
{
	struct pca6416_drv_data *drv_data =
		CONTAINER_OF(gpio_cb, struct pca6416_drv_data, gpio_cb);
	ARG_UNUSED(pins);

	k_work_submit(&drv_data->work);
}

static int gpio_pca6416_config(const struct device *dev, gpio_pin_t pin,
			       gpio_flags_t flags)
{
	const struct pca6416_config *cfg = dev->config;
	struct pca6416_drv_data *drv_data = dev->data;
	struct pca6416_pin_state *pins = &drv_data->pin_state;
	int rc = 0;
	bool data_first = false;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Single Ended lines (Open drain and open source) not supported */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* The PCA6416 has no internal pull up support */
	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		return -ENOTSUP;
	}

	/* Simultaneous input & output mode not supported */
	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

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
		rc = i2c_burst_write_dt(&cfg->i2c, PCA6416_OUTPUT_PORT0,
			(uint8_t *)&pins->output, 2);
	}

	if (rc == 0) {
		/* Set pin directions */
		rc = i2c_burst_write_dt(&cfg->i2c, PCA6416_CONFIG_PORT0,
			(uint8_t *)&pins->dir, 2);
	}

	if (rc == 0) {
		/* Refresh input status */
		rc = update_input(dev);
	}

out:
	k_sem_give(&drv_data->lock);
	return rc;
}

static int gpio_pca6416_port_read(const struct device *dev,
				 gpio_port_value_t *value)
{
	const struct pca6416_config *cfg = dev->config;
	struct pca6416_drv_data *drv_data = dev->data;
	uint16_t input_pin_data;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Read Input Register */
	rc = i2c_burst_read_dt(&cfg->i2c, PCA6416_INPUT_PORT0, (uint8_t *)&input_pin_data, 2);

	LOG_DBG("read %x got %d", input_pin_data, rc);

	if (rc == 0) {
		drv_data->pin_state.input = input_pin_data;
		*value = (gpio_port_value_t)(drv_data->pin_state.input);
	}

	k_sem_give(&drv_data->lock);
	return rc;
}

static int gpio_pca6416_port_write(const struct device *dev,
				   gpio_port_pins_t mask,
				   gpio_port_value_t value,
				   gpio_port_value_t toggle)
{
	const struct pca6416_config *cfg = dev->config;
	struct pca6416_drv_data *drv_data = dev->data;
	uint16_t *outp = &drv_data->pin_state.output;
	int rc;
	uint16_t orig_out;
	uint16_t out;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	orig_out = *outp;
	out = ((orig_out & ~mask) | (value & mask)) ^ toggle;

	rc = i2c_burst_write_dt(&cfg->i2c, PCA6416_OUTPUT_PORT0, (uint8_t *)&out, 2);

	if (rc == 0) {
		*outp = out;
	}

	k_sem_give(&drv_data->lock);

	LOG_DBG("write %x msk %08x val %08x => %x: %d", orig_out, mask,
		value, out, rc);

	return rc;
}

static int gpio_pca6416_port_set_masked(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	return gpio_pca6416_port_write(dev, mask, value, 0);
}

static int gpio_pca6416_port_set_bits(const struct device *dev,
				      gpio_port_pins_t pins)
{
	return gpio_pca6416_port_write(dev, pins, pins, 0);
}

static int gpio_pca6416_port_clear_bits(const struct device *dev,
					gpio_port_pins_t pins)
{
	return gpio_pca6416_port_write(dev, pins, 0, 0);
}

static int gpio_pca6416_port_toggle_bits(const struct device *dev,
					 gpio_port_pins_t pins)
{
	return gpio_pca6416_port_write(dev, 0, 0, pins);
}

static int gpio_pca6416_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	const struct pca6416_config *cfg = dev->config;
	struct pca6416_drv_data *drv_data = dev->data;
	struct pca6416_irq_state *irq = &drv_data->irq_state;

	if (!cfg->interrupt_enabled) {
		return -ENOTSUP;
	}
	/* Device does not support level-triggered interrupts. */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	if (mode == GPIO_INT_MODE_DISABLED) {
		irq->falling &= ~BIT(pin);
		irq->rising &= ~BIT(pin);
	} else { /* GPIO_INT_MODE_EDGE */
		if (trig == GPIO_INT_TRIG_BOTH) {
			irq->falling |= BIT(pin);
			irq->rising |= BIT(pin);
		} else if (trig == GPIO_INT_TRIG_LOW) {
			irq->falling |= BIT(pin);
			irq->rising &= ~BIT(pin);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			irq->falling &= ~BIT(pin);
			irq->rising |= BIT(pin);
		}
	}

	k_sem_give(&drv_data->lock);

	return 0;
}

static int gpio_pca6416_manage_callback(const struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	struct pca6416_drv_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

/**
 * @brief Initialization function of PCA6416
 *
 * This sets initial input/ output configuration and output states.
 * The interrupt is configured if this is enabled.
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_pca6416_init(const struct device *dev)
{
	const struct pca6416_config *cfg = dev->config;
	struct pca6416_drv_data *drv_data = dev->data;
	int rc = 0;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus device not found");
		goto out;
	}

	/* Do an initial read, this clears the interrupt pin and sets
	 * up the initial value of the pin state input data.
	 */
	rc = update_input(dev);
	if (rc) {
		goto out;
	}
	if (cfg->interrupt_enabled) {
		if (!gpio_is_ready_dt(&cfg->gpio_int)) {
			LOG_ERR("Cannot get pointer to gpio interrupt device");
			rc = -EINVAL;
			goto out;
		}

		drv_data->dev = dev;

		k_work_init(&drv_data->work, gpio_pca6416_work_handler);

		rc = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
		if (rc) {
			goto out;
		}

		rc = gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
						GPIO_INT_EDGE_TO_ACTIVE);
		if (rc) {
			goto out;
		}

		gpio_init_callback(&drv_data->gpio_cb,
					gpio_pca6416_init_cb,
					BIT(cfg->gpio_int.pin));
		rc = gpio_add_callback(cfg->gpio_int.port,
					&drv_data->gpio_cb);

		if (rc) {
			goto out;
		}

	}
out:
	if (rc) {
		LOG_ERR("%s init failed: %d", dev->name, rc);
	} else {
		LOG_INF("%s init ok", dev->name);
	}
	return rc;
}

static DEVICE_API(gpio, api_table) = {
	.pin_configure = gpio_pca6416_config,
	.port_get_raw = gpio_pca6416_port_read,
	.port_set_masked_raw = gpio_pca6416_port_set_masked,
	.port_set_bits_raw = gpio_pca6416_port_set_bits,
	.port_clear_bits_raw = gpio_pca6416_port_clear_bits,
	.port_toggle_bits = gpio_pca6416_port_toggle_bits,
	.pin_interrupt_configure = gpio_pca6416_pin_interrupt_configure,
	.manage_callback = gpio_pca6416_manage_callback,
};

#define GPIO_PCA6416_INIT(n)							\
	static const struct pca6416_config pca6416_cfg_##n = {			\
		.i2c = I2C_DT_SPEC_INST_GET(n),					\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
		.interrupt_enabled = DT_INST_NODE_HAS_PROP(n, interrupt_gpios),	\
		.gpio_int = GPIO_DT_SPEC_INST_GET(n, interrupt_gpios),	\
	};									\
										\
	static struct pca6416_drv_data pca6416_drvdata_##n = {			\
		.lock = Z_SEM_INITIALIZER(pca6416_drvdata_##n.lock, 1, 1),	\
		.pin_state.dir = ALL_PINS,					\
		.pin_state.output = ALL_PINS,					\
	};									\
	DEVICE_DT_INST_DEFINE(n,						\
		gpio_pca6416_init,						\
		NULL,								\
		&pca6416_drvdata_##n,						\
		&pca6416_cfg_##n,						\
		POST_KERNEL,							\
		CONFIG_GPIO_PCA6416_INIT_PRIORITY,				\
		&api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PCA6416_INIT)
