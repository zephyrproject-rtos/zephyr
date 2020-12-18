/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gpio_emul

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/gpio/gpio_emul.h>
#include <errno.h>
#include <zephyr.h>

#include "gpio_utils.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_emul);

#define GPIO_EMUL_INT_BITMASK						\
	(GPIO_INT_DISABLE | GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL |	\
	 GPIO_INT_EDGE | GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)

/**
 * @brief GPIO Emulator interrupt capabilities
 *
 * These enumerations are used as a bitmask and allow the GPIO Emulator to
 * model GPIO interrupt controllers with varying interrupt trigger support.
 *
 * For example, some controllers to not support level interrupts,
 * some controllers do not support rising and falling edge simultaneously,
 * etc.
 *
 * This primarily affects the behaviour of @ref gpio_pin_interrupt_configure.
 */
enum gpio_emul_interrupt_cap {
	GPIO_EMUL_INT_CAP_EDGE_RISING = 1,
	GPIO_EMUL_INT_CAP_EDGE_FALLING = 2,
	GPIO_EMUL_INT_CAP_LEVEL_HIGH = 16,
	GPIO_EMUL_INT_CAP_LEVEL_LOW = 32,
};

/**
 * @brief Emulated GPIO controller configuration data
 *
 * This structure contains all of the state for a given emulated GPIO
 * controller as well as all of the pins associated with it.
 *
 * The @a flags member is a pointer to an array which is @a num_pins in size.
 *
 * @a num_pins must be in the range [1, @ref GPIO_MAX_PINS_PER_PORT].
 *
 * Pin direction as well as other pin properties are set using
 * specific bits in @a flags. For more details, see @ref gpio_interface.
 *
 * Changes are synchronized using @ref gpio_emul_data.mu.
 */
struct gpio_emul_config {
	/** Common @ref gpio_driver_config */
	const struct gpio_driver_config common;
	/** Number of pins available in the given GPIO controller instance */
	const gpio_pin_t num_pins;
	/** Supported interrupts */
	const enum gpio_emul_interrupt_cap interrupt_caps;
};

/**
 * @brief Emulated GPIO controller data
 *
 * This structure contains data structures used by a emulated GPIO
 * controller.
 *
 * If the application wishes to specify a "wiring" for the emulated
 * GPIO, then a @ref gpio_callback_handler_t should be registered using
 * @ref gpio_add_callback.
 *
 * Changes are to @ref gpio_emul_data and @ref gpio_emul_config are
 * synchronized using @a mu.
 */
struct gpio_emul_data {
	/** Common @ref gpio_driver_data */
	struct gpio_driver_data common;
	/** Pointer to an array of flags is @a num_pins in size */
	gpio_flags_t *flags;
	/** Input values for each pin */
	gpio_port_value_t input_vals;
	/** Output values for each pin */
	gpio_port_value_t output_vals;
	/** Interrupt status for each pin */
	gpio_port_pins_t interrupts;
	/** Mutex to synchronize accesses to driver data and config */
	struct k_mutex mu;
	/** Singly-linked list of callbacks associated with the controller */
	sys_slist_t callbacks;
};

/**
 * @brief Obtain a mask of pins that match all of the provided @p flags
 *
 * Use this function to see which pins match the current GPIO configuration.
 *
 * The caller must ensure that @ref gpio_emul_data.mu is locked.
 *
 * @param port The emulated GPIO device pointer
 * @param mask A mask of flags to match
 * @param flags The flags to match
 *
 * @return a mask of the pins with matching @p flags
 */
static gpio_port_pins_t
get_pins_with_flags(const struct device *port, gpio_port_pins_t mask,
	gpio_flags_t flags)
{
	size_t i;
	gpio_port_pins_t matched = 0;
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;
	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	for (i = 0; i < config->num_pins; ++i) {
		if ((drv_data->flags[i] & mask) == flags) {
			matched |= BIT(i);
		}
	}

	return matched;
}

