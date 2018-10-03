/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>

#include "gpio_utils.h"

struct gpio_nrfx_data {
	sys_slist_t callbacks;

	/* Mask holding information about which pins have been configured to
	 * trigger interrupts using gpio_nrfx_config function.
	 */
	u32_t pin_int_en;

	/* Mask holding information about which pins have enabled callbacks
	 * using gpio_nrfx_enable_callback function.
	 */
	u32_t int_en;

	u32_t active_level;
	u32_t trig_edge;
	u32_t double_edge;
	u32_t inverted;
};

struct gpio_nrfx_cfg {
	NRF_GPIO_Type *port;
	u8_t port_num;
};

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
		if (!nrf_gpiote_te_is_enabled(channel)) {
			nrf_gpiote_events_t evt =
				offsetof(NRF_GPIOTE_Type, EVENTS_IN[channel]);

			nrf_gpiote_event_configure(channel, abs_pin, polarity);
			nrf_gpiote_event_clear(evt);
			nrf_gpiote_event_enable(channel);
			nrf_gpiote_int_enable(BIT(channel));
			return 0;
		}
	}

	return -ENODEV;
}

static void gpiote_channel_free(u32_t abs_pin)
{
	u32_t intenset = nrf_gpiote_int_is_enabled(NRF_GPIOTE_INT_IN_MASK);

	for (size_t i = 0; i < GPIOTE_CH_NUM; i++) {
		if ((nrf_gpiote_event_pin_get(i) == abs_pin)
		    && (intenset & BIT(i))) {
			nrf_gpiote_event_disable(i);
			nrf_gpiote_int_disable(BIT(i));
			return;
		}
	}
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
			} else if (((data->active_level & BIT(pin)) != 0)
				   ^ ((BIT(pin) & data->inverted) != 0)) {
				pol = NRF_GPIOTE_POLARITY_LOTOHI;
			} else {
				pol = NRF_GPIOTE_POLARITY_HITOLO;
			}

			res = gpiote_channel_alloc(abs_pin, pol);
		} else {
		/* For level triggering we use sense mechanism. */
			nrf_gpio_pin_sense_t sense;

			if (((data->active_level & BIT(pin)) != 0)
			    ^ ((BIT(pin) & data->inverted) != 0)) {
				sense = NRF_GPIO_PIN_SENSE_HIGH;
			} else {
				sense = NRF_GPIO_PIN_SENSE_LOW;
			}

			nrf_gpio_cfg_sense_set(abs_pin, sense);
		}
	}
	return res;
}

static int gpio_nrfx_config(struct device *port, int access_op,
			    u32_t pin, int flags)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	nrf_gpio_pin_pull_t pull;
	nrf_gpio_pin_drive_t drive;
	nrf_gpio_pin_dir_t dir;
	nrf_gpio_pin_input_t input;
	u8_t from_pin;
	u8_t to_pin;

	switch (flags & (GPIO_DS_LOW_MASK | GPIO_DS_HIGH_MASK)) {
	case GPIO_DS_DFLT_LOW | GPIO_DS_DFLT_HIGH:
		drive = NRF_GPIO_PIN_S0S1;
		break;
	case GPIO_DS_DFLT_LOW | GPIO_DS_ALT_HIGH:
		drive = NRF_GPIO_PIN_S0H1;
		break;
	case GPIO_DS_DFLT_LOW | GPIO_DS_DISCONNECT_HIGH:
		drive = NRF_GPIO_PIN_S0D1;
		break;

	case GPIO_DS_ALT_LOW | GPIO_DS_DFLT_HIGH:
		drive = NRF_GPIO_PIN_H0S1;
		break;
	case GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH:
		drive = NRF_GPIO_PIN_H0H1;
		break;
	case GPIO_DS_ALT_LOW | GPIO_DS_DISCONNECT_HIGH:
		drive = NRF_GPIO_PIN_H0D1;
		break;

	case GPIO_DS_DISCONNECT_LOW | GPIO_DS_DFLT_HIGH:
		drive = NRF_GPIO_PIN_D0S1;
		break;
	case GPIO_DS_DISCONNECT_LOW | GPIO_DS_ALT_HIGH:
		drive = NRF_GPIO_PIN_D0H1;
		break;

	default:
		return -EINVAL;
	}

	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
		pull = NRF_GPIO_PIN_PULLUP;
	} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
		pull = NRF_GPIO_PIN_PULLDOWN;
	} else {
		pull = NRF_GPIO_PIN_NOPULL;
	}

	dir = ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT)
	      ? NRF_GPIO_PIN_DIR_OUTPUT
	      : NRF_GPIO_PIN_DIR_INPUT;

	input = (dir == NRF_GPIO_PIN_DIR_INPUT)
		? NRF_GPIO_PIN_INPUT_CONNECT
		: NRF_GPIO_PIN_INPUT_DISCONNECT;

	if (access_op == GPIO_ACCESS_BY_PORT) {
		from_pin = 0;
		to_pin   = 31;
	} else {
		from_pin = pin;
		to_pin   = pin;
	}

	for (u8_t curr_pin = from_pin; curr_pin <= to_pin; ++curr_pin) {
		int res;

		nrf_gpio_cfg(NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num,
					      curr_pin),
			     dir, input, pull, drive, NRF_GPIO_PIN_NOSENSE);

		WRITE_BIT(data->pin_int_en, curr_pin, flags & GPIO_INT);
		WRITE_BIT(data->trig_edge, curr_pin, flags & GPIO_INT_EDGE);
		WRITE_BIT(data->double_edge, curr_pin,
			  flags & GPIO_INT_DOUBLE_EDGE);
		WRITE_BIT(data->active_level, curr_pin,
			  flags & GPIO_INT_ACTIVE_HIGH);
		WRITE_BIT(data->inverted, curr_pin, flags & GPIO_POL_INV);

		res = gpiote_pin_int_cfg(port, curr_pin);
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
		nrf_gpio_port_out_write(reg, value ^ data->inverted);
	} else {
		if ((value > 0) ^ ((BIT(pin) & data->inverted) != 0)) {
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

	u32_t port_in = nrf_gpio_port_in_read(reg) ^ data->inverted;

	if (access_op == GPIO_ACCESS_BY_PORT) {
		*value = port_in;
	} else {
		*value = (port_in & BIT(pin)) ? 1 : 0;
	}

	return 0;
}

static int gpio_nrfx_manage_callback(struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	_gpio_manage_callback(&get_port_data(port)->callbacks, callback, set);
	return 0;
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
		from_pin = 0;
		to_pin   = 31;
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
	.manage_callback = gpio_nrfx_manage_callback,
	.enable_callback = gpio_nrfx_pin_enable_callback,
	.disable_callback = gpio_nrfx_pin_disable_callback
};

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

	u32_t port_in = nrf_gpio_port_in_read(get_port_cfg(port)->port);

	/* Extract which pins after inversion, have logic level same as
	 * interrupt trigger level.
	 */
	u32_t pin_states = ~(port_in ^ data->inverted ^ data->active_level);

	/* Take into consideration only pins that were configured to
	 * trigger interrupts and have callback enabled.
	 */
	u32_t out = pin_states & data->int_en & data->pin_int_en;

	/* Exclude pins that trigger interrupts by edge. */
	out &= ~data->trig_edge & ~data->double_edge;

	return out;
}

