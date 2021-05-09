#ifndef __RISCV_ITE_SOC_H_
#define __RISCV_ITE_SOC_H_
/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <soc_common.h>
#include <devicetree.h>

#define UART_REG_ADDR_INTERVAL 1

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(CONFIG_SRAM_SIZE)

#define ite_write(reg, reg_size, val) \
			((*((volatile unsigned char *)(reg))) = val)
#define ite_read(reg, reg_size) \
			(*((volatile unsigned char *)(reg)))

/* PINMUX config */
#define IT8XXX2_PINMUX_IOF0		0x00
#define IT8XXX2_PINMUX_IOF1		0x01
#define IT8XXX2_PINMUX_PINS		128

#endif /* __RISCV_ITE_SOC_H_ */