/**
 * @brief Obtain a mask of pins that are configured as @ref GPIO_INPUT
 *
 * The caller must ensure that @ref gpio_emul_data.mu is locked.
 *
 * @param port The emulated GPIO device pointer
 *
 * @return a mask of pins that are configured as @ref GPIO_INPUT
 */
static inline gpio_port_pins_t get_input_pins(const struct device *port)
{
	return get_pins_with_flags(port, GPIO_INPUT, GPIO_INPUT);
}

/**
 * @brief Obtain a mask of pins that are configured as @ref GPIO_OUTPUT
 *
 * The caller must ensure that @ref gpio_emul_data.mu is locked.
 *
 * @param port The emulated GPIO device pointer
 *
 * @return a mask of pins that are configured as @ref GPIO_OUTPUT
 */
static inline gpio_port_pins_t get_output_pins(const struct device *port)
{
	return get_pins_with_flags(port, GPIO_OUTPUT, GPIO_OUTPUT);
}

/**
 * Check if @p port has capabilities specified in @p caps
 *
 * @param port The emulated GPIO device pointer
 * @param caps A bitmask of @ref gpio_emul_interrupt_cap
 *
 * @return true if all @p caps are present, otherwise false
 */
static inline bool gpio_emul_config_has_caps(const struct device *port,
		int caps) {

	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	return (caps & config->interrupt_caps) == caps;
}

/*
 * GPIO backend API (for setting input pin values)
 */
static void gpio_emul_gen_interrupt_bits(const struct device *port,
					gpio_port_pins_t mask,
					gpio_port_value_t prev_values,
					gpio_port_value_t values,
					gpio_port_pins_t *interrupts,
					bool detect_edge)
{
	size_t i;
	bool bit;
	bool prev_bit;
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;
	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	for (i = 0, *interrupts = 0; mask && i < config->num_pins;
	     ++i, mask >>= 1, prev_values >>= 1, values >>= 1) {
		if ((mask & 1) == 0) {
			continue;
		}

		prev_bit = ((prev_values & 1) != 0);
		bit = ((values & 1) != 0);

		switch (drv_data->flags[i] & GPIO_EMUL_INT_BITMASK) {
		case GPIO_INT_EDGE_RISING:
			if (gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_EDGE_RISING)) {
				if (detect_edge && !prev_bit && bit) {
					*interrupts |= BIT(i);
				}
			}
			break;
		case GPIO_INT_EDGE_FALLING:
			if (gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_EDGE_FALLING)) {
				if (detect_edge && prev_bit && !bit) {
					*interrupts |= BIT(i);
				}
			}
			break;
		case GPIO_INT_EDGE_BOTH:
			if (gpio_emul_config_has_caps(port,
				GPIO_EMUL_INT_CAP_EDGE_RISING | GPIO_EMUL_INT_CAP_EDGE_FALLING)) {
				if (detect_edge && prev_bit != bit) {
					*interrupts |= BIT(i);
				}
			}
			break;
		case GPIO_INT_LEVEL_LOW:
			if (gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_LEVEL_LOW)) {
				if (!bit) {
					*interrupts |= BIT(i);
				}
			}
			break;
		case GPIO_INT_LEVEL_HIGH:
			if (gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_LEVEL_HIGH)) {
				if (bit) {
					*interrupts |= BIT(i);
				}
			}
			break;
		case 0:
		case GPIO_INT_DISABLE:
			break;
		default:
			LOG_DBG("unhandled case %u",
				drv_data->flags[i] & GPIO_EMUL_INT_BITMASK);
			break;
		}
	}
}

/**
 * @brief Trigger possible interrupt events after an input pin has changed
 *
 * For more information, see @ref gpio_interface.
 *
 * The caller must ensure that @ref gpio_emul_data.mu is locked.
 *
 * @param port The emulated GPIO port
 * @param mask The mask of pins that have changed
 * @param prev_values Previous pin values
 * @param values Current pin values
 */