static inline void fire_callbacks(struct device *port, u32_t pins)
{
	_gpio_fire_callbacks(&get_port_data(port)->callbacks, port, pins);
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

	if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_PORT)) {
#ifdef CONFIG_GPIO_NRF_P0
		fired_triggers[0] =
			check_level_trigger_pins(DEVICE_GET(gpio_nrfx_p0));
#endif
#ifdef CONFIG_GPIO_NRF_P1
		fired_triggers[1] =
			check_level_trigger_pins(DEVICE_GET(gpio_nrfx_p1));
#endif
	}

	/* Handle interrupt from GPIOTE channels. */
	for (size_t i = 0; i < GPIOTE_CH_NUM; i++) {
		nrf_gpiote_events_t evt =
			offsetof(NRF_GPIOTE_Type, EVENTS_IN[i]);

		if (nrf_gpiote_int_is_enabled(BIT(i)) &&
		    nrf_gpiote_event_is_set(evt)) {
			u32_t abs_pin = nrf_gpiote_event_pin_get(i);
			/* Divide absolute pin number to port and pin parts. */
			fired_triggers[abs_pin / 32] |= BIT(abs_pin % 32);
			nrf_gpiote_event_clear(evt);
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

	if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_PORT)) {
		u32_t active_level_triggers =
#ifdef CONFIG_GPIO_NRF_P0
			check_level_trigger_pins(DEVICE_GET(gpio_nrfx_p0)) |
#endif
#ifdef CONFIG_GPIO_NRF_P1
			check_level_trigger_pins(DEVICE_GET(gpio_nrfx_p1)) |
#endif
			0;

		/* The PORT event is generated on the rising edge of the DETECT
		 * signal, i.e. when a pin state changes to the level
		 * configured to be sensed. If any level triggering pins are
		 * still in their active state, don't clear the PORT event so
		 * that the interrupt is fired again * and the proper callbacks
		 * are executed.
		 */
		if (active_level_triggers == 0) {
			nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
		}
	}
}

static int gpio_nrfx_init(struct device *port)
{
	static bool gpio_initialized;

	if (!gpio_initialized) {
		gpio_initialized = true;
		IRQ_CONNECT(CONFIG_GPIOTE_IRQ, CONFIG_GPIOTE_IRQ_PRI,
			    gpiote_event_handler, NULL, 0);

		irq_enable(CONFIG_GPIOTE_IRQ);
		nrf_gpiote_int_enable(NRF_GPIOTE_INT_PORT_MASK);
	}

	return 0;
}

#ifdef CONFIG_SOC_SERIES_NRF51X
#define NRF_P0 NRF_GPIO
#endif

#define GPIO_NRF_DEVICE(id)						\
	static const struct gpio_nrfx_cfg gpio_nrfx_p##id##_cfg = {	\
		.port = NRF_P##id,					\
		.port_num = id						\
	};								\
									\
	static struct gpio_nrfx_data gpio_nrfx_p##id##_data;		\
									\
	DEVICE_AND_API_INIT(gpio_nrfx_p##id,				\
			    CONFIG_GPIO_P##id##_DEV_NAME,		\
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
