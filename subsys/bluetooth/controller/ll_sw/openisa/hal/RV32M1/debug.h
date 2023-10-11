/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* CONFIG_BT_CTLR_DEBUG_PINS */
/* CONFIG_BT_CTLR_DEBUG_PINS */
/*
 * The BLE SW link layer defines 10 GPIO pins for debug related
 * information (but really only uses 7 pins). Zephyr has a block of
 * 10 GPIOs routed to J2 odd pins 1-19 (inside bank of J2).
 *
 * Mapping:
 *
 * J2.1  : DEBUG_CPU_SLEEP
 * J2.3  : DEBUG_TICKER_ISR, DEBUG_TICKER_TASK
 * J2.5  : DEBUG_TICKER_JOB
 * J2.7  : DEBUG_RADIO_PREPARE_A, DEBUG_RADIO_CLOSE_A
 * J2.9  : DEBUG_RADIO_PREPARE_S, DEBUG_RADIO_START_S, DEBUG_RADIO_CLOSE_S
 * J2.11 : DEBUG_RADIO_PREPARE_O, DEBUG_RADIO_START_O, DEBUG_RADIO_CLOSE_O
 * J2.13 : DEBUG_RADIO_PREPARE_M, DEBUG_RADIO_START_M, DEBUG_RADIO_CLOSE_M
 * J2.15 : DEBUG_RADIO_ISR
 * J2.17 : DEBUG_RADIO_XTAL
 * J2.19 : DEBUG_RADIO_ACTIVE
 *
 */
#ifdef CONFIG_BT_CTLR_DEBUG_PINS

#include <zephyr/drivers/gpio.h>

extern const struct device *vega_debug_portb;
extern const struct device *vega_debug_portc;
extern const struct device *vega_debug_portd;

#define DEBUG0_PIN       5
#define DEBUG0_PORT		 vega_debug_portd

#define DEBUG1_PIN       4
#define DEBUG1_PORT		 vega_debug_portd

#define DEBUG2_PIN       3
#define DEBUG2_PORT		 vega_debug_portd

#define DEBUG3_PIN       2
#define DEBUG3_PORT		 vega_debug_portd

#define DEBUG4_PIN       1
#define DEBUG4_PORT		 vega_debug_portd

#define DEBUG5_PIN       0
#define DEBUG5_PORT		 vega_debug_portd

#define DEBUG6_PIN       30
#define DEBUG6_PORT		 vega_debug_portc

#define DEBUG7_PIN       29
#define DEBUG7_PORT		 vega_debug_portc

#define DEBUG8_PIN       28
#define DEBUG8_PORT		 vega_debug_portc

#define DEBUG9_PIN       29
#define DEBUG9_PORT		 vega_debug_portb



/* below are some interesting macros referenced by controller
 * which can be defined to SoC's GPIO toggle to observe/debug the
 * controller's runtime behavior.
 *
 * The ULL/LLL has defined 10 bits for doing debug output to be captured
 * by a logic analyzer (i.e. minimally invasive). Vega board has some
 * GPIO routed to the headers. Unfortunately they are spread across
 * ports if you want to implement the 10 unique bits.
 *
 * portd
 *
 */
#define DEBUG_INIT() \
	do { \
		vega_debug_portb = DEVICE_DT_GET(DT_NODELABEL(gpiob)); \
		vega_debug_portc = DEVICE_DT_GET(DT_NODELABEL(gpioc)); \
		vega_debug_portd = DEVICE_DT_GET(DT_NODELABEL(gpiod)); \
		\
		__ASSERT_NO_MSG(device_is_ready(vega_debug_portb)); \
		__ASSERT_NO_MSG(device_is_ready(vega_debug_portc)); \
		__ASSERT_NO_MSG(device_is_ready(vega_debug_portd)); \
		\
		gpio_pin_configure(DEBUG0_PORT, DEBUG0_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG1_PORT, DEBUG1_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG2_PORT, DEBUG2_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG3_PORT, DEBUG3_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG4_PORT, DEBUG4_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG5_PORT, DEBUG5_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG6_PORT, DEBUG6_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG7_PORT, DEBUG7_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG8_PORT, DEBUG8_PIN, GPIO_OUTPUT); \
		gpio_pin_configure(DEBUG9_PORT, DEBUG9_PIN, GPIO_OUTPUT); \
		\
		gpio_pin_set(DEBUG0_PORT, DEBUG0_PIN, 1); \
		gpio_pin_set(DEBUG0_PORT, DEBUG0_PIN, 0); \
	} while (0)

