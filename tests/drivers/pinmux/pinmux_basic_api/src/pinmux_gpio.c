/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_pinmux_basic
 * @{
 * @defgroup t_pinmux_gpio test_pinmux_gpio
 * @brief TestPurpose: verify PINMUX works on GPIO port
 * @details
 * - Test Steps (Quark_se_c1000_devboard)
 *   -# Connect pin_53(GPIO_19) and pin_50(GPIO_16).
 *   -# Configure GPIO_19 as output pin and set init  LOW.
 *   -# Configure GPIO_16 as input pin and set LEVEL HIGH interrupt.
 *   -# Set pin_50 to PINMUX_FUNC_A and test GPIO functionality.
 *   -# Set pin_50 to PINMUX_FUNC_B and test GPIO functionality.
 * - Expected Results
 *   -# When set pin_50 to PINMUX_FUNC_A, pin_50 works as GPIO and gpio
 *	callback will be triggered.
 *   -# When set pin_50 to PINMUX_FUNC_B, pin_50 works as I2S and gpio
 *	callback will not be triggered.
 * @}
 */

#include <gpio.h>
#include <pinmux.h>
#include <ztest.h>

#define PINMUX_NAME CONFIG_PINMUX_NAME

#if defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD)
#define GPIO_DEV_NAME DT_GPIO_QMSI_0_NAME
#define GPIO_OUT 15 /* GPIO15_I2S_RXD */
#define GPIO_IN 16 /* GPIO16_I2S_RSCK */
#define PIN_IN 50 /* GPIO16_I2S_RSCK */
#elif defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD_SS)
#define GPIO_DEV_NAME DT_GPIO_QMSI_SS_0_NAME
#define GPIO_OUT 4  /* GPIO_SS_AIN_12 */
#define GPIO_IN 5  /* GPIO_SS_AIN_13 */
#define PIN_IN 13  /* GPIO_SS_AIN_13 */
#elif defined(CONFIG_BOARD_ARDUINO_101)
#define GPIO_DEV_NAME DT_GPIO_QMSI_0_NAME
#define GPIO_OUT 16  /* IO8 */
#define GPIO_IN 19  /* IO4 */
#define PIN_IN 53  /* IO4 */
#elif defined(CONFIG_BOARD_ARDUINO_101_SSS)
#define GPIO_DEV_NAME DT_GPIO_QMSI_SS_0_NAME
#define GPIO_OUT 2  /* AD0 */
#define GPIO_IN 3  /* AD1 */
#define PIN_IN 11  /* AD1 */
#elif defined(CONFIG_BOARD_QUARK_D2000_CRB)
#define GPIO_DEV_NAME DT_GPIO_QMSI_0_NAME
#define GPIO_OUT 8  /* DIO7 */
#define GPIO_IN 9  /* DIO8 */
#define PIN_IN 9  /* DIO8 */
#endif

#define MAX_INT_CNT 10

static bool cb_triggered;

static void callback(struct device *dev,
		     struct gpio_callback *gpio_cb, u32_t pins)
{
	static int cb_cnt;

	TC_PRINT("callback triggered: %d\n", ++cb_cnt);

	if (cb_cnt >= MAX_INT_CNT) {
		cb_triggered = true;
		gpio_pin_write(dev, GPIO_OUT, 0);
	}
}

static int test_gpio(u32_t pin, u32_t func)
{
	u32_t function;
	struct gpio_callback gpio_cb;
	struct device *pinmux = device_get_binding(PINMUX_NAME);

	if (!pinmux) {
		TC_PRINT("Cannot get PINMUX\n");
		return TC_FAIL;
	}

	struct device *gpio_dev = device_get_binding(GPIO_DEV_NAME);

	if (!gpio_dev) {
		TC_PRINT("Cannot get GPIO device\n");
		return TC_FAIL;
	}

	cb_triggered = false;

	/* 1. Configure PIN_OUT and set init voltage */
	if (gpio_pin_configure(gpio_dev, GPIO_OUT, GPIO_DIR_OUT)) {
		TC_PRINT("PIN_OUT configure fail\n");
		return TC_FAIL;
	}

	if (gpio_pin_write(gpio_dev, GPIO_OUT, 0)) {
		TC_PRINT("Set PIN_OUT init LOW fail\n");
		return TC_FAIL;
	}

	/* 2. Configure PIN_IN and set callback */
	if (gpio_pin_configure(gpio_dev, GPIO_IN,
				GPIO_DIR_IN | GPIO_INT | GPIO_INT_DEBOUNCE |
				GPIO_INT_LEVEL | GPIO_INT_ACTIVE_HIGH)) {
		TC_PRINT("PIN_IN configure fail\n");
		return TC_FAIL;
	}

	gpio_init_callback(&gpio_cb, callback, BIT(GPIO_IN));
	if (gpio_add_callback(gpio_dev, &gpio_cb)) {
		TC_PRINT("Set PIN_IN callback fail\n");
		return TC_FAIL;
	}

	gpio_pin_enable_callback(gpio_dev, GPIO_IN);

	/* 3. Verify pinmux_pin_set() */
	if (pinmux_pin_set(pinmux, pin, func)) {
		TC_PRINT("Fail to set pin func, %u : %u\n", pin, func);
		return TC_FAIL;
	}

	/* 4. Verify pinmux_pin_get() */
	if (pinmux_pin_get(pinmux, pin, &function)) {
		TC_PRINT("Fail to get pin func\n");
		return TC_FAIL;
	}

	/* 5. Verify the setting works */
	if (function != func) {
		TC_PRINT("Error. PINMUX get doesn't match PINMUX set\n");
		return TC_FAIL;
	}

	/* 6. Set PIN_OUT HIGH and verify pin func works */
	if (gpio_pin_write(gpio_dev, GPIO_OUT, 1)) {
		TC_PRINT("Set PIN_OUT HIGH fail\n");
		return TC_FAIL;
	}

	k_sleep(1000);

	if (cb_triggered) {
		TC_PRINT("GPIO callback is triggered\n");
		return TC_PASS;
	} else {
		TC_PRINT("GPIO callback is not triggered\n");
		return TC_FAIL;
	}
}

void test_pinmux_gpio(void)
{
	zassert_true(test_gpio(PIN_IN, PINMUX_FUNC_A) == TC_PASS, NULL);
	zassert_true(test_gpio(PIN_IN, PINMUX_FUNC_B) == TC_FAIL, NULL);
}
