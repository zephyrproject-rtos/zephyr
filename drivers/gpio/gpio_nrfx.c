/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>

#include "gpio_utils.h"

/* Mask holding information about which channels are allocated. */
static atomic_t gpiote_alloc_mask;

struct gpio_nrfx_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;

	/* Mask holding information about which pins have been configured to
	 * trigger interrupts using gpio_nrfx_config function.
	 */
	uint32_t pin_int_en;

	uint32_t int_active_level;
	uint32_t trig_edge;
	uint32_t double_edge;
};

struct gpio_nrfx_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	NRF_GPIO_Type *port;
	uint8_t port_num;
};

static inline struct gpio_nrfx_data *get_port_data(const struct device *port)
{
	return port->data;
}

static inline const struct gpio_nrfx_cfg *get_port_cfg(const struct device *port)
{
	return port->config;
}

static int gpiote_channel_alloc(atomic_t *mask, uint32_t abs_pin,
				nrf_gpiote_polarity_t polarity)
{
	for (uint8_t channel = 0; channel < GPIOTE_CH_NUM; ++channel) {
		atomic_val_t prev = atomic_or(mask, BIT(channel));

		if ((prev & BIT(channel)) == 0) {
			nrf_gpiote_event_t evt =
				offsetof(NRF_GPIOTE_Type, EVENTS_IN[channel]);

			nrf_gpiote_event_configure(NRF_GPIOTE, channel, abs_pin,
						   polarity);
			nrf_gpiote_event_clear(NRF_GPIOTE, evt);
			nrf_gpiote_event_enable(NRF_GPIOTE, channel);
			nrf_gpiote_int_enable(NRF_GPIOTE, BIT(channel));
			return 0;
		}
	}

	return -ENODEV;
}

/* Function checks if given pin does not have already enabled GPIOTE event and
 * disables it.
 */
static void gpiote_pin_cleanup(atomic_t *mask, uint32_t abs_pin)
{
	uint32_t intenset = nrf_gpiote_int_enable_check(NRF_GPIOTE,
						     NRF_GPIOTE_INT_IN_MASK);

	for (size_t i = 0; i < GPIOTE_CH_NUM; i++) {
		if ((nrf_gpiote_event_pin_get(NRF_GPIOTE, i) == abs_pin)
		    && (intenset & BIT(i))) {
			(void)atomic_and(mask, ~BIT(i));
			nrf_gpiote_event_disable(NRF_GPIOTE, i);
			nrf_gpiote_int_disable(NRF_GPIOTE, BIT(i));
			return;
		}
	}
}

static inline uint32_t sense_for_pin(const struct gpio_nrfx_data *data,
				  uint32_t pin)
{
	if ((BIT(pin) & data->int_active_level) != 0U) {
		return NRF_GPIO_PIN_SENSE_HIGH;
	}
	return NRF_GPIO_PIN_SENSE_LOW;
}

static int gpiote_pin_int_cfg(const struct device *port, uint32_t pin)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
	int res = 0;

	gpiote_pin_cleanup(&gpiote_alloc_mask, abs_pin);
	nrf_gpio_cfg_sense_set(abs_pin, NRF_GPIO_PIN_NOSENSE);

	/* Pins trigger interrupts only if pin has been configured to do so */
	if (data->pin_int_en & BIT(pin)) {
		if (data->trig_edge & BIT(pin)) {
		/* For edge triggering we use GPIOTE channels. */
			nrf_gpiote_polarity_t pol;

			if (data->double_edge & BIT(pin)) {
				pol = NRF_GPIOTE_POLARITY_TOGGLE;
			} else if ((data->int_active_level & BIT(pin)) != 0U) {
				pol = NRF_GPIOTE_POLARITY_LOTOHI;
			} else {
				pol = NRF_GPIOTE_POLARITY_HITOLO;
			}

			res = gpiote_channel_alloc(&gpiote_alloc_mask,
						   abs_pin, pol);
		} else {
		/* For level triggering we use sense mechanism. */
			uint32_t sense = sense_for_pin(data, pin);

			nrf_gpio_cfg_sense_set(abs_pin, sense);
		}
	}
	return res;
}