#define DEBUG_CPU_SLEEP(flag) gpio_pin_set(DEBUG0_PORT, DEBUG0_PIN, flag)

#define DEBUG_TICKER_ISR(flag) gpio_pin_set(DEBUG1_PORT, DEBUG1_PIN, flag)

#define DEBUG_TICKER_TASK(flag) gpio_pin_set(DEBUG1_PORT, DEBUG1_PIN, flag)

#define DEBUG_TICKER_JOB(flag) gpio_pin_set(DEBUG2_PORT, DEBUG2_PIN, flag)

#define DEBUG_RADIO_ISR(flag) gpio_pin_set(DEBUG7_PORT, DEBUG7_PIN, flag)

#define DEBUG_RADIO_XTAL(flag) gpio_pin_set(DEBUG8_PORT, DEBUG8_PIN, flag)

#define DEBUG_RADIO_ACTIVE(flag) gpio_pin_set(DEBUG9_PORT, DEBUG9_PIN, flag)

#define DEBUG_RADIO_CLOSE(flag) \
	do { \
		if (!flag) { \
			gpio_pin_set(DEBUG3_PORT, DEBUG3_PIN, flag); \
			gpio_pin_set(DEBUG4_PORT, DEBUG4_PIN, flag); \
			gpio_pin_set(DEBUG5_PORT, DEBUG5_PIN, flag); \
			gpio_pin_set(DEBUG6_PORT, DEBUG6_PIN, flag); \
		} \
	} while (0)

#define DEBUG_RADIO_PREPARE_A(flag) \
		gpio_pin_set(DEBUG3_PORT, DEBUG3_PIN, flag)

#define DEBUG_RADIO_START_A(flag) \
		gpio_pin_set(DEBUG3_PORT, DEBUG3_PIN, flag)

#define DEBUG_RADIO_CLOSE_A(flag) \
		gpio_pin_set(DEBUG3_PORT, DEBUG3_PIN, flag)

#define DEBUG_RADIO_PREPARE_S(flag) \
		gpio_pin_set(DEBUG4_PORT, DEBUG4_PIN, flag)

#define DEBUG_RADIO_START_S(flag) \
		gpio_pin_set(DEBUG4_PORT, DEBUG4_PIN, flag)

#define DEBUG_RADIO_CLOSE_S(flag) \
		gpio_pin_set(DEBUG4_PORT, DEBUG4_PIN, flag)

#define DEBUG_RADIO_PREPARE_O(flag) \
		gpio_pin_set(DEBUG5_PORT, DEBUG5_PIN, flag)

#define DEBUG_RADIO_START_O(flag) \
		gpio_pin_set(DEBUG5_PORT, DEBUG5_PIN, flag)

#define DEBUG_RADIO_CLOSE_O(flag) \
		gpio_pin_set(DEBUG5_PORT, DEBUG5_PIN, flag)

#define DEBUG_RADIO_PREPARE_M(flag) \
		gpio_pin_set(DEBUG6_PORT, DEBUG6_PIN, flag)

#define DEBUG_RADIO_START_M(flag) \
		gpio_pin_set(DEBUG6_PORT, DEBUG6_PIN, flag)

#define DEBUG_RADIO_CLOSE_M(flag) \
		gpio_pin_set(DEBUG6_PORT, DEBUG6_PIN, flag)

#else

#define DEBUG_INIT()
#define DEBUG_CPU_SLEEP(flag)
#define DEBUG_TICKER_ISR(flag)
#define DEBUG_TICKER_TASK(flag)
#define DEBUG_TICKER_JOB(flag)
#define DEBUG_RADIO_ISR(flag)
#define DEBUG_RADIO_HCTO(flag)
#define DEBUG_RADIO_XTAL(flag)
#define DEBUG_RADIO_ACTIVE(flag)
#define DEBUG_RADIO_CLOSE(flag)
#define DEBUG_RADIO_PREPARE_A(flag)
#define DEBUG_RADIO_START_A(flag)
#define DEBUG_RADIO_CLOSE_A(flag)
#define DEBUG_RADIO_PREPARE_S(flag)
#define DEBUG_RADIO_START_S(flag)
#define DEBUG_RADIO_CLOSE_S(flag)
#define DEBUG_RADIO_PREPARE_O(flag)
#define DEBUG_RADIO_START_O(flag)
#define DEBUG_RADIO_CLOSE_O(flag)
#define DEBUG_RADIO_PREPARE_M(flag)
#define DEBUG_RADIO_START_M(flag)
#define DEBUG_RADIO_CLOSE_M(flag)

#endif
