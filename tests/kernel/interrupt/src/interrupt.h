/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <kernel_structs.h>


#if defined(CONFIG_ARM)
#include <arch/arm/cortex_m/cmsis.h>
static void trigger_irq(int irq)
{
	printk("Triggering irq : %d\n", irq);
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU) || defined(CONFIG_CPU_CORTEX_M0) \
	|| defined(CONFIG_CPU_CORTEX_M0PLUS)
	NVIC_SetPendingIRQ(irq);
#else
	NVIC->STIR = irq;
#endif
}

#elif defined(CONFIG_RISCV32)
static void trigger_irq(int irq)
{
	u32_t mip;

	printk("Triggering irq : %d\n", irq);
	__asm__ volatile ("csrrs %0, mip, %1\n"
			: "=r" (mip)
			: "r" (1 << irq));
}

#elif defined(CONFIG_CPU_ARCV2)
static void trigger_irq(int irq)
{
	printk("Triggering irq : %d\n", irq);
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, irq);
}
#else
/* for not supported architecture */
#define NO_TRIGGER_FROM_SW
#endif

