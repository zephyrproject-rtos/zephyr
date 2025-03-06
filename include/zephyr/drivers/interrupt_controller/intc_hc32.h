/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for interrupt/event controller in HC32 MCUs
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_HC32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_HC32_H_

#include <zephyr/types.h>


/**
 * @brief external trigger flags
 */
enum hc32_extint_trigger {
	HC32_EXTINT_TRIG_FALLING = 0x0,     /* trigger on falling edge */
	HC32_EXTINT_TRIG_RISING  = 0x1,     /* trigger on rising edge  */
	HC32_EXTINT_TRIG_BOTH    = 0x2,     /* trigger on falling edge */
	HC32_EXTINT_TRIG_LOW_LVL = 0x3,     /* trigger on low level */
};

/* callback for exti interrupt */
typedef void (*hc32_extint_callback_t)(int pin, void *user);


/* extint config */
void hc32_extint_enable(int port, int pin);
void hc32_extint_disable(int port, int pin);
void hc32_extint_trigger(int pin, int trigger);
int  hc32_extint_set_callback(int pin, hc32_extint_callback_t cb, void *user);
void hc32_extint_unset_callback(int pin);


/* intc config */
int hc32_intc_irq_signin(int irqn, int intsrc);
int hc32_intc_irq_signout(int irqn);


#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_HC32_H_ */
