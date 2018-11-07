/*
 * Copyright (c) 2018 Alexander Kozlov <alexander.kozlov@cloudbear.ru>
 *                    Vitaly Gaiduk <vitaly.gaiduk@cloudbear.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV32_CLOUDBEAR_BM310_SOC_H_
#define __RISCV32_CLOUDBEAR_BM310_SOC_H_

#include <soc_common.h>

/* Platform Level Interrupt Controller Configuration */
#define PLIC_PRIO_BASE_ADDR 0x0C000000
#define PLIC_IRQ_EN_BASE_ADDR 0x0C002000
#define PLIC_REG_BASE_ADDR 0x0C200000

#define PLIC_MAX_PRIORITY 8

/* Timer configuration */
#define RISCV_MTIME_BASE 0x0200BFF8
#define RISCV_MTIMECMP_BASE 0x02004000

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE CONFIG_RISCV_RAM_BASE_ADDR
#define RISCV_RAM_SIZE CONFIG_RISCV_RAM_SIZE

#endif /* __RISCV32_CLOUDBEAR_BM310_SOC_H_ */
