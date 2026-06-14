/*
 * Copyright (c) 2021, Yonatan Schachter
 * Copyright (c) 2025, Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#include "gpio_rpi_pico.h"

#if GPIO_RPI_HI_AVAILABLE
#define PORT_NO(port) ((((struct gpio_rpi_config *)port->config)->high_dev != NULL) ? 0 : 1)
#else
#define PORT_NO(port) ((uintptr_t)port & 0) /* generate zero and suppress unused warning */
#endif

static int gpio_rpi_configure(const struct device *dev,
				gpio_pin_t pin,
				gpio_flags_t flags)
{
	const int offset = GPIO_RPI_PINS_PER_PORT * PORT_NO(dev);
	struct gpio_rpi_data *data = dev->data;

	if (flags == GPIO_DISCONNECTED) {
		gpio_set_pulls(pin + offset, false, false);
		/* This is almost the opposite of the Pico SDK's gpio_set_function. */
		gpio_set_input_enabled_output_disabled(pin + offset, false, true);
#ifdef CONFIG_SOC_SERIES_RP2350
		hw_set_bits(&pads_bank0_hw->io[pin + offset], PADS_BANK0_GPIO0_ISO_BITS);
#endif
		return 0;
	}

	gpio_set_pulls(pin + offset, (flags & GPIO_PULL_UP) != 0U, (flags & GPIO_PULL_DOWN) != 0U);

	/* Avoid gpio_init, since that also clears previously set direction/high/low */
	gpio_set_function(pin + offset, GPIO_FUNC_SIO);

	if (flags & GPIO_INPUT) {
		gpio_set_dir(pin + offset, GPIO_IN);
	} else {
		gpio_set_input_enabled(pin + offset, false);
	}

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			data->single_ended_mask |= BIT(pin);

			/* Setting the initial state of output data, and output enable.
			 * The output data will not change from here on, only output
			 * enable will. If none of the GPIO_OUTPUT_INIT_* flags have
			 * been set then fall back to the non-agressive input mode.
			 */
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				data->open_drain_mask |= BIT(pin);
				gpio_put(pin + offset, 0);
				gpio_set_dir(pin + offset,
					     (flags & GPIO_OUTPUT_INIT_LOW) ? GPIO_OUT : GPIO_IN);
			} else {
				data->open_drain_mask &= ~(BIT(pin));
				gpio_put(pin + offset, 1);
				gpio_set_dir(pin + offset,
					     (flags & GPIO_OUTPUT_INIT_HIGH) ? GPIO_OUT : GPIO_IN);
			}
		} else {
			data->single_ended_mask &= ~(BIT(pin));
			if (flags & GPIO_OUTPUT_INIT_HIGH) {
				gpio_put(pin + offset, 1);
			} else if (flags & GPIO_OUTPUT_INIT_LOW) {
				gpio_put(pin + offset, 0);
			}
			gpio_set_dir(pin + offset, GPIO_OUT);
		}
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_rpi_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const int offset = GPIO_RPI_PINS_PER_PORT * PORT_NO(dev);
	struct gpio_rpi_data *data = dev->data;

	*flags = 0;

	/* RP2xxxx supports Bus Keeper mode where both pull-up and pull-down are enabled. */
	if (gpio_is_pulled_up(pin + offset)) {
		*flags |= GPIO_PULL_UP;
	}
	if (gpio_is_pulled_down(pin + offset)) {
		*flags |= GPIO_PULL_DOWN;
	}

	if (gpio_get_dir(pin + offset)) {
		*flags |= gpio_get_out_level(pin + offset) ? GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW;
		if (data->single_ended_mask & BIT(pin)) {
			*flags |=
				data->open_drain_mask & BIT(pin) ? GPIO_OPEN_DRAIN : GPIO_PUSH_PULL;
		}
	}

	if (gpio_is_input_enabled(pin + offset)) {
		*flags |= GPIO_INPUT;
	}

	return 0;
}
#endif

static int gpio_rpi_port_get_raw(const struct device *port, uint32_t *value)
{
	*value = gpio_get_all_n(PORT_NO(port));

	return 0;
}