static void gpio_emul_pend_interrupt(const struct device *port, gpio_port_pins_t mask,
				    gpio_port_value_t prev_values,
				    gpio_port_value_t values)
{
	gpio_port_pins_t interrupts;
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;

	gpio_emul_gen_interrupt_bits(port, mask, prev_values, values,
		&interrupts, true);
	while (interrupts != 0) {
		gpio_fire_callbacks(&drv_data->callbacks, port, interrupts);
		gpio_emul_gen_interrupt_bits(port, mask, prev_values, values,
			&interrupts, false);
	}
}

int gpio_emul_input_set_masked_pend(const struct device *port, gpio_port_pins_t mask,
			      gpio_port_value_t values, bool pend)
{
	int ret;
	gpio_port_pins_t input_mask;
	gpio_port_pins_t prev_values;
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;
	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	if (mask == 0) {
		return 0;
	}

	if (~config->common.port_pin_mask & mask) {
		return -EINVAL;
	}

	if (values & ~mask) {
		return -EINVAL;
	}

	k_mutex_lock(&drv_data->mu, K_FOREVER);

	input_mask = get_input_pins(port);
	if (~input_mask & mask) {
		ret = -EINVAL;
		goto unlock;
	}

	prev_values = drv_data->input_vals;
	drv_data->input_vals &= ~mask;
	drv_data->input_vals |= values;
	values = drv_data->input_vals;

	if (pend) {
		gpio_emul_pend_interrupt(port, mask, prev_values, values);
	}
	ret = 0;

unlock:
	k_mutex_unlock(&drv_data->mu);

	return ret;
}

/* documented in drivers/gpio/gpio_emul.h */
int gpio_emul_input_set_masked(const struct device *port, gpio_port_pins_t mask,
			      gpio_port_value_t values)
{
	return gpio_emul_input_set_masked_pend(port, mask, values, true);
}

/* documented in drivers/gpio/gpio_emul.h */
int gpio_emul_output_get_masked(const struct device *port, gpio_port_pins_t mask,
			       gpio_port_value_t *values)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;
	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	if (mask == 0) {
		return 0;
	}

	if (~config->common.port_pin_mask & mask) {
		return -EINVAL;
	}

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	*values = drv_data->output_vals & get_output_pins(port);
	k_mutex_unlock(&drv_data->mu);

	return 0;
}

/* documented in drivers/gpio/gpio_emul.h */
int gpio_emul_flags_get(const struct device *port, gpio_pin_t pin, gpio_flags_t *flags)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;
	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	if (flags == NULL) {
		return -EINVAL;
	}

	if ((config->common.port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	*flags = drv_data->flags[pin];
	k_mutex_unlock(&drv_data->mu);

	return 0;
}

/*
 * GPIO Driver API
 *
 * API is documented at drivers/gpio.h
 */

static int gpio_emul_pin_configure(const struct device *port, gpio_pin_t pin,
				  gpio_flags_t flags)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;
	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	if (flags & GPIO_OPEN_DRAIN) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	if ((config->common.port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	drv_data->flags[pin] = flags;
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			drv_data->output_vals &= ~BIT(pin);
			if (flags & GPIO_INPUT) {
				/* for push-pull mode to generate interrupts */
				gpio_emul_input_set_masked_pend(port, BIT(pin), drv_data->output_vals, false);
			}
		} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
			drv_data->output_vals |= BIT(pin);
			if (flags & GPIO_INPUT) {
				/* for push-pull mode to generate interrupts */
				gpio_emul_input_set_masked_pend(port, BIT(pin), drv_data->output_vals, false);
			}
		}
	} else if (flags & GPIO_INPUT) {
		if (flags & GPIO_PULL_UP) {
			gpio_emul_input_set_masked_pend(port, BIT(pin), BIT(pin), false);
		} else if (flags & GPIO_PULL_DOWN) {
			gpio_emul_input_set_masked_pend(port, BIT(pin), 0, false);
		}
	}

	k_mutex_unlock(&drv_data->mu);
	gpio_fire_callbacks(&drv_data->callbacks, port, BIT(pin));

	return 0;
}

static int gpio_emul_port_get_raw(const struct device *port, gpio_port_value_t *values)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;

	if (values == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	*values = drv_data->input_vals & get_input_pins(port);
	k_mutex_unlock(&drv_data->mu);

	return 0;
}

