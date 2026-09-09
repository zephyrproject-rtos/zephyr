/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

/* Set in rv_exception_handler */
volatile bool _trap_active;

#define PLIC_BASE_ADDR DT_REG_ADDR(DT_INST(0, sifive_plic_1_0_0))

extern void rv_exception_handler(void);

static int plic_vectored_enable(void)
{
	/*
	 * Enable vectored mode in the PLIC. Must be performed after plic_init,
	 * since the feature enable register is cleared at this point
	 */
	sys_write32(BIT(1), PLIC_BASE_ADDR);
	return 0;
}

SYS_INIT(plic_vectored_enable, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Install rv_exception_handler at mtvec[0] */
Z_ISR_DECLARE_DIRECT(0, ISR_FLAG_DIRECT, rv_exception_handler);