static int gpio_rpi_port_set_masked_raw(const struct device *port,
					uint32_t mask, uint32_t value)
{
	struct gpio_rpi_data *data = port->data;

	/* First handle push-pull pins: */
	gpio_put_masked_n(PORT_NO(port), mask & ~data->single_ended_mask, value);
	/* Then handle open-drain pins: */
	gpio_set_dir_masked_n(PORT_NO(port), mask & data->single_ended_mask & data->open_drain_mask,
			      ~value);
	/* Then handle open-source pins: */
	gpio_set_dir_masked_n(PORT_NO(port),
			      mask & data->single_ended_mask & ~data->open_drain_mask, value);

	return 0;
}

static int gpio_rpi_port_set_bits_raw(const struct device *port,
					uint32_t pins)
{
	struct gpio_rpi_data *data = port->data;

	/* First handle push-pull pins: */
	gpio_set_mask_n(PORT_NO(port), pins & ~data->single_ended_mask);
	/* Then handle open-drain pins: */
	gpio_set_dir_in_masked_n(PORT_NO(port),
				 pins & data->single_ended_mask & data->open_drain_mask);
	/* Then handle open-source pins: */
	gpio_set_dir_out_masked_n(PORT_NO(port),
				  pins & data->single_ended_mask & ~data->open_drain_mask);

	return 0;
}

static int gpio_rpi_port_clear_bits_raw(const struct device *port,
					uint32_t pins)
{
	struct gpio_rpi_data *data = port->data;

	/* First handle push-pull pins: */
	gpio_clr_mask_n(PORT_NO(port), pins & ~data->single_ended_mask);
	/* Then handle open-drain pins: */
	gpio_set_dir_out_masked_n(PORT_NO(port),
				  pins & data->single_ended_mask & data->open_drain_mask);
	/* Then handle open-source pins: */
	gpio_set_dir_in_masked_n(PORT_NO(port),
				 pins & data->single_ended_mask & ~data->open_drain_mask);

	return 0;
}

static int gpio_rpi_port_toggle_bits(const struct device *port,
					uint32_t pins)
{
	struct gpio_rpi_data *data = port->data;

	/* First handle push-pull pins: */
	gpio_xor_mask_n(PORT_NO(port), pins & ~data->single_ended_mask);
	/* Then handle single-ended pins: */
	gpio_toggle_dir_masked_n(PORT_NO(port), pins & data->single_ended_mask);

	return 0;
}

static int gpio_rpi_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	const struct gpio_rpi_config *config = dev->config;
	const int offset = GPIO_RPI_PINS_PER_PORT * PORT_NO(dev);
	uint32_t events = 0;

	if (!config->has_irq) {
		return -ENOTSUP;
	}

	gpio_set_irq_enabled(pin + offset, ALL_EVENTS, false);
	if (mode != GPIO_INT_DISABLE) {
		if (mode & GPIO_INT_EDGE) {
			if (trig & GPIO_INT_LOW_0) {
				events |= GPIO_IRQ_EDGE_FALL;
			}
			if (trig & GPIO_INT_HIGH_1) {
				events |= GPIO_IRQ_EDGE_RISE;
			}
		} else {
			if (trig & GPIO_INT_LOW_0) {
				events |= GPIO_IRQ_LEVEL_LOW;
			}
			if (trig & GPIO_INT_HIGH_1) {
				events |= GPIO_IRQ_LEVEL_HIGH;
			}
		}
		gpio_set_irq_enabled(pin + offset, events, true);
	}

	return 0;
}

