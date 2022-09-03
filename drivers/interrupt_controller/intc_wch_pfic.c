/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nuclie's Extended Core Interrupt Controller
 */

#include <zephyr/device.h>

struct PFIC {
	volatile uint32_t ISR[8];
	volatile uint32_t IPR[8];
	volatile uint32_t ITHRESDR;
	volatile uint32_t RESERVED;
	volatile uint32_t CFGR;
	volatile uint32_t GISR;
	volatile uint8_t VTFIDR[4];
	volatile uint8_t RESERVED0[12];
	volatile uint32_t VTFADDR[4];
	volatile uint8_t RESERVED1[0x90];
	volatile uint32_t IENR[8];
	volatile uint8_t RESERVED2[0x60];
	volatile uint32_t IRER[8];
	volatile uint8_t RESERVED3[0x60];
	volatile uint32_t IPSR[8];
	volatile uint8_t RESERVED4[0x60];
	volatile uint32_t IPRR[8];
	volatile uint8_t RESERVED5[0x60];
	volatile uint32_t IACTR[8];
	volatile uint8_t RESERVED6[0xE0];
	volatile uint8_t IPRIOR[256];
	volatile uint8_t RESERVED7[0x810];
	volatile uint32_t SCTLR;
};

#define PFIC (*((volatile struct PFIC *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(pfic), 0))))

/**
 * @brief Enable interrupt
 */
void wch_pfic_irq_enable(uint32_t irq)
{
	PFIC.IENR[irq >> 5] = (1 << (irq & 0x1F));
}

/**
 * @brief Disable interrupt
 */
void wch_pfic_irq_disable(uint32_t irq)
{
	PFIC.IRER[irq >> 5] = (1 << (irq & 0x1F));
}

/**
 * @brief Get enable status of interrupt
 */
int wch_pfic_irq_is_enabled(uint32_t irq)
{
	return (PFIC.ISR[irq >> 5] & (1 << (irq & 0x1F))) ? 1 : 0;
}

/**
 * @brief Set priority and level of interrupt
 */
void wch_pfic_irq_priority_set(uint32_t irq, uint32_t pri, uint32_t flags)
{
	PFIC.IPRIOR[irq] = pri;
}

static int wch_pfic_init(const struct device *dev)
{
	return 0;
}

SYS_INIT(wch_pfic_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
