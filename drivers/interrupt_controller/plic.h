/*
 * Copyright (c) 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PLIC_H_
#define __PLIC_H_

#include <soc.h>

#define PLIC_IRQS        (CONFIG_NUM_IRQS - RISCV_MAX_GENERIC_IRQ)
#define PLIC_EN_SIZE     ((PLIC_IRQS >> 5) + 1)

/* FE310 definitons for the PLIC */
#if defined(CONFIG_SOC_SERIES_RISCV32_FE310)
#define PLIC_REG_BASE_ADDR      FE310_PLIC_REG_BASE_ADDR
#define PLIC_IRQ_EN_BASE_ADDR   FE310_PLIC_IRQ_EN_BASE_ADDR
#define PLIC_PRIO_BASE_ADDR     FE310_PLIC_PRIO_BASE_ADDR
#define PLIC_MAX_PRIORITY       FE310_PLIC_MAX_PRIORITY
#endif

#endif