static int gpio_rpi_manage_callback(const struct device *dev,
				struct gpio_callback *callback, bool set)
{
	struct gpio_rpi_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_rpi_get_pending_int(const struct device *dev)
{
	return gpio_has_pending_irq();
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_rpi_port_get_direction(const struct device *port, gpio_port_pins_t map,
				  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const int offset = GPIO_RPI_PINS_PER_PORT * PORT_NO(port);

	/* The Zephyr API considers a disconnected pin to be neither an input nor output.
	 * Since we disable both OE and IE for disconnected pins clear the mask bits.
	 */
	for (int pin = 0; pin < NUM_BANK0_GPIOS; pin++) {
		if (gpio_is_output_disabled(pin + offset)) {
			map &= ~BIT(pin);
		}
		if (inputs && gpio_is_input_enabled(pin + offset)) {
			*inputs |= BIT(pin);
		}
	}
	if (inputs) {
		*inputs &= map;
	}
	if (outputs) {
		*outputs = gpio_get_dir_all_bits_n(PORT_NO(port)) & map;
	}

	return 0;
}
#endif

static DEVICE_API(gpio, gpio_rpi_driver_api) = {
	.pin_configure = gpio_rpi_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_rpi_get_config,
#endif
	.port_get_raw = gpio_rpi_port_get_raw,
	.port_set_masked_raw = gpio_rpi_port_set_masked_raw,
	.port_set_bits_raw = gpio_rpi_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rpi_port_clear_bits_raw,
	.port_toggle_bits = gpio_rpi_port_toggle_bits,
	.pin_interrupt_configure = gpio_rpi_pin_interrupt_configure,
	.manage_callback = gpio_rpi_manage_callback,
	.get_pending_int = gpio_rpi_get_pending_int,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_rpi_port_get_direction,
#endif
};

__maybe_unused static void gpio_rpi_isr(const struct device *dev)
{
	struct gpio_rpi_data *data = dev->data;

	for (uint32_t pin = 0; pin < NUM_BANK0_GPIOS; pin++) {
		if (gpio_get_irq_event_mask(pin)) {
			gpio_acknowledge_irq(pin, ALL_EVENTS);

#if GPIO_RPI_HI_AVAILABLE
			if (pin >= GPIO_RPI_PINS_PER_PORT) {
				const struct gpio_rpi_config *config = dev->config;
				struct gpio_rpi_data *high_data = config->high_dev->data;

				gpio_fire_callbacks(&high_data->callbacks, config->high_dev,
						    BIT(pin - GPIO_RPI_PINS_PER_PORT));
				continue;
			}
#endif

			gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
		}
	}
}

static int gpio_rpi_bank_init(const struct device *dev)
{
	const struct gpio_rpi_config *config = dev->config;

	if (config->bank_config_func != NULL) {
		config->bank_config_func();
	}

	return 0;
}

#define DEVICE_IF_GPIO_RPI_HI_NODE(node_id)                                                        \
	COND_CODE_1(IS_GPIO_RPI_LO_NODE(node_id), (), (DEVICE_DT_GET(node_id)))

#define FIND_GPIO_RPI_HI_DEVICE(node_id)                                                           \
	COND_CODE_1(UTIL_CAT(GPIO_REG_, DT_REG_ADDR(node_id)),                                     \
			(DT_FOREACH_CHILD(DT_PARENT(node_id), DEVICE_IF_GPIO_RPI_HI_NODE)), (NULL))

#if GPIO_RPI_HI_AVAILABLE
#define GPIO_RPI_INIT_HIGH_DEV(node_id) .high_dev = FIND_GPIO_RPI_HI_DEVICE(node_id),
#else
#define GPIO_RPI_INIT_HIGH_DEV(node_id)
#endif

#define GPIO_RPI_IRQ_FLAGS(node_id)                                                                \
	COND_CODE_1(DT_IRQ_HAS_CELL(DT_PARENT(node_id), flags),                                    \
		    (DT_IRQ(DT_PARENT(node_id), flags)), (0))

/*
 * GPIO banks keep their register blocks on the parent node. Only bank0 is
 * supported, so the parent regions are gpio0/sio0/rio0/pads0.
 */
#if CONFIG_DT_HAS_RASPBERRYPI_RP1_GPIO_ENABLED
#define GPIO_RPI_MMIO_NAMES gpio, rio, pads
#else
#define GPIO_RPI_MMIO_NAMES gpio, sio, pads
#endif

#define GPIO_RPI_MMIO_INIT(name, node_id)                                                          \
	.name = Z_DEVICE_MMIO_NAMED_ROM_INITIALIZER(UTIL_CAT(name, 0), DT_PARENT(node_id)),

#define GPIO_RPI_MMIO_MAP(name, dev) DEVICE_MMIO_NAMED_MAP(dev, name, K_MEM_CACHE_NONE);

#define GPIO_RPI_MMIO_MAP_ALL(node_id)                                                             \
	const struct device *dev __maybe_unused = DEVICE_DT_GET(node_id);                          \
	FOR_EACH_FIXED_ARG(GPIO_RPI_MMIO_MAP, (), dev, GPIO_RPI_MMIO_NAMES)

#define GPIO_RPI_MMIO_INIT_ALL(node_id)                                                            \
	FOR_EACH_FIXED_ARG(GPIO_RPI_MMIO_INIT, (), node_id, GPIO_RPI_MMIO_NAMES)

#define GPIO_RPI_COMMON_INIT(node_id)                                                              \
	static void gpio_rpi_bank_config_##node_id(void)                                           \
	{                                                                                          \
		GPIO_RPI_MMIO_MAP_ALL(node_id)                                                     \
		IF_ENABLED(IS_GPIO_RPI_LO_NODE(node_id), (                                         \
			(void)gpio_rpi_hal_irq_setup();                                            \
			IF_ENABLED(DT_IRQ_HAS_IDX(DT_PARENT(node_id), 0), (                        \
				IRQ_CONNECT(DT_IRQN(DT_PARENT(node_id)),                           \
					    DT_IRQ(DT_PARENT(node_id), priority),                  \
					    gpio_rpi_isr, DEVICE_DT_GET(node_id),                  \
					    GPIO_RPI_IRQ_FLAGS(node_id));                          \
				irq_enable(DT_IRQN(DT_PARENT(node_id)));                           \
			))                                                                         \
		))                                                                                 \
	}                                                                                          \
	static const struct gpio_rpi_config gpio_rpi_##node_id##_config = {                        \
		.common = {                                                                        \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(node_id),                 \
		},                                                                                 \
		.ngpios = DT_PROP(node_id, ngpios),                                                \
		.has_irq = DT_IRQ_HAS_IDX(DT_PARENT(node_id), 0),                                  \
		.bank_config_func = gpio_rpi_bank_config_##node_id,                                \
		GPIO_RPI_MMIO_INIT_ALL(node_id)                                                    \
		GPIO_RPI_INIT_HIGH_DEV(node_id)                                                    \
	};                                                                                         \
	static struct gpio_rpi_data gpio_rpi_##node_id##_data;                                     \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, gpio_rpi_bank_init, NULL, &gpio_rpi_##node_id##_data,            \
			 &gpio_rpi_##node_id##_config, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,     \
			 &gpio_rpi_driver_api);

