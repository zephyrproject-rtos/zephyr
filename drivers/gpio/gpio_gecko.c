/*
 * Copyright (c) 2017, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_gpio_port

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <em_gpio.h>
#ifdef CONFIG_SOC_GECKO_DEV_INIT
#include <em_cmu.h>
#endif

#include <zephyr/drivers/gpio/gpio_utils.h>

#if CONFIG_GPIO_GECKO_COMMON_INIT_PRIORITY >= CONFIG_GPIO_INIT_PRIORITY
#error CONFIG_GPIO_GECKO_COMMON_INIT_PRIORITY must be less than \
	CONFIG_GPIO_INIT_PRIORITY.
#endif

#if DT_NODE_HAS_PROP(id, peripheral_id)
#define GET_GECKO_GPIO_INDEX(id) DT_INST_PROP(id, peripheral_id)
#else
#if defined(CONFIG_SOC_SERIES_EFR32BG22) || \
	defined(CONFIG_SOC_SERIES_EFR32BG27) || \
	defined(CONFIG_SOC_SERIES_EFR32MG21) || \
	defined(CONFIG_SOC_SERIES_EFR32MG24)
#define GECKO_GPIO_PORT_ADDR_SPACE_SIZE sizeof(GPIO_PORT_TypeDef)
#else
#define GECKO_GPIO_PORT_ADDR_SPACE_SIZE sizeof(GPIO_P_TypeDef)
#endif
/* Assumption for calculating gpio index:
 * 1. Address space of the first GPIO port is the address space for GPIO port A
 */
#define GET_GECKO_GPIO_INDEX(id) (DT_INST_REG_ADDR(id) - DT_REG_ADDR(DT_NODELABEL(gpioa))) \
	/ GECKO_GPIO_PORT_ADDR_SPACE_SIZE
#endif /* DT_NODE_HAS_PROP(id, peripheral_id) */

/*
 * Macros to set the GPIO MODE registers
 *
 * See https://www.silabs.com/documents/public/reference-manuals/EFM32WG-RM.pdf
 * pages 972 and 982.
 */
/**
 * @brief Create the value to set the GPIO MODEL register
 * @param[in] pin The index of the pin. Valid values are 0..7.
 * @param[in] mode The mode that should be set.
 * @return The value that can be set into the GPIO MODEL register.
 */
#define GECKO_GPIO_MODEL(pin, mode) (mode << (pin * 4))

/**
 * @brief Create the value to set the GPIO MODEH register
 * @param[in] pin The index of the pin. Valid values are 8..15.
 * @param[in] mode The mode that should be set.
 * @return The value that can be set into the GPIO MODEH register.
 */
#define GECKO_GPIO_MODEH(pin, mode) (mode << ((pin - 8) * 4))


#define NUMBER_OF_PORTS (SIZEOF_FIELD(GPIO_TypeDef, P) / \
			 SIZEOF_FIELD(GPIO_TypeDef, P[0]))

struct gpio_gecko_common_config {
};

struct gpio_gecko_common_data {
	/* a list of all ports */
	const struct device *ports[NUMBER_OF_PORTS];
	size_t count;
};

struct gpio_gecko_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Port_TypeDef gpio_index;
};

struct gpio_gecko_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* mask of pins on which interrupt is enabled */
	uint32_t int_enabled_mask;
};

static inline void gpio_gecko_add_port(struct gpio_gecko_common_data *data,
				       const struct device *dev)
{
	__ASSERT(dev, "No port device!");
	data->ports[data->count++] = dev;
}

