/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>

#include "gpio_utils.h"

struct gpio_nrfx_data {
	struct gpio_driver_data general;
	sys_slist_t callbacks;

	/* Mask holding information about which pins have been configured to
	 * trigger interrupts using gpio_nrfx_config function.
	 */
	u32_t pin_int_en;

	/* Mask holding information about which pins have enabled callbacks
	 * using gpio_nrfx_enable_callback function.
	 */
	u32_t int_en;

	u32_t int_active_level;
	u32_t trig_edge;
	u32_t double_edge;
};

struct gpio_nrfx_cfg {
	NRF_GPIO_Type *port;
	u8_t port_num;
};

static int gpio_nrfx_pin_interrupt_configure(struct device *port, u32_t pin,
					     unsigned int flags);

static inline struct gpio_nrfx_data *get_port_data(struct device *port)
{
	return port->driver_data;
}

static inline const struct gpio_nrfx_cfg *get_port_cfg(struct device *port)
{
	return port->config->config_info;
}

static int gpiote_channel_alloc(u32_t abs_pin, nrf_gpiote_polarity_t polarity)
{
	for (u8_t channel = 0; channel < GPIOTE_CH_NUM; ++channel) {
		if (!nrf_gpiote_te_is_enabled(NRF_GPIOTE, channel)) {
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

static void gpiote_channel_free(u32_t abs_pin)
{
	u32_t intenset = nrf_gpiote_int_enable_check(NRF_GPIOTE,
						     NRF_GPIOTE_INT_IN_MASK);

	for (size_t i = 0; i < GPIOTE_CH_NUM; i++) {
		if ((nrf_gpiote_event_pin_get(NRF_GPIOTE, i) == abs_pin)
		    && (intenset & BIT(i))) {
			nrf_gpiote_event_disable(NRF_GPIOTE, i);
			nrf_gpiote_int_disable(NRF_GPIOTE, BIT(i));
			return;
		}
	}
}

static inline u32_t sense_for_pin(const struct gpio_nrfx_data *data,
				  u32_t pin)
{
	if ((BIT(pin) & data->int_active_level) != 0U) {
		return NRF_GPIO_PIN_SENSE_HIGH;
	}
	return NRF_GPIO_PIN_SENSE_LOW;
}

static int gpiote_pin_int_cfg(struct device *port, u32_t pin)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	u32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
	int res = 0;

	gpiote_channel_free(abs_pin);
	nrf_gpio_cfg_sense_set(abs_pin, NRF_GPIO_PIN_NOSENSE);

	/* Pins trigger interrupts only if pin has been configured to do so
	 * and callback has been enabled for that pin.
	 */
	if ((data->pin_int_en & BIT(pin)) && (data->int_en & BIT(pin))) {
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

			res = gpiote_channel_alloc(abs_pin, pol);
		} else {
		/* For level triggering we use sense mechanism. */
			u32_t sense = sense_for_pin(data, pin);

			nrf_gpio_cfg_sense_set(abs_pin, sense);
		}
	}
	return res;
}

static int gpio_nrfx_config(struct device *port, int access_op,
			    u32_t pin, int flags)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	struct gpio_nrfx_data *data = get_port_data(port);
	nrf_gpio_pin_pull_t pull;
	nrf_gpio_pin_drive_t drive;
	nrf_gpio_pin_dir_t dir;
	nrf_gpio_pin_input_t input;
	u8_t from_pin;
	u8_t to_pin;

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

	if (access_op == GPIO_ACCESS_BY_PORT) {
		from_pin = 0U;
		to_pin   = 31U;
	} else {
		from_pin = pin;
		to_pin   = pin;
	}

	for (u8_t curr_pin = from_pin; curr_pin <= to_pin; ++curr_pin) {
		int res;

		if ((flags & GPIO_OUTPUT) != 0) {
			if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
				nrf_gpio_port_out_set(reg, BIT(curr_pin));
			} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
				nrf_gpio_port_out_clear(reg, BIT(curr_pin));
			}
		}

		nrf_gpio_cfg(NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num,
					      curr_pin),
			     dir, input, pull, drive, NRF_GPIO_PIN_NOSENSE);

		WRITE_BIT(data->general.invert, curr_pin,
			  flags & GPIO_ACTIVE_LOW);

		res = gpio_nrfx_pin_interrupt_configure(port, curr_pin, flags);
		if (res != 0) {
			return res;
		}
	}

	return 0;
}

static int gpio_nrfx_write(struct device *port, int access_op,
			   u32_t pin, u32_t value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	struct gpio_nrfx_data *data = get_port_data(port);

	if (access_op == GPIO_ACCESS_BY_PORT) {
		nrf_gpio_port_out_write(reg, value ^ data->general.invert);
	} else {
		if ((value > 0) ^ ((BIT(pin) & data->general.invert) != 0)) {
			nrf_gpio_port_out_set(reg, BIT(pin));
		} else {
			nrf_gpio_port_out_clear(reg, BIT(pin));
		}
	}

	return 0;
}

