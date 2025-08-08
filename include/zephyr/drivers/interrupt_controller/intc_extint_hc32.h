/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for external interrupt/event controller in HC32 MCUs
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTINT_HC32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTINT_HC32_H_

#include <zephyr/types.h>

/**
 * @brief external trigger flags
 */
enum hc32_extint_trigger {
	HC32_EXTINT_TRIG_FALLING = 0x0, /* trigger on falling edge */
	HC32_EXTINT_TRIG_RISING = 0x1,  /* trigger on rising edge  */
	HC32_EXTINT_TRIG_BOTH = 0x2,    /* trigger on falling edge */
	HC32_EXTINT_TRIG_LOW_LVL = 0x3, /* trigger on low level */
	HC32_EXTINT_TRIG_NOT_SUPPT,     /* trigger not supported */
};

/* callback for exti interrupt */
typedef void (*hc32_extint_callback_t)(int pin, void *user);

typedef void (*hc32_extint_enable_t)(const struct device *dev, int port, int pin);
typedef void (*hc32_extint_disable_t)(const struct device *dev, int port, int pin);
typedef void (*hc32_extint_trigger_t)(const struct device *dev, int pin, int trigger);
typedef int (*hc32_extint_set_callback_t)(const struct device *dev, int pin,
					  hc32_extint_callback_t cb, void *user);
typedef void (*hc32_extint_unset_callback_t)(const struct device *dev, int pin);

struct hc32_extint_driver_api {
	hc32_extint_enable_t extint_enable;
	hc32_extint_disable_t extint_disable;
	hc32_extint_trigger_t extint_set_trigger;
	hc32_extint_set_callback_t extint_set_cb;
	hc32_extint_unset_callback_t extint_unset_cb;
};

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTINT_HC32_H_ */