static int gpio_gecko_configure(const struct device *dev,
				gpio_pin_t pin,
				gpio_flags_t flags)
{
	const struct gpio_gecko_config *config = dev->config;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;
	GPIO_Mode_TypeDef mode;
	unsigned int out = 0U;

	if (flags & GPIO_OUTPUT) {
		/* Following modes enable both output and input */
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				mode = gpioModeWiredAnd;
			} else {
				mode = gpioModeWiredOr;
			}
		} else {
			mode = gpioModePushPull;
		}
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			out = 1U;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			out = 0U;
		} else {
			out = GPIO_PinOutGet(gpio_index, pin);
		}
	} else if (flags & GPIO_INPUT) {
		if (flags & GPIO_PULL_UP) {
			mode = gpioModeInputPull;
			out = 1U; /* pull-up*/
		} else if (flags & GPIO_PULL_DOWN) {
			mode = gpioModeInputPull;
			/* out = 0 means pull-down*/
		} else {
			mode = gpioModeInput;
		}
	} else {
		/* Neither input nor output mode is selected */
		mode = gpioModeDisabled;
	}
	/* The flags contain options that require touching registers in the
	 * GPIO module and the corresponding PORT module.
	 *
	 * Start with the GPIO module and set up the pin direction register.
	 * 0 - pin is input, 1 - pin is output
	 */

	GPIO_PinModeSet(gpio_index, pin, mode, out);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_gecko_get_config(const struct device *dev,
				 gpio_pin_t pin,
				 gpio_flags_t *out_flags)
{
	const struct gpio_gecko_config *config = dev->config;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;
	GPIO_Mode_TypeDef mode;
	unsigned int out;
	gpio_flags_t flags = 0;

	mode = GPIO_PinModeGet(gpio_index, pin);
	out = GPIO_PinOutGet(gpio_index, pin);

	switch (mode) {
	case gpioModeWiredAnd:
		flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN;

		if (out) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}

		break;
	case gpioModeWiredOr:
		flags = GPIO_OUTPUT | GPIO_OPEN_SOURCE;

		if (out) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}

		break;
	case gpioModePushPull:
		flags = GPIO_OUTPUT | GPIO_PUSH_PULL;

		if (out) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}

		break;
	case gpioModeInputPull:
		flags = GPIO_INPUT;

		if (out) {
			flags |= GPIO_PULL_UP;
		} else {
			flags |= GPIO_PULL_DOWN;
		}

		break;
	case gpioModeInput:
		flags = GPIO_INPUT;
		break;
	case gpioModeDisabled:
		flags = GPIO_DISCONNECTED;
		break;
	default:
		break;
	}

	*out_flags = flags;

	return 0;
}
#endif

static int gpio_gecko_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_gecko_config *config = dev->config;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;

	*value = GPIO_PortInGet(gpio_index);

	return 0;
}

static int gpio_gecko_port_set_masked_raw(const struct device *dev,
					  uint32_t mask,
					  uint32_t value)
{
	const struct gpio_gecko_config *config = dev->config;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;

	GPIO_PortOutSetVal(gpio_index, value, mask);

	return 0;
}

static int gpio_gecko_port_set_bits_raw(const struct device *dev,
					uint32_t mask)
{
	const struct gpio_gecko_config *config = dev->config;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;

	GPIO_PortOutSet(gpio_index, mask);

	return 0;
}

static int gpio_gecko_port_clear_bits_raw(const struct device *dev,
					  uint32_t mask)
{
	const struct gpio_gecko_config *config = dev->config;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;

	GPIO_PortOutClear(gpio_index, mask);

	return 0;
}

static int gpio_gecko_port_toggle_bits(const struct device *dev,
				       uint32_t mask)
{
	const struct gpio_gecko_config *config = dev->config;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;

	GPIO_PortOutToggle(gpio_index, mask);

	return 0;
}

static int gpio_gecko_pin_interrupt_configure(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct gpio_gecko_config *config = dev->config;
	struct gpio_gecko_data *data = dev->data;

	/* Interrupt on static level is not supported by the hardware */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		GPIO_IntDisable(BIT(pin));
	} else {
		/* Interrupt line is already in use */
		if ((GPIO->IEN & BIT(pin)) != 0) {
			/* TODO: Return an error only if request is done for
			 * a pin from a different port.
			 */
			return -EBUSY;
		}

		bool rising_edge = true;
		bool falling_edge = true;

		if (trig == GPIO_INT_TRIG_LOW) {
			rising_edge = false;
			falling_edge = true;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			rising_edge = true;
			falling_edge = false;
		} /* default is GPIO_INT_TRIG_BOTH */

		GPIO_ExtIntConfig(config->gpio_index, pin, pin,
			       rising_edge, falling_edge, true);
	}

	WRITE_BIT(data->int_enabled_mask, pin, mode != GPIO_INT_DISABLE);

	return 0;
}

