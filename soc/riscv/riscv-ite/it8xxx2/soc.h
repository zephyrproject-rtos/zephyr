#ifndef __RISCV_ITE_SOC_H_
#define __RISCV_ITE_SOC_H_
/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <soc_common.h>

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(CONFIG_SRAM_SIZE)

#ifndef _ASMLANGUAGE
void soc_interrupt_init(void);
#endif

#endif /* __RISCV_ITE_SOC_H_ */
