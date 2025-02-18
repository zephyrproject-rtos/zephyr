/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_EMI_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_EMI_H

typedef struct {
	uint32_t CFG;
	uint32_t INTCTRL;
	uint32_t IRQNUM;
	uint32_t SAR;
	uint32_t INTSTS;
} EMI_Type;

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_EMI_H */