static int gpio_gecko_manage_callback(const struct device *dev,
				      struct gpio_callback *callback, bool set)
{
	struct gpio_gecko_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

/**
 * Handler for both odd and even pin interrupts
 */
static void gpio_gecko_common_isr(const struct device *dev)
{
	struct gpio_gecko_common_data *data = dev->data;
	uint32_t enabled_int, int_status;
	const struct device *port_dev;
	struct gpio_gecko_data *port_data;

	int_status = GPIO->IF;

	for (unsigned int i = 0; int_status && (i < data->count); i++) {
		port_dev = data->ports[i];
		port_data = port_dev->data;
		enabled_int = int_status & port_data->int_enabled_mask;
		if (enabled_int != 0) {
			int_status &= ~enabled_int;
#if defined(_SILICON_LABS_32B_SERIES_2)
			GPIO->IF_CLR = enabled_int;
#else
			GPIO->IFC = enabled_int;
#endif
			gpio_fire_callbacks(&port_data->callbacks, port_dev,
					    enabled_int);
		}
	}
}

static DEVICE_API(gpio, gpio_gecko_driver_api) = {
	.pin_configure = gpio_gecko_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_gecko_get_config,
#endif
	.port_get_raw = gpio_gecko_port_get_raw,
	.port_set_masked_raw = gpio_gecko_port_set_masked_raw,
	.port_set_bits_raw = gpio_gecko_port_set_bits_raw,
	.port_clear_bits_raw = gpio_gecko_port_clear_bits_raw,
	.port_toggle_bits = gpio_gecko_port_toggle_bits,
	.pin_interrupt_configure = gpio_gecko_pin_interrupt_configure,
	.manage_callback = gpio_gecko_manage_callback,
};

static DEVICE_API(gpio, gpio_gecko_common_driver_api) = {
	.manage_callback = gpio_gecko_manage_callback,
};

static int gpio_gecko_common_init(const struct device *dev);

static const struct gpio_gecko_common_config gpio_gecko_common_config = {
};

static struct gpio_gecko_common_data gpio_gecko_common_data;

DEVICE_DT_DEFINE(DT_INST(0, silabs_gecko_gpio),
		    gpio_gecko_common_init,
		    NULL,
		    &gpio_gecko_common_data, &gpio_gecko_common_config,
		    PRE_KERNEL_1, CONFIG_GPIO_GECKO_COMMON_INIT_PRIORITY,
		    &gpio_gecko_common_driver_api);

static int gpio_gecko_common_init(const struct device *dev)
{
#ifdef CONFIG_SOC_GECKO_DEV_INIT
	CMU_ClockEnable(cmuClock_GPIO, true);
#endif
	gpio_gecko_common_data.count = 0;
	IRQ_CONNECT(GPIO_EVEN_IRQn,
		    DT_IRQ_BY_NAME(DT_INST(0, silabs_gecko_gpio), gpio_even, priority),
		    gpio_gecko_common_isr,
		    DEVICE_DT_GET(DT_INST(0, silabs_gecko_gpio)), 0);

	IRQ_CONNECT(GPIO_ODD_IRQn,
		    DT_IRQ_BY_NAME(DT_INST(0, silabs_gecko_gpio), gpio_odd, priority),
		    gpio_gecko_common_isr,
		    DEVICE_DT_GET(DT_INST(0, silabs_gecko_gpio)), 0);

	irq_enable(GPIO_EVEN_IRQn);
	irq_enable(GPIO_ODD_IRQn);

	return 0;
}

#define GPIO_PORT_INIT(idx) \
static int gpio_gecko_port##idx##_init(const struct device *dev); \
\
static const struct gpio_gecko_config gpio_gecko_port##idx##_config = { \
	.common = { \
		.port_pin_mask = (gpio_port_pins_t)(-1), \
	}, \
	.gpio_index = GET_GECKO_GPIO_INDEX(idx), \
}; \
\
static struct gpio_gecko_data gpio_gecko_port##idx##_data; \
\
DEVICE_DT_INST_DEFINE(idx, \
		    gpio_gecko_port##idx##_init, \
		    NULL, \
		    &gpio_gecko_port##idx##_data, \
		    &gpio_gecko_port##idx##_config, \
		    POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, \
		    &gpio_gecko_driver_api); \
\
static int gpio_gecko_port##idx##_init(const struct device *dev) \
{ \
	gpio_gecko_add_port(&gpio_gecko_common_data, dev); \
	return 0; \
}

DT_INST_FOREACH_STATUS_OKAY(GPIO_PORT_INIT)
