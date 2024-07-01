/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_VBAT_H
#define ENE_KB1200_VBAT_H

/**
 * Structure type to access VBAT Power Domain Module (VBAT).
 */

#define KB1200_BBRAM_SIZE 124

struct vbat_regs {
	volatile uint8_t  BKUPSTS;       /* VBAT Battery-Backed Status Register */
	volatile uint8_t  Reserved0[3];  /* Reserved */
	volatile uint8_t  PASCR[124];    /* VBAT Scratch Register */
	volatile uint8_t  INTRCTR;       /* VBAT INTRUSION# Control register */
	volatile uint8_t  Reserved1[3];  /* Reserved */
	volatile uint32_t Reserved2[7];  /* Reserved */
	volatile uint8_t  MTCCFG;        /* Monotonic Counter Configure register */
	volatile uint8_t  Reserved3[3];  /* Reserved */
	volatile uint8_t  MTCIE;         /* Monotonic Counter Interrupt register */
	volatile uint8_t  Reserved4[3];  /* Reserved */
	volatile uint8_t  MTCPF;         /* Monotonic Counter Pending Flag register */
	volatile uint8_t  Reserved5[3];  /* Reserved */
	volatile uint32_t MTCMAT;        /* Monotonic Counter Match register */
	volatile uint32_t MTCCNT;        /* Monotonic Counter Value register */
};

#define BBRAM_STATUS_IBBR  BIT(7)
#define BBRAM_STATUS_INTRU BIT(6)
#define BBRAM_STATUS_VCC0  BIT(1)
#define BBRAM_STATUS_VCC   BIT(0)

#endif /* ENE_KB1200_VBAT_H */