#define GPIO_RP1_INIT(node_id)                                                                     \
	BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(raspberrypi_rp1_gpio) == 1,                           \
		     "raspberrypi,rp1-gpio supports only a single enabled GPIO bank.");            \
	BUILD_ASSERT(DT_REG_ADDR(node_id) == 0,                                                    \
		     "Only RP1 GPIO bank 0 may be used as GPIO.");                                 \
	GPIO_RPI_COMMON_INIT(node_id)

#define GPIO_RPI_PICO_INIT(node_id)                                                                \
	BUILD_ASSERT(DT_CHILD_NUM(DT_PARENT(node_id)) > 0 &&                                       \
			     DT_CHILD_NUM(DT_PARENT(node_id)) <= 2,                                \
		     "raspberrypi,pico-gpio node must have one or two child node.");               \
	BUILD_ASSERT(GPIO_RPI_LO_AVAILABLE,                                                        \
		     "raspberrypi,pico-gpio node must have reg=0 child node.");                    \
	GPIO_RPI_COMMON_INIT(node_id)

DT_FOREACH_STATUS_OKAY(raspberrypi_rp1_gpio, GPIO_RP1_INIT)
DT_FOREACH_STATUS_OKAY(raspberrypi_pico_gpio_port, GPIO_RPI_PICO_INIT)
