/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is a driver for the GRLIB IRQMP interrupt controller common in LEON
 * systems.
 *
 * Interrupt level 1..15 are SPARC interrupts. Interrupt level 16..31, if
 * implemented in the interrupt controller, are IRQMP "extended interrupts".
 *
 * For more information about IRQMP, see the GRLIB IP Core User's Manual.
 */

#define DT_DRV_COMPAT gaisler_irqmp

#include <zephyr/kernel.h>
#include <zephyr/init.h>

/*
 * Register description for IRQMP and IRQAMP interrupt controllers
 * IRQMP      - Multiprocessor Interrupt Controller
 * IRQ(A)MP   - Multiprocessor Interrupt Controller with extended ASMP support
 */
#define IRQMP_NCPU_MAX 16
struct irqmp_regs {
	uint32_t ilevel;                                /* 0x00 */
	uint32_t ipend;                                 /* 0x04 */
	uint32_t iforce0;                               /* 0x08 */
	uint32_t iclear;                                /* 0x0c */
	uint32_t mpstat;                                /* 0x10 */
	uint32_t brdlst;                                /* 0x14 */
	uint32_t errstat;                               /* 0x18 */
	uint32_t wdogctrl;                              /* 0x1c */
	uint32_t asmpctrl;                              /* 0x20 */
	uint32_t icselr[2];                             /* 0x24 */
	uint32_t reserved2c;                            /* 0x2c */
	uint32_t reserved30;                            /* 0x30 */
	uint32_t reserved34;                            /* 0x34 */
	uint32_t reserved38;                            /* 0x38 */
	uint32_t reserved3c;                            /* 0x3c */
	uint32_t pimask[IRQMP_NCPU_MAX];                /* 0x40 */
	uint32_t piforce[IRQMP_NCPU_MAX];               /* 0x80 */
	uint32_t pextack[IRQMP_NCPU_MAX];               /* 0xc0 */
};

#define IRQMP_PEXTACK_EID       (0x1f << 0)

static volatile struct irqmp_regs *get_irqmp_regs(void)
{
	return (struct irqmp_regs *) DT_INST_REG_ADDR(0);
}

static int get_irqmp_eirq(void)
{
	return DT_INST_PROP(0, eirq);
}

void arch_irq_enable(unsigned int source)
{
	volatile struct irqmp_regs *regs = get_irqmp_regs();
	volatile uint32_t *pimask = &regs->pimask[0];
	const uint32_t setbit = (1U << source);
	unsigned int key;

	key = arch_irq_lock();
	*pimask |= setbit;
	arch_irq_unlock(key);
}

void arch_irq_disable(unsigned int source)
{
	volatile struct irqmp_regs *regs = get_irqmp_regs();
	volatile uint32_t *pimask = &regs->pimask[0];
	const uint32_t keepbits = ~(1U << source);
	unsigned int key;

	key = arch_irq_lock();
	*pimask &= keepbits;
	arch_irq_unlock(key);
}

int arch_irq_is_enabled(unsigned int source)
{
	volatile struct irqmp_regs *regs = get_irqmp_regs();
	volatile uint32_t *pimask = &regs->pimask[0];

	return !!(*pimask & (1U << source));
}

int z_sparc_int_get_source(int irl)
{
	volatile struct irqmp_regs *regs = get_irqmp_regs();
	const int eirq = get_irqmp_eirq();
	int source;

	if ((eirq != 0) && (irl == eirq)) {
		source = regs->pextack[0] & IRQMP_PEXTACK_EID;
		if (source == 0) {
			source = irl;
		}
	} else {
		source = irl;
	}

	return source;
}

static int irqmp_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	volatile struct irqmp_regs *regs = get_irqmp_regs();

	regs->ilevel = 0;
	regs->ipend = 0;
	regs->iforce0 = 0;
	regs->pimask[0] = 0;
	regs->piforce[0] = 0xfffe0000;

	return 0;
}

SYS_INIT(irqmp_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
