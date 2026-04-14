/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_INTC_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_INTC_H_

#include <zephyr/types.h>
#include <zephyr/toolchain.h>

/* Interrupt controller operations (intc.c) */
uint32_t hexagon_intc_get_pending(void);
void hexagon_intc_ack(uint32_t irq);
void hexagon_intc_set_pending(uint32_t irq);
void hexagon_intc_enable(uint32_t irq);
void hexagon_intc_disable(uint32_t irq);
void hexagon_intc_set_priority(uint32_t irq, uint32_t priority);
void hexagon_intc_init(void);

/* Fatal error handler (fatal.c) */
FUNC_NORETURN void z_hexagon_fatal_error(unsigned int reason);

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_INTC_H_ */
