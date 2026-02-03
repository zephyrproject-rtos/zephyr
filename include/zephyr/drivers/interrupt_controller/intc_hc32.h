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

/* intc config */
int hc32_intc_irq_signin(int irqn, int intsrc);
int hc32_intc_irq_signout(int irqn);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_HC32_H_ */
