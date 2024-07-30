/*
 * Copyright (c) 2023 Frontgrade Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_GRGPIO_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_GRGPIO_H_

struct grgpio_regs {
	uint32_t data;        /* 0x00 I/O port data register */
	uint32_t output;      /* 0x04 I/O port output register */
	uint32_t dir;         /* 0x08 I/O port direction register */
	uint32_t imask;       /* 0x0C Interrupt mask register */
	uint32_t ipol;        /* 0x10 Interrupt polarity register */
	uint32_t iedge;       /* 0x14 Interrupt edge register */
	uint32_t bypass;      /* 0x18 Bypass register */
	uint32_t cap;         /* 0x1C Capability register */
	uint32_t irqmap[4];   /* 0x20 - 0x2C Interrupt map registers */
	uint32_t res_30;      /* 0x30 Reserved */
	uint32_t res_34;      /* 0x34 Reserved */
	uint32_t res_38;      /* 0x38 Reserved */
	uint32_t res_3C;      /* 0x3C Reserved */
	uint32_t iavail;      /* 0x40 Interrupt available register */
	uint32_t iflag;       /* 0x44 Interrupt flag register */
	uint32_t res_48;      /* 0x48 Reserved */
	uint32_t pulse;       /* 0x4C Pulse register */
	uint32_t res_50;      /* 0x50 Reserved */
	uint32_t output_or;   /* 0x54 I/O port output register, logical-OR */
	uint32_t dir_or;      /* 0x58 I/O port dir. register, logical-OR */
	uint32_t imask_or;    /* 0x5C Interrupt mask register, logical-OR */
	uint32_t res_60;      /* 0x60 Reserved */
	uint32_t output_and;  /* 0x64 I/O port output register, logical-AND */
	uint32_t dir_and;     /* 0x68 I/O port dir. register, logical-AND */
	uint32_t imask_and;   /* 0x6C Interrupt mask register, logical-AND */
	uint32_t res_70;      /* 0x70 Reserved */
	uint32_t output_xor;  /* 0x74 I/O port output register, logical-XOR */
	uint32_t dir_xor;     /* 0x78 I/O port dir. register, logical-XOR */
	uint32_t imask_xor;   /* 0x7C Interrupt mask register, logical-XOR */
};

#define GRGPIO_CAP_PU_BIT       18
#define GRGPIO_CAP_IER_BIT      17
#define GRGPIO_CAP_IFL_BIT      16
#define GRGPIO_CAP_IRQGEN_BIT    8
#define GRGPIO_CAP_NLINES_BIT    0

#define GRGPIO_CAP_PU           (0x1 << GRGPIO_CAP_PU_BIT)
#define GRGPIO_CAP_IER          (0x1 << GRGPIO_CAP_IER_BIT)
#define GRGPIO_CAP_IFL          (0x1 << GRGPIO_CAP_IFL_BIT)
#define GRGPIO_CAP_IRQGEN       (0x1f << GRGPIO_CAP_IRQGEN_BIT)
#define GRGPIO_CAP_NLINES       (0x1f << GRGPIO_CAP_NLINES_BIT)

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_GRGPIO_H_ */
