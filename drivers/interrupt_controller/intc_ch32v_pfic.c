/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch32v_pfic

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_ch32v_pfic.h>

#define PFIC_BASE DT_INST_REG_ADDR(0)

#define PFIC_R32_ISR(n)      (PFIC_BASE + 0x000 + 0x4 * n)
#define PFIC_R32_IPR(n)      (PFIC_BASE + 0x020 + 0x4 * n)
#define PFIC_R32_ITHRESDR    (PFIC_BASE + 0x040)
#define PFIC_R32_VTFBADDRR   (PFIC_BASE + 0x044)
#define PFIC_R32_CFGR        (PFIC_BASE + 0x048)
#define PFIC_R32_GISR        (PFIC_BASE + 0x04C)
#define PFIC_R32_IDCFGR      (PFIC_BASE + 0x050)
#define PFIC_R32_VTFADDRR(n) (PFIC_BASE + 0x060 + 0x4 * n)
#define PFIC_R32_IENR(n)     (PFIC_BASE + 0x100 + 0x4 * n)
#define PFIC_R32_IRER(n)     (PFIC_BASE + 0x180 + 0x4 * n)
#define PFIC_R32_IPSR(n)     (PFIC_BASE + 0x200 + 0x4 * n)
#define PFIC_R32_IPRR(n)     (PFIC_BASE + 0x280 + 0x4 * n)
#define PFIC_R32_IACTR(n)    (PFIC_BASE + 0x300 + 0x4 * n)
#define PFIC_R32_IPRIOR(n)   (PFIC_BASE + 0x400 + n)
#define PFIC_R32_SCTLR       (PFIC_BASE + 0xD10)

/* PFIC_R32_ITHRESDR */
#define ITHRESDR_THRESHOLD(n) ((n & BIT_MASK(4)) << 4)

#define PFIC_IRQN_GROUP(irqn) (irqn >> 5)
#define PFIC_IRQN_SHIFT(irqn) (irqn & BIT_MASK(5))

void ch32v_pfic_enable(unsigned int irq)
{
	sys_write32(BIT(PFIC_IRQN_SHIFT(irq)), PFIC_R32_IENR(PFIC_IRQN_GROUP(irq)));
}

void ch32v_pfic_disable(unsigned int irq)
{
	uint32_t regval;

	/* Temporarily mask ISRs with priority lower than SYSTICK (1) */
	regval = sys_read32(PFIC_R32_ITHRESDR);
	sys_write32(ITHRESDR_THRESHOLD(1), PFIC_R32_ITHRESDR);

	sys_write32(BIT(PFIC_IRQN_SHIFT(irq)), PFIC_R32_IRER(PFIC_IRQN_GROUP(irq)));

	/* Restore the original threshold */
	sys_write32(regval, PFIC_R32_ITHRESDR);
}

int ch32v_pfic_is_enabled(unsigned int irq)
{
	return sys_read32(PFIC_R32_ISR(PFIC_IRQN_GROUP(irq))) & BIT(PFIC_IRQN_SHIFT(irq));
}

void ch32v_pfic_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	sys_write8(prio, PFIC_R32_IPRIOR(irq));
}