static int gpio_nrfx_config(const struct device *port,
			    gpio_pin_t pin, gpio_flags_t flags)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	nrf_gpio_pin_pull_t pull;
	nrf_gpio_pin_drive_t drive;
	nrf_gpio_pin_dir_t dir;
	nrf_gpio_pin_input_t input;

	switch (flags & (GPIO_DS_LOW_MASK | GPIO_DS_HIGH_MASK |
			 GPIO_OPEN_DRAIN)) {
	case GPIO_DS_DFLT_LOW | GPIO_DS_DFLT_HIGH:
		drive = NRF_GPIO_PIN_S0S1;
		break;
	case GPIO_DS_DFLT_LOW | GPIO_DS_ALT_HIGH:
		drive = NRF_GPIO_PIN_S0H1;
		break;
	case GPIO_DS_DFLT_LOW | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_S0D1;
		break;

	case GPIO_DS_ALT_LOW | GPIO_DS_DFLT_HIGH:
		drive = NRF_GPIO_PIN_H0S1;
		break;
	case GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH:
		drive = NRF_GPIO_PIN_H0H1;
		break;
	case GPIO_DS_ALT_LOW | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_H0D1;
		break;

	case GPIO_DS_DFLT_HIGH | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0S1;
		break;
	case GPIO_DS_ALT_HIGH | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0H1;
		break;

	default:
		return -EINVAL;
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		pull = NRF_GPIO_PIN_PULLUP;
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		pull = NRF_GPIO_PIN_PULLDOWN;
	} else {
		pull = NRF_GPIO_PIN_NOPULL;
	}

	dir = ((flags & GPIO_OUTPUT) != 0)
	      ? NRF_GPIO_PIN_DIR_OUTPUT
	      : NRF_GPIO_PIN_DIR_INPUT;

	input = ((flags & GPIO_INPUT) != 0)
		? NRF_GPIO_PIN_INPUT_CONNECT
		: NRF_GPIO_PIN_INPUT_DISCONNECT;

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			nrf_gpio_port_out_set(reg, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			nrf_gpio_port_out_clear(reg, BIT(pin));
		}
	}

	nrf_gpio_cfg(NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num, pin),
		     dir, input, pull, drive, NRF_GPIO_PIN_NOSENSE);

	return 0;
}

static int gpio_nrfx_port_get_raw(const struct device *port, uint32_t *value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	*value = nrf_gpio_port_in_read(reg);

	return 0;
}

static int gpio_nrfx_port_set_masked_raw(const struct device *port,
					 uint32_t mask,
					 uint32_t value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	uint32_t value_tmp;

	value_tmp = nrf_gpio_port_out_read(reg) & ~mask;
	nrf_gpio_port_out_write(reg, value_tmp | (mask & value));

	return 0;
}

static int gpio_nrfx_port_set_bits_raw(const struct device *port,
				       uint32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_set(reg, mask);

	return 0;
}

static int gpio_nrfx_port_clear_bits_raw(const struct device *port,
					 uint32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_clear(reg, mask);

	return 0;
}

static int gpio_nrfx_port_toggle_bits(const struct device *port,
				      uint32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	uint32_t value;

	value = nrf_gpio_port_out_read(reg);
	nrf_gpio_port_out_write(reg, value ^ mask);

	return 0;
}

static int gpio_nrfx_pin_interrupt_configure(const struct device *port,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num, pin);

	if ((mode == GPIO_INT_MODE_EDGE) &&
	    (nrf_gpio_pin_dir_get(abs_pin) == NRF_GPIO_PIN_DIR_OUTPUT)) {
		/*
		 * The pin's output value as specified in the GPIO will be
		 * ignored as long as the pin is controlled by GPIOTE.
		 * Pin with output enabled cannot be used as an edge interrupt
		 * source.
		 */
		return -ENOTSUP;
	}

	WRITE_BIT(data->pin_int_en, pin, mode != GPIO_INT_MODE_DISABLED);
	WRITE_BIT(data->trig_edge, pin, mode == GPIO_INT_MODE_EDGE);
	WRITE_BIT(data->double_edge, pin, trig == GPIO_INT_TRIG_BOTH);
	WRITE_BIT(data->int_active_level, pin, trig == GPIO_INT_TRIG_HIGH);

	return gpiote_pin_int_cfg(port, pin);
}

