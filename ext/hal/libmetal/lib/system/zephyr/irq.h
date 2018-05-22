/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/irq.c
 * @brief	Zephyr libmetal irq definitions.
 */

#ifndef __METAL_IRQ__H__
#error "Include metal/irq.h instead of metal/zephyr/irq.h"
#endif

#ifndef __METAL_ZEPHYR_IRQ__H__
#define __METAL_ZEPHYR_IRQ__H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      default interrupt handler
 * @param[in]  vector interrupt vector
 */
void metal_irq_isr(unsigned int vector);


#ifdef __cplusplus
}
#endif

#endif /* __METAL_ZEPHYR_IRQ__H__ */
