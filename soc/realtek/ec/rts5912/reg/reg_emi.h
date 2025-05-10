/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_EMI_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_EMI_H

struct emi_reg {
	volatile uint32_t CFG;
	volatile uint32_t INTCTRL;
	volatile uint32_t IRQNUM;
	volatile uint32_t SAR;
	volatile uint32_t INTSTS;
	volatile uint32_t STS;
};

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_EMI_H */