static int gpio_nrfx_read(struct device *port, int access_op,
			  u32_t pin, u32_t *value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	struct gpio_nrfx_data *data = get_port_data(port);

	u32_t dir = nrf_gpio_port_dir_read(reg);
	u32_t port_in = nrf_gpio_port_in_read(reg) & ~dir;
	u32_t port_out = nrf_gpio_port_out_read(reg) & dir;
	u32_t port_val = (port_in | port_out) ^ data->general.invert;

	if (access_op == GPIO_ACCESS_BY_PORT) {
		*value = port_val;
	} else {
		*value = (port_val & BIT(pin)) ? 1 : 0;
	}

	return 0;
}

static int gpio_nrfx_port_get_raw(struct device *port, u32_t *value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	*value = nrf_gpio_port_in_read(reg);

	return 0;
}

static int gpio_nrfx_port_set_masked_raw(struct device *port, u32_t mask,
					 u32_t value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	u32_t value_tmp;

	value_tmp = nrf_gpio_port_out_read(reg) & ~mask;
	nrf_gpio_port_out_write(reg, value_tmp | (mask & value));

	return 0;
}

static int gpio_nrfx_port_set_bits_raw(struct device *port, u32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_set(reg, mask);

	return 0;
}

static int gpio_nrfx_port_clear_bits_raw(struct device *port, u32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_clear(reg, mask);

	return 0;
}

static int gpio_nrfx_port_toggle_bits(struct device *port, u32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	u32_t value;

	value = nrf_gpio_port_out_read(reg);
	nrf_gpio_port_out_write(reg, value ^ mask);

	return 0;
}

static int gpio_nrfx_pin_interrupt_configure(struct device *port,
		unsigned int pin, unsigned int flags)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	u32_t abs_pin = NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num, pin);

	if (((flags & GPIO_INT_ENABLE) != 0) &&
	    ((flags & GPIO_INT_EDGE) != 0) &&
	    (nrf_gpio_pin_dir_get(abs_pin) == NRF_GPIO_PIN_DIR_OUTPUT)) {
		/*
		 * The pin's output value as specified in the GPIO will be
		 * ignored as long as the pin is controlled by GPIOTE.
		 * Pin with output enabled cannot be used as an edge interrupt
		 * source.
		 */
		return -ENOTSUP;
	}

	WRITE_BIT(data->pin_int_en, pin, flags & GPIO_INT_ENABLE);
	WRITE_BIT(data->int_en, pin, true);
	WRITE_BIT(data->trig_edge, pin, flags & GPIO_INT_EDGE);
	WRITE_BIT(data->double_edge, pin, (flags & GPIO_INT_LOW_0) &&
					  (flags & GPIO_INT_HIGH_1));

	bool active_high = ((flags & GPIO_INT_HIGH_1) != 0);

	if (((flags & GPIO_INT_LEVELS_LOGICAL) != 0) &&
	    ((data->general.invert & BIT(pin)) != 0)) {
		active_high = !active_high;
	}
	WRITE_BIT(data->int_active_level, pin, active_high);

	return gpiote_pin_int_cfg(port, pin);
}

static int gpio_nrfx_manage_callback(struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	return gpio_manage_callback(&get_port_data(port)->callbacks,
				     callback, set);
}

static int gpio_nrfx_pin_manage_callback(struct device *port,
					 int access_op,
					 u32_t pin,
					 bool enable)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	int res = 0;
	u8_t from_pin;
	u8_t to_pin;

	if (access_op == GPIO_ACCESS_BY_PORT) {
		from_pin = 0U;
		to_pin   = 31U;
	} else {
		from_pin = pin;
		to_pin   = pin;
	}

	for (u8_t curr_pin = from_pin; curr_pin <= to_pin; ++curr_pin) {
		WRITE_BIT(data->int_en, curr_pin, enable);

		res = gpiote_pin_int_cfg(port, curr_pin);
		if (res != 0) {
			return res;
		}
	}

	return res;
}

static inline int gpio_nrfx_pin_enable_callback(struct device *port,
						int access_op,
						u32_t pin)
{
	return gpio_nrfx_pin_manage_callback(port, access_op, pin, true);
}

static inline int gpio_nrfx_pin_disable_callback(struct device *port,
						 int access_op,
						 u32_t pin)
{
	return gpio_nrfx_pin_manage_callback(port, access_op, pin, false);
}