static int gpio_emul_port_set_masked_raw(const struct device *port,
					gpio_port_pins_t mask,
					gpio_port_value_t values)
{
	gpio_port_pins_t output_mask;
	gpio_port_pins_t prev_values;
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	output_mask = get_output_pins(port);
	mask &= output_mask;
	prev_values = drv_data->output_vals;
	prev_values &= output_mask;
	values &= output_mask;
	drv_data->output_vals &= ~mask;
	drv_data->output_vals |= values;
	/* in push-pull, set input values & fire interrupts */
	gpio_emul_input_set_masked(port, mask & get_input_pins(port), drv_data->output_vals);
	k_mutex_unlock(&drv_data->mu);
	/* for output-wiring, so the user can take action based on ouput */
	if (prev_values ^ values) {
		gpio_fire_callbacks(&drv_data->callbacks, port, mask & ~get_input_pins(port));
	}

	return 0;
}

static int gpio_emul_port_set_bits_raw(const struct device *port,
				      gpio_port_pins_t pins)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	pins &= get_output_pins(port);
	drv_data->output_vals |= pins;
	/* in push-pull, set input values & fire interrupts */
	gpio_emul_input_set_masked(port, pins & get_input_pins(port), drv_data->output_vals);
	k_mutex_unlock(&drv_data->mu);
	/* for output-wiring, so the user can take action based on ouput */
	gpio_fire_callbacks(&drv_data->callbacks, port, pins & ~get_input_pins(port));

	return 0;
}

static int gpio_emul_port_clear_bits_raw(const struct device *port,
					gpio_port_pins_t pins)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	pins &= get_output_pins(port);
	drv_data->output_vals &= ~pins;
	/* in push-pull, set input values & fire interrupts */
	gpio_emul_input_set_masked(port, pins & get_input_pins(port), drv_data->output_vals);
	k_mutex_unlock(&drv_data->mu);
	/* for output-wiring, so the user can take action based on ouput */
	gpio_fire_callbacks(&drv_data->callbacks, port, pins & ~get_input_pins(port));

	return 0;
}

static int gpio_emul_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;

	k_mutex_lock(&drv_data->mu, K_FOREVER);
	drv_data->output_vals ^= (pins & get_output_pins(port));
	/* in push-pull, set input values but do not fire interrupts (yet) */
	gpio_emul_input_set_masked_pend(port, pins & get_input_pins(port), drv_data->output_vals, false);
	k_mutex_unlock(&drv_data->mu);
	/* for output-wiring, so the user can take action based on ouput */
	gpio_fire_callbacks(&drv_data->callbacks, port, pins);

	return 0;
}

static bool gpio_emul_level_trigger_supported(const struct device *port,
					     enum gpio_int_trig trig)
{
	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		return gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_LEVEL_LOW);
	case GPIO_INT_TRIG_HIGH:
		return gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_LEVEL_HIGH);
	case GPIO_INT_TRIG_BOTH:
		return gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_LEVEL_LOW
			| GPIO_EMUL_INT_CAP_LEVEL_HIGH);
	default:
		return false;
	}
}

static bool gpio_emul_edge_trigger_supported(const struct device *port,
					    enum gpio_int_trig trig)
{
	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		return gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_EDGE_FALLING);
	case GPIO_INT_TRIG_HIGH:
		return gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_EDGE_RISING);
	case GPIO_INT_TRIG_BOTH:
		return gpio_emul_config_has_caps(port, GPIO_EMUL_INT_CAP_EDGE_FALLING
			| GPIO_EMUL_INT_CAP_EDGE_RISING);
	default:
		return false;
	}
}

