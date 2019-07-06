/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_GPIO_H__
#define __TEST_GPIO_H__

#include <zephyr.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <ztest.h>

#if defined(DT_ALIAS_GPIO_0_LABEL)
#define DEV_NAME DT_ALIAS_GPIO_0_LABEL
#elif defined(DT_ALIAS_GPIO_1_LABEL)
#define DEV_NAME DT_ALIAS_GPIO_1_LABEL
#elif defined(DT_ALIAS_GPIO_3_LABEL)
#define DEV_NAME DT_ALIAS_GPIO_3_LABEL
#else
#error Unsupported board
#endif

#define PIN_OUT 2
#define PIN_IN 3


#define MAX_INT_CNT 3
struct drv_data {
	struct gpio_callback gpio_cb;
	int mode;
	int index;
	int aux;
};

void test_gpio_pin_read_write(void);
void test_gpio_callback_edge_high(void);
void test_gpio_callback_edge_low(void);
void test_gpio_callback_level_high(void);
void test_gpio_callback_level_low(void);
void test_gpio_callback_add_remove(void);
void test_gpio_callback_self_remove(void);
void test_gpio_callback_enable_disable(void);

#endif /* __TEST_GPIO_H__ */