static const struct gpio_driver_api gpio_nrfx_drv_api_funcs = {
	.config = gpio_nrfx_config,
	.write = gpio_nrfx_write,
	.read = gpio_nrfx_read,
	.port_get_raw = gpio_nrfx_port_get_raw,
	.port_set_masked_raw = gpio_nrfx_port_set_masked_raw,
	.port_set_bits_raw = gpio_nrfx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_nrfx_port_clear_bits_raw,
	.port_toggle_bits = gpio_nrfx_port_toggle_bits,
	.pin_interrupt_configure = gpio_nrfx_pin_interrupt_configure,
	.manage_callback = gpio_nrfx_manage_callback,
	.enable_callback = gpio_nrfx_pin_enable_callback,
	.disable_callback = gpio_nrfx_pin_disable_callback
};

static inline u32_t get_level_pins(struct device *port)
{
	struct gpio_nrfx_data *data = get_port_data(port);

	/* Take into consideration only pins that were configured to
	 * trigger interrupts and have callback enabled.
	 */
	u32_t out = data->int_en & data->pin_int_en;

	/* Exclude pins that trigger interrupts by edge. */
	out &= ~data->trig_edge & ~data->double_edge;

	/* The sequence above assumes that the sense field will be
	 * configured only for these pins.  If anybody's modifying
	 * PIN_CNF directly it won't work.
	 */
	return out;
}

static void cfg_level_pins(struct device *port)
{
	const struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	u32_t pin = 0U;
	u32_t bit = 1U << pin;
	u32_t level_pins = get_level_pins(port);

	/* Configure sense detection on all pins that use it. */
	while (level_pins) {
		if (level_pins & bit) {
			u32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
			u32_t sense = sense_for_pin(data, pin);

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
static u32_t check_level_trigger_pins(struct device *port)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	u32_t level_pins = get_level_pins(port);
	u32_t port_in = nrf_gpio_port_in_read(cfg->port);

	/* Extract which pins have logic level same as interrupt trigger level.
	 */
	u32_t pin_states = ~(port_in ^ data->int_active_level);

	/* Discard pins that aren't configured for level. */
	u32_t out = pin_states & level_pins;

	/* Disable sense detection on all pins that use it, whether
	 * they appear to have triggered or not.  This ensures
	 * nobody's requesting DETECT.
	 */
	u32_t pin = 0U;
	u32_t bit = 1U << pin;

	while (level_pins) {
		if (level_pins & bit) {
			u32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);

			nrf_gpio_cfg_sense_set(abs_pin, NRF_GPIO_PIN_NOSENSE);
			level_pins &= ~bit;
		}
		++pin;
		bit <<= 1;
	}

	return out;
}

static inline void fire_callbacks(struct device *port, u32_t pins)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	sys_slist_t *list = &data->callbacks;
	struct gpio_callback *cb, *tmp;

	/* Instead of calling the common gpio_fire_callbacks() function,
	 * iterate the list of callbacks locally, to be able to perform
	 * additional masking of the pins and to call handlers only for
	 * the currently enabled callbacks.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		/* Check currently enabled callbacks (data->int_en) in each
		 * iteration, as some callbacks may get disabled also in any
		 * of the handlers called here.
		 */
		if ((cb->pin_mask & pins) & data->int_en) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(port, cb, pins);
		}
	}
}

#ifdef CONFIG_GPIO_NRF_P0
DEVICE_DECLARE(gpio_nrfx_p0);
#endif
#ifdef CONFIG_GPIO_NRF_P1
DEVICE_DECLARE(gpio_nrfx_p1);
#endif

static void gpiote_event_handler(void)
{
	u32_t fired_triggers[GPIO_COUNT] = {0};
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
			u32_t abs_pin = nrf_gpiote_event_pin_get(NRF_GPIOTE, i);
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

static int gpio_nrfx_init(struct device *port)
{
	static bool gpio_initialized;

	if (!gpio_initialized) {
		gpio_initialized = true;
		IRQ_CONNECT(DT_NORDIC_NRF_GPIOTE_GPIOTE_0_IRQ_0,
			    DT_NORDIC_NRF_GPIOTE_GPIOTE_0_IRQ_0_PRIORITY,
			    gpiote_event_handler, NULL, 0);

		irq_enable(DT_NORDIC_NRF_GPIOTE_GPIOTE_0_IRQ_0);
		nrf_gpiote_int_enable(NRF_GPIOTE, NRF_GPIOTE_INT_PORT_MASK);
	}

	return 0;
}

#define GPIO_NRF_DEVICE(id)						\
	static const struct gpio_nrfx_cfg gpio_nrfx_p##id##_cfg = {	\
		.port = NRF_P##id,					\
		.port_num = id						\
	};								\
									\
	static struct gpio_nrfx_data gpio_nrfx_p##id##_data;		\
									\
	DEVICE_AND_API_INIT(gpio_nrfx_p##id,				\
			    DT_NORDIC_NRF_GPIO_GPIO_##id##_LABEL,	\
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
