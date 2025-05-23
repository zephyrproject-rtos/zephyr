/*
 * Copyright (c) 2021, Yonatan Schachter
 * Copyright (c) 2025, Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>

/* pico-sdk includes */
#include <hardware/gpio.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/iobank0.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#define DT_DRV_COMPAT raspberrypi_pico_gpio_port

#define ALL_EVENTS (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE \
		| GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH)

#define GPIO_RPI_PINS_PER_PORT 32

#define ADDR_IS_ZERO(n, x)     (DT_INST_REG_ADDR(n) == 0) |
#define ADDR_IS_NON_ZERO(n, x) (DT_INST_REG_ADDR(n) != 0) |
#define GPIO_RPI_LO_AVAILABLE  (DT_INST_FOREACH_STATUS_OKAY_VARGS(ADDR_IS_ZERO) 0)
#define GPIO_RPI_HI_AVAILABLE  (DT_INST_FOREACH_STATUS_OKAY_VARGS(ADDR_IS_NON_ZERO) 0)

#if GPIO_RPI_HI_AVAILABLE
#define PORT_NO(port) ((((struct gpio_rpi_config *)port->config)->high_dev != NULL) ? 0 : 1)
#else
#define PORT_NO(port) ((int)port & 0) /* generate zero and suppress unused warning */
#endif

struct gpio_rpi_config {
	struct gpio_driver_config common;
	void (*bank_config_func)(void);
#if GPIO_RPI_HI_AVAILABLE
	const struct device *high_dev;
#endif
};

struct gpio_rpi_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	uint32_t single_ended_mask;
	uint32_t open_drain_mask;
};

static inline void gpio_set_dir_out_masked_n(uint n, uint32_t mask)
{
	if (!n) {
		gpio_set_dir_out_masked(mask);
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_set(mask);
#else
		sio_hw->gpio_hi_oe_set = mask;
#endif
	}
}

static inline void gpio_set_dir_in_masked_n(uint n, uint32_t mask)
{
	if (!n) {
		gpio_set_dir_in_masked(mask);
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_clr(mask);
#else
		sio_hw->gpio_hi_oe_clr = mask;
#endif
	}
}

static inline void gpio_set_dir_masked_n(uint n, uint32_t mask, uint32_t value)
{
	if (!n) {
		gpio_set_dir_masked(mask, value);
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_xor((gpioc_hi_oe_get() ^ value) & mask);
#else
		sio_hw->gpio_oe_togl = (sio_hw->gpio_hi_oe ^ value) & mask;
#endif
	}
}

static inline uint32_t gpio_get_all_n(uint n)
{
	if (!n) {
		return gpio_get_all();
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		return gpioc_hi_in_get();
#else
		return sio_hw->gpio_hi_in;
#endif
	}

	return 0;
}

static inline void gpio_toggle_dir_masked_n(uint n, uint32_t mask)
{
	if (!n) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_lo_oe_xor(mask);
#else
		sio_hw->gpio_oe_togl = mask;
#endif
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_xor(mask);
#else
		sio_hw->gpio_hi_oe_togl = mask;
#endif
	}
}