static int gpio_emul_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	int ret;
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;
	const struct gpio_emul_config *config =
		(const struct gpio_emul_config *)port->config;

	if ((BIT(pin) & config->common.port_pin_mask) == 0) {
		return -EINVAL;
	}

	if (mode != GPIO_INT_MODE_DISABLED) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
		case GPIO_INT_TRIG_HIGH:
		case GPIO_INT_TRIG_BOTH:
			break;
		default:
			return -EINVAL;
		}
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		if (!gpio_emul_level_trigger_supported(port, trig)) {
			return -ENOTSUP;
		}
	}

	if (mode == GPIO_INT_MODE_EDGE) {
		if (!gpio_emul_edge_trigger_supported(port, trig)) {
			return -ENOTSUP;
		}
	}

	k_mutex_lock(&drv_data->mu, K_FOREVER);

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		drv_data->flags[pin] &= ~GPIO_EMUL_INT_BITMASK;
		drv_data->flags[pin] |= GPIO_INT_DISABLE;
		break;
	case GPIO_INT_MODE_LEVEL:
	case GPIO_INT_MODE_EDGE:
		drv_data->flags[pin] &= ~GPIO_EMUL_INT_BITMASK;
		drv_data->flags[pin] |= (mode | trig);
		break;
	default:
		ret = -EINVAL;
		goto unlock;
	}

	ret = 0;

unlock:
	k_mutex_unlock(&drv_data->mu);

	return ret;
}

static int gpio_emul_manage_callback(const struct device *port,
				    struct gpio_callback *cb, bool set)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)port->data;

	return gpio_manage_callback(&drv_data->callbacks, cb, set);
}

static gpio_port_pins_t gpio_emul_get_pending_int(const struct device *dev)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)dev->data;

	return drv_data->interrupts;
}

static const struct gpio_driver_api gpio_emul_driver = {
	.pin_configure = gpio_emul_pin_configure,
	.port_get_raw = gpio_emul_port_get_raw,
	.port_set_masked_raw = gpio_emul_port_set_masked_raw,
	.port_set_bits_raw = gpio_emul_port_set_bits_raw,
	.port_clear_bits_raw = gpio_emul_port_clear_bits_raw,
	.port_toggle_bits = gpio_emul_port_toggle_bits,
	.pin_interrupt_configure = gpio_emul_pin_interrupt_configure,
	.manage_callback = gpio_emul_manage_callback,
	.get_pending_int = gpio_emul_get_pending_int,
};

static int gpio_emul_init(const struct device *dev)
{
	struct gpio_emul_data *drv_data =
		(struct gpio_emul_data *)dev->data;

	sys_slist_init(&drv_data->callbacks);
	return k_mutex_init(&drv_data->mu);
}

/*
 * Device Initialization
 */

#define GPIO_EMUL_INT_CAPS(_num) (0					\
	+ DT_INST_PROP(_num, rising_edge)				\
		* GPIO_EMUL_INT_CAP_EDGE_RISING				\
	+ DT_INST_PROP(_num, falling_edge)				\
		* GPIO_EMUL_INT_CAP_EDGE_FALLING			\
	+ DT_INST_PROP(_num, high_level)				\
		* GPIO_EMUL_INT_CAP_LEVEL_HIGH				\
	+ DT_INST_PROP(_num, low_level)					\
		* GPIO_EMUL_INT_CAP_LEVEL_LOW				\
	)

#define DEFINE_GPIO_EMUL(_num)						\
									\
	static gpio_flags_t						\
		gpio_emul_flags_##_num[DT_INST_PROP(_num, ngpios)];	\
									\
	static const struct gpio_emul_config gpio_emul_config_##_num = {\
		.common = {						\
			.port_pin_mask =				\
				GPIO_PORT_PIN_MASK_FROM_DT_INST(_num),	\
		},							\
		.num_pins = DT_INST_PROP(_num, ngpios),			\
		.interrupt_caps = GPIO_EMUL_INT_CAPS(_num)		\
	};								\
									\
	static struct gpio_emul_data gpio_emul_data_##_num = {		\
		.flags = gpio_emul_flags_##_num,			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(_num, gpio_emul_init,			\
			    device_pm_control_nop,			\
			    &gpio_emul_data_##_num,			\
			    &gpio_emul_config_##_num, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &gpio_emul_driver)

DT_INST_FOREACH_STATUS_OKAY(DEFINE_GPIO_EMUL);