static int gpio_nrfx_manage_callback(const struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	return gpio_manage_callback(&get_port_data(port)->callbacks,
				     callback, set);
}

static const struct gpio_driver_api gpio_nrfx_drv_api_funcs = {
	.pin_configure = gpio_nrfx_config,
	.port_get_raw = gpio_nrfx_port_get_raw,
	.port_set_masked_raw = gpio_nrfx_port_set_masked_raw,
	.port_set_bits_raw = gpio_nrfx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_nrfx_port_clear_bits_raw,
	.port_toggle_bits = gpio_nrfx_port_toggle_bits,
	.pin_interrupt_configure = gpio_nrfx_pin_interrupt_configure,
	.manage_callback = gpio_nrfx_manage_callback,
};

static inline uint32_t get_level_pins(const struct device *port)
{
	struct gpio_nrfx_data *data = get_port_data(port);

	/* Take into consideration only pins that were configured to
	 * trigger interrupts.
	 */
	uint32_t out = data->pin_int_en;

	/* Exclude pins that trigger interrupts by edge. */
	out &= ~data->trig_edge & ~data->double_edge;

	/* The sequence above assumes that the sense field will be
	 * configured only for these pins.  If anybody's modifying
	 * PIN_CNF directly it won't work.
	 */
	return out;
}

static void cfg_level_pins(const struct device *port)
{
	const struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t pin = 0U;
	uint32_t bit = 1U << pin;
	uint32_t level_pins = get_level_pins(port);

	/* Configure sense detection on all pins that use it. */
	while (level_pins) {
		if (level_pins & bit) {
			uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
			uint32_t sense = sense_for_pin(data, pin);

			nrf_gpio_cfg_sense_set(abs_pin, sense);
			level_pins &= ~bit;
		}
		++pin;
		bit <<= 1;
	}
}

/**
 * @brief Function for getting pins that triggered level interrupt.
 *
 * @param port Pointer to GPIO port device.
 *
 * @return Bitmask where 1 marks pin as trigger source.
 */
static uint32_t check_level_trigger_pins(const struct device *port)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t level_pins = get_level_pins(port);
	uint32_t port_in = nrf_gpio_port_in_read(cfg->port);

	/* Extract which pins have logic level same as interrupt trigger level.
	 */
	uint32_t pin_states = ~(port_in ^ data->int_active_level);

	/* Discard pins that aren't configured for level. */
	uint32_t out = pin_states & level_pins;

	/* Disable sense detection on all pins that use it, whether
	 * they appear to have triggered or not.  This ensures
	 * nobody's requesting DETECT.
	 */
	uint32_t pin = 0U;
	uint32_t bit = 1U << pin;

	while (level_pins) {
		if (level_pins & bit) {
			uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);

			nrf_gpio_cfg_sense_set(abs_pin, NRF_GPIO_PIN_NOSENSE);
			level_pins &= ~bit;
		}
		++pin;
		bit <<= 1;
	}

	return out;
}

static inline void fire_callbacks(const struct device *port, uint32_t pins)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	sys_slist_t *list = &data->callbacks;

	gpio_fire_callbacks(list, port, pins);
}

#ifdef CONFIG_GPIO_NRF_P0
DEVICE_DECLARE(gpio_nrfx_p0);
#endif
#ifdef CONFIG_GPIO_NRF_P1
DEVICE_DECLARE(gpio_nrfx_p1);
#endif