static int gpio_rpi_configure(const struct device *dev,
				gpio_pin_t pin,
				gpio_flags_t flags)
{
	const int offset = GPIO_RPI_PINS_PER_PORT * PORT_NO(dev);
	struct gpio_rpi_data *data = dev->data;

	if (flags == GPIO_DISCONNECTED) {
		gpio_disable_pulls(pin);
		/* This is almost the opposite of the Pico SDK's gpio_set_function. */
		hw_write_masked(&pads_bank0_hw->io[pin], PADS_BANK0_GPIO0_OD_BITS,
				PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
#ifdef SOC_SERIES_RP2350
		hw_set_bits(&pads_bank0_hw->io[gpio], PADS_BANK0_GPIO0_ISO_BITS);
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
				gpio_set_dir(pin + offset, flags & GPIO_OUTPUT_INIT_LOW);
			} else {
				data->open_drain_mask &= ~(BIT(pin));
				gpio_put(pin + offset, 1);
				gpio_set_dir(pin + offset, flags & GPIO_OUTPUT_INIT_HIGH);
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

	if (pads_bank0_hw->io[pin] & PADS_BANK0_GPIO0_IE_BITS) {
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
	const int offset = GPIO_RPI_PINS_PER_PORT * PORT_NO(dev);
	uint32_t events = 0;

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
	io_bank0_irq_ctrl_hw_t *irq_ctrl_base =
		get_core_num() ? &io_bank0_hw->proc1_irq_ctrl : &io_bank0_hw->proc0_irq_ctrl;
	ARRAY_FOR_EACH_PTR(irq_ctrl_base->ints, p) {
		if (*p) {
			return 1;
		}
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_rpi_port_get_direction(const struct device *port, gpio_port_pins_t map,
				  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	/* The Zephyr API considers a disconnected pin to be neither an input nor output.
	 * Since we disable both OE and IE for disconnected pins clear the mask bits.
	 */
	for (int pin = 0; pin < NUM_BANK0_GPIOS; pin++) {
		if (pads_bank0_hw->io[pin] & PADS_BANK0_GPIO0_OD_BITS) {
			map &= ~BIT(pin);
		}
		if (inputs && (pads_bank0_hw->io[pin] & PADS_BANK0_GPIO0_IE_BITS)) {
			*inputs |= BIT(pin);
		}
	}
	if (inputs) {
		*inputs &= map;
	}
	if (outputs) {
		*outputs = sio_hw->gpio_oe & map;
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

static void gpio_rpi_isr(const struct device *dev)
{
	struct gpio_rpi_data *data = dev->data;
	io_bank0_irq_ctrl_hw_t *irq_ctrl_base;
	const io_rw_32 *status_reg;
	uint32_t events;
	uint32_t pin;

	irq_ctrl_base = &iobank0_hw->proc0_irq_ctrl;
	for (pin = 0; pin < NUM_BANK0_GPIOS; pin++) {
		status_reg = &irq_ctrl_base->ints[pin / 8];
		events = (*status_reg >> 4 * (pin % 8)) & ALL_EVENTS;
		if (events) {
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

#define GPIO_REG_0U            1
#define IS_GPIO_RPI_LO_NODE(n) UTIL_CAT(GPIO_REG_, DT_REG_ADDR(n))

#define DEVICE_IF_GPIO_RPI_HI_NODE(n) COND_CODE_1(IS_GPIO_RPI_LO_NODE(n), (), (DEVICE_DT_GET(n)))

#define FIND_GPIO_RPI_HI_DEVICE(n)                                                                 \
	COND_CODE_1(UTIL_CAT(GPIO_REG_, DT_REG_ADDR(n)),                                           \
			(DT_FOREACH_CHILD(DT_PARENT(n), DEVICE_IF_GPIO_RPI_HI_NODE)), (NULL))

#if GPIO_RPI_HI_AVAILABLE
#define GPIO_RPI_INIT_HIGH_DEV(idx) .high_dev = FIND_GPIO_RPI_HI_DEVICE(DT_DRV_INST(idx)),
#else
#define GPIO_RPI_INIT_HIGH_DEV(idx)
#endif

#define GPIO_RPI_INIT(idx)                                                                         \
	BUILD_ASSERT(DT_CHILD_NUM(DT_INST_PARENT(idx)) > 0 &&                                      \
			     DT_CHILD_NUM(DT_INST_PARENT(idx)) <= 2,                               \
		     "raspberrypi,pico-gpio node must have one or two child node.");               \
	BUILD_ASSERT(GPIO_RPI_LO_AVAILABLE,                                                        \
		     "raspberrypi,pico-gpio node must have reg=0 child node.");                    \
	IF_ENABLED(IS_GPIO_RPI_LO_NODE(DT_DRV_INST(idx)), (                                        \
		static void bank_##idx##_config_func(void)                                         \
		{                                                                                  \
			IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(idx)),                                  \
				    DT_IRQ(DT_INST_PARENT(idx), priority),                         \
				    gpio_rpi_isr, DEVICE_DT_INST_GET(idx), 0);                     \
			irq_enable(DT_IRQN(DT_INST_PARENT(idx)));                                  \
		}                                                                                  \
	))                                                                                         \
	static const struct gpio_rpi_config gpio_rpi_##idx##_config = {                            \
		.common = {                                                                        \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),                     \
		},                                                                                 \
		IF_ENABLED(IS_GPIO_RPI_LO_NODE(DT_DRV_INST(idx)), (                                \
			.bank_config_func = bank_##idx##_config_func,                              \
		))                                                                                 \
		GPIO_RPI_INIT_HIGH_DEV(idx)                                                        \
	};                                                                                         \
	static struct gpio_rpi_data gpio_rpi_##idx##_data;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, gpio_rpi_bank_init, NULL, &gpio_rpi_##idx##_data,               \
			      &gpio_rpi_##idx##_config, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,    \
			      &gpio_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RPI_INIT)
