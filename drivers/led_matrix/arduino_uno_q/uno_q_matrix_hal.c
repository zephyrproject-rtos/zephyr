/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Shankar Sridhar
 *
 * drivers/led_matrix/uno_q_matrix_hal.c
 *
 * Arduino UNO Q board-specific HAL for the Charlieplex LED matrix driver.
 *
 */

#define DT_DRV_COMPAT arduino_uno_q_ledmatrix

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/led_matrix.h>

LOG_MODULE_DECLARE(led_matrix, CONFIG_LED_MATRIX_UNO_Q_LOG_LEVEL);

#define NUM_MATRIX_PINS 11

/* ------------------------------------------------------------------ */
/* GPIO specs — read from device tree at compile time                  */
/* ------------------------------------------------------------------ */

static const struct gpio_dt_spec matrix_pins[NUM_MATRIX_PINS] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 0),   /* PF0  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 1),   /* PF1  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 2),   /* PF2  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 3),   /* PF3  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 4),   /* PF4  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 5),   /* PF5  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 6),   /* PF6  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 7),   /* PF7  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 8),   /* PF8  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 9),   /* PF9  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 10),  /* PF10 */
};

/* ------------------------------------------------------------------ */
/* led_matrix_hal_api implementations                                  */
/* ------------------------------------------------------------------ */

/*
 * uno_q_set_pin — drive one matrix GPIO to HIGH, LOW, or Hi-Z.
 */
static inline void uno_q_set_pin(uint32_t pin_index, int state)
{
	if (pin_index >= NUM_MATRIX_PINS) {
		return;
	}

	const struct gpio_dt_spec *spec = &matrix_pins[pin_index];

	switch (state) {
	case 1:
		/* Physical HIGH — anode side of the LED */
		gpio_pin_configure_dt(spec, GPIO_OUTPUT_HIGH);
		break;
	case 0:
		/* Physical LOW — cathode side of the LED */
		gpio_pin_configure_dt(spec, GPIO_OUTPUT_LOW);
		break;
	default:
		/* Hi-Z — disconnects the pin, prevents sneak current paths */
		gpio_pin_configure_dt(spec, GPIO_INPUT);
		break;
	}
}

/*
 * uno_q_all_pins_highz — set every matrix pin to Hi-Z in one call.
 *
 * Called at the start of every ISR tick before lighting the next LED,
 * and whenever the display is blanked.
 */
static void uno_q_all_pins_highz(void)
{
	for (int i = 0; i < NUM_MATRIX_PINS; i++) {
		gpio_pin_configure_dt(&matrix_pins[i], GPIO_INPUT);
	}
}

/*
 * uno_q_init_pins — verify GPIO readiness and set all pins to Hi-Z.
 *
 * Called once from led_matrix_init() during POST_KERNEL init.
 * Returns -ENODEV if any pin is not ready, 0 on success.
 */
static int uno_q_init_pins(void)
{
	for (int i = 0; i < NUM_MATRIX_PINS; i++) {
		if (!gpio_is_ready_dt(&matrix_pins[i])) {
			LOG_ERR("Matrix GPIO pin %d not ready", i);
			return -ENODEV;
		}
	}

	uno_q_all_pins_highz();
	return 0;
}

/*
 * uno_q_hal_api — concrete implementation of led_matrix_hal_api.
 *
 */
static const struct led_matrix_hal_api uno_q_hal_api = {
	.led_matrix_set_pin        = uno_q_set_pin,
	.led_matrix_all_pins_highz = uno_q_all_pins_highz,
	.led_matrix_init_pins      = uno_q_init_pins,
};

/* ------------------------------------------------------------------ */
/* Device instantiation — one instance per enabled DT node            */
/* ------------------------------------------------------------------ */

BUILD_ASSERT(NUM_MATRIX_PINS * (NUM_MATRIX_PINS - 1) >= 8 * 13,
	     "Not enough GPIO pins to Charlieplex an 8x13 matrix");

#define UNO_Q_LED_MATRIX_DEVICE(inst)									\
    static const uint32_t uno_q_led_pairs_##inst[] =                    \
        DT_INST_PROP(inst, led_pairs);                                  \
																		\
    static const struct led_matrix_config uno_q_config_##inst = {      	\
        .hal         = &uno_q_hal_api,                                  \
       	.pin_map     = (const struct led_matrix_pin_pair *)             \
                        uno_q_led_pairs_##inst,                         \
        .pin_map_len = DT_INST_PROP_LEN(inst, led_pairs) / 2,          	\
        .rows        = DT_INST_PROP(inst, rows),                        \
        .cols        = DT_INST_PROP(inst, columns),                     \
    };                                                                  \
																		\
	static struct led_matrix_data uno_q_data_##inst;					\
																		\
	DEVICE_DT_INST_DEFINE(inst,											\
			      led_matrix_init,										\
			      NULL,													\
			      &uno_q_data_##inst,									\
			      &uno_q_config_##inst,									\
			      POST_KERNEL,											\
			      CONFIG_LED_MATRIX_UNO_Q_INIT_PRIORITY,				\
			      &led_matrix_display_api)

BUILD_ASSERT(sizeof(struct led_matrix_pin_pair) == 2 * sizeof(uint32_t),
             "led_matrix_pin_pair layout mismatch — cast unsafe");

DT_INST_FOREACH_STATUS_OKAY(UNO_Q_LED_MATRIX_DEVICE)
