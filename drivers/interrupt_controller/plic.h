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

/* Mi-V definitons for the PLIC */
#if defined(CONFIG_SOC_SERIES_RISCV32_MIV)
#define PLIC_REG_BASE_ADDR      MIV_PLIC_REG_BASE_ADDR
#define PLIC_IRQ_EN_BASE_ADDR   MIV_PLIC_IRQ_EN_BASE_ADDR
#define PLIC_PRIO_BASE_ADDR     MIV_PLIC_PRIO_BASE_ADDR
#define PLIC_MAX_PRIORITY       MIV_PLIC_MAX_PRIORITY
#endif

#endif
