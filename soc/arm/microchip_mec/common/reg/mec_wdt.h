/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_WDT_H
#define _MEC_WDT_H

#include <stdint.h>
#include <stddef.h>

/* Load register */
#define MCHP_WDT_LOAD_REG_OFS		0x00u
#define MCHP_WDT_LOAD_REG_MASK		0xffffu

/* Control register */
#define MCHP_WDT_CTRL_REG_OFS		0x04u
#define MCHP_WDT_CTRL_REG_MASK		0x021du
#define MCHP_WDT_CTRL_EN_POS		0u
#define MCHP_WDT_CTRL_EN_MASK		BIT(MCHP_WDT_CTRL_EN_POS)
#define MCHP_WDT_CTRL_EN		BIT(MCHP_WDT_CTRL_EN_POS)
#define MCHP_WDT_CTRL_HTMR_STALL_POS	2u
#define MCHP_WDT_CTRL_HTMR_STALL_MASK	BIT(MCHP_WDT_CTRL_HTMR_STALL_POS)
#define MCHP_WDT_CTRL_HTMR_STALL_EN	BIT(MCHP_WDT_CTRL_HTMR_STALL_POS)
#define MCHP_WDT_CTRL_WKTMR_STALL_POS	3u
#define MCHP_WDT_CTRL_WKTMR_STALL_MASK	BIT(MCHP_WDT_CTRL_WKTMR_STALL_POS)
#define MCHP_WDT_CTRL_WKTMR_STALL_EN	BIT(MCHP_WDT_CTRL_WKTMR_STALL_POS)
#define MCHP_WDT_CTRL_JTAG_STALL_POS	4u
#define MCHP_WDT_CTRL_JTAG_STALL_MASK	BIT(MCHP_WDT_CTRL_JTAG_STALL_POS)
#define MCHP_WDT_CTRL_JTAG_STALL_EN	BIT(MCHP_WDT_CTRL_JTAG_STALL_POS)
/*
 * WDT mode selecting action taken upon count expiration.
 * 0 = Generate chip reset
 * 1 = Clear this bit,
 *     Set event status
 *     Generate interrupt if event IEN bit is set
 *     Kick WDT causing it to reload from LOAD register
 * If interrupt is enabled in GIRQ21 and NVIC then the EC will jump
 * to the WDT ISR.
 */
#define MCHP_WDT_CTRL_MODE_POS		9u
#define MCHP_WDT_CTRL_MODE_MASK		BIT(MCHP_WDT_CTRL_MODE_POS)
#define MCHP_WDT_CTRL_MODE_RESET	0u
#define MCHP_WDT_CTRL_MODE_IRQ		BIT(MCHP_WDT_CTRL_MODE_POS)

/* WDT Kick register. Write any value to reload counter */
#define MCHP_WDT_KICK_REG_OFS		0x08u
#define MCHP_WDT_KICK_REG_MASK		0xffu
#define MCHP_WDT_KICK_VAL		0

/* WDT Count register. Read only */
#define MCHP_WDT_CNT_RO_REG_OFS		0x0cu
#define MCHP_WDT_CNT_RO_REG_MASK	0xffffu

/* Status Register */
#define MCHP_WDT_STS_REG_OFS		0x10u
#define MCHP_WDT_STS_REG_MASK		0x01u
#define MCHP_WDT_STS_EVENT_IRQ_POS	0u
#define MCHP_WDT_STS_EVENT_IRQ		BIT(MCHP_WDT_STS_EVENT_IRQ_POS)

/* Interrupt Enable Register */
#define MCHP_WDT_IEN_REG_OFS		0x14u
#define MCHP_WDT_IEN_REG_MASK		0x01u
#define MCHP_WDT_IEN_EVENT_IRQ_POS	0u
#define MCHP_WDT_IEN_EVENT_IRQ_MASK	BIT(MCHP_WDT_IEN_EVENT_IRQ_POS)
#define MCHP_WDT_IEN_EVENT_IRQ_EN	BIT(MCHP_WDT_IEN_EVENT_IRQ_POS)

/** @brief Watchdog timer. Size = 24(0x18) */
struct wdt_regs {
	volatile uint16_t LOAD;
	uint8_t RSVD1[2];
	volatile uint16_t CTRL;
	uint8_t RSVD2[2];
	volatile uint8_t KICK;
	uint8_t RSVD3[3];
	volatile uint16_t CNT;
	uint8_t RSVD4[2];
	volatile uint16_t STS;
	uint8_t RSVD5[2];
	volatile uint8_t IEN;
};

#endif	/* #ifndef _MEC_WDT_H */
