/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_GPTD_H
#define ENE_KB1200_GPTD_H

/**
 *  Structure type to access GPIO Trigger Detector (GPTD).
 */
struct gptd_regs {
	volatile uint32_t GPTDIE;  /*Interrupt Enable Register */
	volatile uint32_t Reserved1[3];
	volatile uint32_t GPTDPF; /*Event Pending Flag Register */
	volatile uint32_t Reserved2[3];
	volatile uint32_t GPTDCHG; /*Change Trigger Register */
	volatile uint32_t Reserved3[3];
	volatile uint32_t GPTDEL; /*Level/Edge Trigger Register */
	volatile uint32_t Reserved4[3];
	volatile uint32_t GPTDPS; /*Polarity Selection Register */
	volatile uint32_t Reserved5[3];
	volatile uint32_t GPTDWE; /*WakeUP Enable Register */
	volatile uint32_t Reserved6[3];
};

#endif /* ENE_KB1200_GPTD_H */
