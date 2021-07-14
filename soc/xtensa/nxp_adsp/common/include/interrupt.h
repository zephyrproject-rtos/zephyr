/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Daniel Baluta <daniel.baluta@nxp.com>
 */

/* IRQ numbers */
#if CONFIG_INTERRUPT_LEVEL_1

#define IRQ_NUM_SOFTWARE0	8	/* Level 1 */

#define IRQ_MASK_SOFTWARE0	BIT(IRQ_NUM_SOFTWARE0)

#endif

#if CONFIG_INTERRUPT_LEVEL_2

#define IRQ_NUM_TIMER0		2	/* Level 2 */
#define IRQ_NUM_MU		7	/* Level 2 */
#define IRQ_NUM_SOFTWARE1	9	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP0	19	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP1	20	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP2	21	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP3	22	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP4	23	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP5	24	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP6	25	/* Level 2 */
#define IRQ_NUM_IRQSTR_DSP7	26	/* Level 2 */

#define IRQ_MASK_TIMER0		BIT(IRQ_NUM_TIMER0)
#define IRQ_MASK_MU		BIT(IRQ_NUM_MU)
#define IRQ_MASK_SOFTWARE1	BIT(IRQ_NUM_SOFTWARE1)
#define IRQ_MASK_IRQSTR_DSP0	BIT(IRQ_NUM_IRQSTR_DSP0)
#define IRQ_MASK_IRQSTR_DSP1	BIT(IRQ_NUM_IRQSTR_DSP1)
#define IRQ_MASK_IRQSTR_DSP2	BIT(IRQ_NUM_IRQSTR_DSP2)
#define IRQ_MASK_IRQSTR_DSP3	BIT(IRQ_NUM_IRQSTR_DSP3)
#define IRQ_MASK_IRQSTR_DSP4	BIT(IRQ_NUM_IRQSTR_DSP4)
#define IRQ_MASK_IRQSTR_DSP5	BIT(IRQ_NUM_IRQSTR_DSP5)
#define IRQ_MASK_IRQSTR_DSP6	BIT(IRQ_NUM_IRQSTR_DSP6)
#define IRQ_MASK_IRQSTR_DSP7	BIT(IRQ_NUM_IRQSTR_DSP7)

#endif

#if CONFIG_INTERRUPT_LEVEL_3

#define IRQ_NUM_TIMER1		3	/* Level 3 */

#define IRQ_MASK_TIMER1		BIT(IRQ_NUM_TIMER1)

#endif

/* 32 HW interrupts + 8 IRQ_STEER lines each with 64 interrupts */
#define PLATFORM_IRQ_HW_NUM	XCHAL_NUM_INTERRUPTS
#define PLATFORM_IRQ_CHILDREN	64 /* Each cascaded struct covers 64 IRQs */
/* IMX: Covered steer IRQs are modulo-64 aligned. */
#define PLATFORM_IRQ_FIRST_CHILD  0

/* irqstr_get_sof_int() - Convert IRQ_STEER interrupt to SOF logical
 * interrupt
 * @irqstr_int Shared IRQ_STEER interrupt
 *
 * Get the SOF interrupt number for a shared IRQ_STEER interrupt number.
 * The IRQ_STEER number is the one specified in the hardware description
 * manuals, while the SOF interrupt number is the one usable with the
 * interrupt_register and interrupt_enable functions.
 *
 * Return: SOF interrupt number
 */
int irqstr_get_sof_int(int irqstr_int);

#if defined CONFIG_IMX8
#define IRQSTR_BASE_ADDR	0x510A0000
#elif defined CONFIG_IMX8X
#define IRQSTR_BASE_ADDR	0x51080000
#else
#error "Unsupported i.MX8 platform detected"
#endif

/* The MASK, SET (unused) and STATUS registers are 512-bit registers
 * split into 16 32-bit registers that we can directly access.
 *
 * The interrupts are mapped in the registers in the following way:
 * Interrupts 480-511 at offset 0
 * Interrupts 448-479 at offset 1
 * Interrupts 416-447 at offset 2
 * ...
 * Interrupts 64-95 at offset 13
 * Interrupts 32-63 at offset 14
 * Interrupts 0-31 at offset 15
 */

#define IRQSTR_CHANCTL			0x00
#define IRQSTR_CH_MASK(n)		(0x04 + 0x04 * (15 - (n)))
#define IRQSTR_CH_SET(n)		(0x44 + 0x04 * (15 - (n)))
#define IRQSTR_CH_STATUS(n)		(0x84 + 0x04 * (15 - (n)))
#define IRQSTR_MASTER_DISABLE		0xC4
#define IRQSTR_MASTER_STATUS		0xC8

#define IRQSTR_RESERVED_IRQS_NUM	32
#ifdef CONFIG_IMX8M
#define IRQSTR_IRQS_NUM			192
#else
#define IRQSTR_IRQS_NUM			512
#endif
#define IRQSTR_IRQS_NUM			512
#define IRQSTR_IRQS_REGISTERS_NUM	16
#define IRQSTR_IRQS_PER_LINE		64

int interrupt_get_irq(unsigned int irq, const char *cascade);
