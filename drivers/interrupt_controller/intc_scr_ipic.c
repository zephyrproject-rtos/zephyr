/*
 * Copyright (c) 2023 Syntacore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syntacore_ipic

/**
 * @file
 * @brief Syntacore Integrated Programmable Interrupt Controller (IPIC) Interface
 *        for RISC-V processors
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/device.h>

#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/intc_scr_ipic.h>
#include <zephyr/irq.h>

#define IPIC_IRQ_LEVEL_HIGH	0
#define IPIC_IRQ_CLEAR_PENDING	(BIT(0))
#define IPIC_IRQ_ENABLE	(BIT(1))
#define IPIC_IRQ_PRIV_MMODE	(BIT(9) | BIT(8))
#define IPIC_IRQ_LN_OFFS	12

#define PLF_IPIC_MBASE	0xbf0
#define IPIC_CISV	(PLF_IPIC_MBASE + 0)
#define IPIC_CICSR	(PLF_IPIC_MBASE + 1)
#define IPIC_IPR	(PLF_IPIC_MBASE + 2)
#define IPIC_ISVR	(PLF_IPIC_MBASE + 3)
#define IPIC_EOI	(PLF_IPIC_MBASE + 4)
#define IPIC_SOI	(PLF_IPIC_MBASE + 5)
#define IPIC_IDX	(PLF_IPIC_MBASE + 6)
#define IPIC_ICSR	(PLF_IPIC_MBASE + 7)
#define IPIC_VOID_VEC	16

#define MK_IRQ_CFG(line, mode, flags) ((mode) | (flags) | ((line) << IPIC_IRQ_LN_OFFS))

static unsigned long ipic_irq_current_vector(void)
{
	return csr_read(IPIC_CISV);
}

static int ipic_irq_setup(int irq_vec, int line, int mode, int flags)
{
	if (IS_ENABLED(CONFIG_IPIC_STATIC_LINE_MAPPING) || irq_vec < 0) {
		irq_vec = line;
	}

	csr_write(IPIC_IDX, irq_vec);
	csr_write(IPIC_ICSR, MK_IRQ_CFG(line, mode, flags | IPIC_IRQ_CLEAR_PENDING));

	return irq_vec;
}

static void ipic_irq_reset(int irq_vec)
{
	ipic_irq_setup(irq_vec, CONFIG_EXT_IPIC_IRQ_LN_NUM,
			IPIC_IRQ_PRIV_MMODE, IPIC_IRQ_CLEAR_PENDING);
}

static void ipic_irq_enable(unsigned int irq_vec)
{
	const unsigned long state =
		(csr_read(IPIC_ICSR) & ~IPIC_IRQ_CLEAR_PENDING) | IPIC_IRQ_ENABLE;
	csr_write(IPIC_IDX, irq_vec);
	csr_write(IPIC_ICSR, state);
}

static void ipic_irq_disable(unsigned int irq_vec)
{
	const unsigned long state =
		csr_read(IPIC_ICSR) & ~(IPIC_IRQ_ENABLE | IPIC_IRQ_CLEAR_PENDING);
	csr_write(IPIC_IDX, irq_vec);
	csr_write(IPIC_ICSR, state);
}

static unsigned long ipic_soi(void)
{
	csr_write(IPIC_SOI, 0);
	return ipic_irq_current_vector();
}

static void ipic_eoi(void)
{
	csr_write(IPIC_EOI, 0);
}

/**
 * @brief Interface to enable a riscv IPIC-specific interrupt line
 *        for IRQS level == 2
 *
 * @param [in] irq_num - IRQ number to enable
 */
void scr_ipic_irq_enable(uint32_t irq_num)
{
	uint32_t key = irq_lock();

	ipic_irq_enable(irq_num);

	irq_unlock(key);
}

/**
 * @brief Interface to disable a riscv IPIC-specific interrupt line
 *        for IRQS level == 2
 *
 * @param [in] irq_num - IRQ number to enable
 */
void scr_ipic_irq_disable(uint32_t irq_num)
{
	uint32_t key = irq_lock();

	ipic_irq_disable(irq_num);

	irq_unlock(key);
}

/**
 * @brief Check if a riscv IPIC-specific interrupt line is enabled
 *
 * @param [in] irq_num - IRQ number
 *
 * @return Integer 1 - if enabled, 0 - otherwise
 */
int scr_ipic_irq_is_enabled(uint32_t irq_num)
{
	return (int)(irq_num & csr_read(IPIC_CISV) && !(irq_num & IPIC_VOID_VEC));
}

/**
 * @brief Handle IPIC external IRQ
 *
 * @param [in] arg - Additional argument to handle IRQ
 */
static void scr_ipic_irq_handler(const void *arg)
{
	unsigned long current_vector = ipic_soi()  + CONFIG_2ND_LVL_ISR_TBL_OFFSET;

	struct _isr_table_entry *ite;

	ite = (struct _isr_table_entry *)&_sw_isr_table[current_vector];
	if (likely(ite)) {
		ite->isr(ite->arg);
	}

	ipic_eoi();
}

static int scr_ipic_init(void)
{
	/* disable interrupts for any line */
	for (int i = 0; i < CONFIG_EXT_IPIC_IRQ_LN_NUM; i++) {
		ipic_irq_reset(i);
	}

	IRQ_CONNECT(RISCV_MACHINE_EXT_IRQ,
			IPIC_IRQ_LEVEL_HIGH,
			scr_ipic_irq_handler,
			NULL,
			0);

	irq_enable(RISCV_MACHINE_EXT_IRQ);

	return 0;
}

SYS_INIT(scr_ipic_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
