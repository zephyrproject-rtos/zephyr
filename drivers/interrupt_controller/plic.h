/*
 * Copyright (c) 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_PLIC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_PLIC_H_

#include <soc.h>

#define PLIC_IRQS        (CONFIG_NUM_IRQS - RISCV_MAX_GENERIC_IRQ)
#define PLIC_EN_SIZE     ((PLIC_IRQS >> 5) + 1)

#endif
