/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <gpio.h>
#include <pinmux.h>
#include <aio_comparator.h>

#define AIO_CMP_DEV_NAME	CONFIG_AIO_COMPARATOR_0_NAME
#define PINMUX_NAME		CONFIG_PINMUX_NAME

#ifdef CONFIG_ARC
#define GPIO_DEV_NAME		DT_GPIO_QMSI_SS_0_NAME
#else
#define GPIO_DEV_NAME		DT_GPIO_QMSI_0_NAME
#endif

#if defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD)
#define PIN_OUT 15	/* GPIO15 */
#define PIN_IN 10	/* GPIO_SS_AIN_10 */
#elif defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD_SS)
#define PIN_OUT 3	/* GPIO_SS_3 */
#define PIN_IN 10	/* GPIO_SS_AIN_10 */
#elif defined(CONFIG_BOARD_QUARK_D2000_CRB)
#define PIN_OUT 8	/* GPIO_8 */
#define PIN_IN 10	/* AIN_10 */
#elif defined(CONFIG_BOARD_ARDUINO_101)
#define PIN_OUT 16	/* GPIO_16 */
#define PIN_IN 10	/* AIN_10 */
#elif defined(CONFIG_BOARD_ARDUINO_101_SSS)
#define PIN_OUT 3	/* GPIO_SS_3 */
#define PIN_IN 10	/* AIN_10 */
#endif

volatile int cb_cnt;
static struct device *aio_dev;
static struct device *gpio_dev;

static void callback(void *param)
{
	int polarity = *(int *)param;

	cb_cnt++;
	TC_PRINT("*** callback triggered %s\n",
		(polarity == AIO_CMP_POL_RISE) ? "rising" : "falling"
	);

	if (polarity == AIO_CMP_POL_FALL) {
		gpio_pin_write(gpio_dev, PIN_OUT, 1);
	} else {
		gpio_pin_write(gpio_dev, PIN_OUT, 0);
	}

	if (aio_cmp_get_pending_int(aio_dev)) {
		TC_PRINT("Catch aio_cmp pending interrupt\n");
	} else {
		TC_PRINT("Fail to catch aio_cmp pending interrupt\n");
	}

	aio_cmp_disable(aio_dev, PIN_IN);
}

static int set_aio_callback(int polarity, int disable)
{
	struct device *pinmux = device_get_binding(PINMUX_NAME);

	if (!pinmux) {
		TC_PRINT("Cannot get PINMUX\n");
		return TC_FAIL;
	}

	aio_dev = device_get_binding(AIO_CMP_DEV_NAME);
	if (!aio_dev) {
		TC_PRINT("AIO Device binding failed\n");
		return TC_FAIL;
	}

	gpio_dev = device_get_binding(GPIO_DEV_NAME);
	if (!gpio_dev) {
		TC_PRINT("GPIO Device binding failed\n");
		return TC_FAIL;
	}

	if (gpio_pin_configure(gpio_dev, PIN_OUT, GPIO_DIR_OUT)) {
		TC_PRINT("Fail to configure GPIO Pin %d\n", PIN_OUT);
		return TC_FAIL;
	}

	if (pinmux_pin_set(pinmux, PIN_IN, PINMUX_FUNC_B)) {
		TC_PRINT("Fail to set pin func, %u\n", PIN_IN);
		return TC_FAIL;
	}

	gpio_pin_write(gpio_dev, PIN_OUT,
			(polarity == AIO_CMP_POL_RISE) ? 0 : 1);

	/* config AIN callback */
	zassert_true(aio_cmp_configure(aio_dev, PIN_IN,
				      polarity, AIO_CMP_REF_B,
				      callback, (void *)aio_dev) == 0,
		    "ERROR registering callback");
	if (disable == 1) {
		if (aio_cmp_disable(aio_dev, PIN_IN)) {
			TC_PRINT("Fail to disable callback\n");
			return TC_FAIL;
		}
	}

	k_sleep(100);
	cb_cnt = 0;
	k_sleep(100);

	gpio_pin_write(gpio_dev, PIN_OUT,
		(polarity == AIO_CMP_POL_RISE) ? 1 : 0);

	k_sleep(1000);
	TC_PRINT("... cb_cnt = %d\n", cb_cnt);

	return TC_PASS;
}

void test_aio_callback_rise(void)
{
	set_aio_callback(AIO_CMP_POL_RISE, 0);
	zassert_true(cb_cnt == 1, "callback is not invoked correctly");
}

void test_aio_callback_fall(void)
{
	set_aio_callback(AIO_CMP_POL_FALL, 0);
	zassert_true(cb_cnt == 1, "callback is not invoked correctly");
}

void test_aio_callback_rise_disable(void)
{
	set_aio_callback(AIO_CMP_POL_RISE, 1);
	zassert_true(cb_cnt == 0, "callback is not invoked correctly");
}
