/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_EMI_H
#define _MEC172X_EMI_H

#include <stdint.h>
#include <stddef.h>

/** @brief Embedded Memory Interface (EMI) Registers */
struct emi_regs {
	volatile uint8_t RT_HOST_TO_EC;
	volatile uint8_t RT_EC_TO_HOST;
	volatile uint8_t EC_ADDR_LSB;
	volatile uint8_t EC_ADDR_MSB;
	volatile uint8_t EC_DATA_0;		/* +0x04 */
	volatile uint8_t EC_DATA_1;
	volatile uint8_t EC_DATA_2;
	volatile uint8_t EC_DATA_3;
	volatile uint8_t INTR_SRC_LSB;	/* +0x08 */
	volatile uint8_t INTR_SRC_MSB;
	volatile uint8_t INTR_MSK_LSB;
	volatile uint8_t INTR_MSK_MSB;
	volatile uint8_t APPID;			/* +0x0C */
	uint8_t RSVD1[3];
	volatile uint8_t APPID_ASSGN;	/* +0x10 */
	uint8_t RSVD2[3];
	uint32_t RSVD3[(0x100 - 0x14) / 4];
	volatile uint8_t HOST_TO_EC;	/* +0x100 */
	volatile uint8_t EC_TO_HOST;
	uint16_t RSVD4[1];
	volatile uint32_t MEM_BA_0;		/* +0x104 */
	volatile uint16_t MEM_RL_0;		/* +0x108 */
	volatile uint16_t MEM_WL_0;
	volatile uint32_t MEM_BA_1;		/* +0x10C */
	volatile uint16_t MEM_RL_1;		/* +0x110 */
	volatile uint16_t MEM_WL_1;
	volatile uint16_t INTR_SET;		/* +0x114 */
	volatile uint16_t HOST_CLR_EN;	/* +0x116 */
	uint32_t RSVD5[2];
	volatile uint32_t APPID_STS_1;	/* +0x120 */
	volatile uint32_t APPID_STS_2;
	volatile uint32_t APPID_STS_3;
	volatile uint32_t APPID_STS_4;
};

#endif /* #ifndef _MEC172X_EMI_H */