static void gpiote_event_handler(void)
{
	uint32_t fired_triggers[GPIO_COUNT] = {0};
	bool port_event = nrf_gpiote_event_check(NRF_GPIOTE,
						 NRF_GPIOTE_EVENT_PORT);

	if (port_event) {
#ifdef CONFIG_GPIO_NRF_P0
		fired_triggers[0] =
			check_level_trigger_pins(DEVICE_GET(gpio_nrfx_p0));
#endif
#ifdef CONFIG_GPIO_NRF_P1
		fired_triggers[1] =
			check_level_trigger_pins(DEVICE_GET(gpio_nrfx_p1));
#endif

		/* Sense detect was disabled while checking pins so
		 * DETECT should be deasserted.
		 */
		nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_PORT);
	}

	/* Handle interrupt from GPIOTE channels. */
	for (size_t i = 0; i < GPIOTE_CH_NUM; i++) {
		nrf_gpiote_event_t evt =
			offsetof(NRF_GPIOTE_Type, EVENTS_IN[i]);

		if (nrf_gpiote_int_enable_check(NRF_GPIOTE, BIT(i)) &&
		    nrf_gpiote_event_check(NRF_GPIOTE, evt)) {
			uint32_t abs_pin = nrf_gpiote_event_pin_get(NRF_GPIOTE, i);
			/* Divide absolute pin number to port and pin parts. */
			fired_triggers[abs_pin / 32U] |= BIT(abs_pin % 32);
			nrf_gpiote_event_clear(NRF_GPIOTE, evt);
		}
	}

#ifdef CONFIG_GPIO_NRF_P0
	if (fired_triggers[0]) {
		fire_callbacks(DEVICE_GET(gpio_nrfx_p0), fired_triggers[0]);
	}
#endif
#ifdef CONFIG_GPIO_NRF_P1
	if (fired_triggers[1]) {
		fire_callbacks(DEVICE_GET(gpio_nrfx_p1), fired_triggers[1]);
	}
#endif

	if (port_event) {
		/* Reprogram sense to match current configuration.
		 * This may cause DETECT to be re-asserted.
		 */
#ifdef CONFIG_GPIO_NRF_P0
		cfg_level_pins(DEVICE_GET(gpio_nrfx_p0));
#endif
#ifdef CONFIG_GPIO_NRF_P1
		cfg_level_pins(DEVICE_GET(gpio_nrfx_p1));
#endif
	}
}

#define GPIOTE_NODE DT_INST(0, nordic_nrf_gpiote)

static int gpio_nrfx_init(const struct device *port)
{
	static bool gpio_initialized;

	if (!gpio_initialized) {
		gpio_initialized = true;
		IRQ_CONNECT(DT_IRQN(GPIOTE_NODE), DT_IRQ(GPIOTE_NODE, priority),
			    gpiote_event_handler, NULL, 0);

		irq_enable(DT_IRQN(GPIOTE_NODE));
		nrf_gpiote_int_enable(NRF_GPIOTE, NRF_GPIOTE_INT_PORT_MASK);
	}

	return 0;
}

/*
 * Device instantiation is done with node labels because 'port_num' is
 * the peripheral number by SoC numbering. We therefore cannot use
 * DT_INST APIs here without wider changes.
 */

#define GPIO(id) DT_NODELABEL(gpio##id)

#define GPIO_NRF_DEVICE(id)						\
	static const struct gpio_nrfx_cfg gpio_nrfx_p##id##_cfg = {	\
		.common = {						\
			.port_pin_mask =				\
			GPIO_PORT_PIN_MASK_FROM_DT_NODE(GPIO(id)),	\
		},							\
		.port = NRF_P##id,					\
		.port_num = id						\
	};								\
									\
	static struct gpio_nrfx_data gpio_nrfx_p##id##_data;		\
									\
	DEVICE_AND_API_INIT(gpio_nrfx_p##id,				\
			    DT_LABEL(GPIO(id)),				\
			    gpio_nrfx_init,				\
			    &gpio_nrfx_p##id##_data,			\
			    &gpio_nrfx_p##id##_cfg,			\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &gpio_nrfx_drv_api_funcs)

#ifdef CONFIG_GPIO_NRF_P0
GPIO_NRF_DEVICE(0);
#endif

#ifdef CONFIG_GPIO_NRF_P1
GPIO_NRF_DEVICE(1);
#endif
