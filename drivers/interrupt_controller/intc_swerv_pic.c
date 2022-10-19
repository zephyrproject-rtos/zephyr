/*
 * Copyright (c) 2019 Western Digital Corporation or its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT swerv_pic

/**
 * @brief SweRV EH1 PIC driver
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>

#define SWERV_PIC_MAX_NUM	CONFIG_NUM_IRQS
#define SWERV_PIC_MAX_ID	(SWERV_PIC_MAX_NUM + RISCV_MAX_GENERIC_IRQ)
#define SWERV_PIC_MAX_PRIO		16

#define SWERV_PIC_mpiccfg		0x3000
#define SWERV_PIC_meipl(s)		(0x0 + (s)*4)
#define SWERV_PIC_meip(x)		(0x1000 + (x)*4)
#define SWERV_PIC_meie(s)		(0x2000 + (s)*4)
#define SWERV_PIC_meigwctrl(s)		(0x4000 + (s)*4)
#define SWERV_PIC_meigwclr(s)		(0x5000 + (s)*4)

#define SWERV_PIC_meivt			"0xBC8"
#define SWERV_PIC_meipt			"0xBC9"
#define SWERV_PIC_meicpct		"0xBCA"
#define SWERV_PIC_meicidpl		"0xBCB"
#define SWERV_PIC_meicurpl		"0xBCC"
#define SWERV_PIC_meihap		"0xFC8"

#define swerv_piccsr(csr) SWERV_PIC_##csr

#define swerv_pic_readcsr(csr, value) \
	volatile("csrr %0, "swerv_piccsr(csr) : "=r" (value))
#define swerv_pic_writecsr(csr, value) \
	volatile("csrw "swerv_piccsr(csr)", %0" :: "rK" (value))

static int save_irq;

static uint32_t swerv_pic_read(uint32_t reg)
{
	return *(volatile uint32_t *)(DT_INST_REG_ADDR(0) + reg);
}

static void swerv_pic_write(uint32_t reg, uint32_t val)
{
	*(volatile uint32_t *)(DT_INST_REG_ADDR(0) + reg) = val;
}

void swerv_pic_irq_enable(uint32_t irq)
{
	uint32_t key;

	if ((irq >= SWERV_PIC_MAX_ID) || (irq < RISCV_MAX_GENERIC_IRQ)) {
		return;
	}

	key = irq_lock();
	swerv_pic_write(SWERV_PIC_meie(irq - RISCV_MAX_GENERIC_IRQ), 1);
	irq_unlock(key);
}

void swerv_pic_irq_disable(uint32_t irq)
{
	uint32_t key;

	if ((irq >= SWERV_PIC_MAX_ID) || (irq < RISCV_MAX_GENERIC_IRQ)) {
		return;
	}

	key = irq_lock();
	swerv_pic_write(SWERV_PIC_meie(irq - RISCV_MAX_GENERIC_IRQ), 0);
	irq_unlock(key);
}

int swerv_pic_irq_is_enabled(uint32_t irq)
{
	if ((irq >= SWERV_PIC_MAX_ID) || (irq < RISCV_MAX_GENERIC_IRQ)) {
		return -1;
	}

	return swerv_pic_read(SWERV_PIC_meie(irq - RISCV_MAX_GENERIC_IRQ))
	  & 0x1;
}

void swerv_pic_set_priority(uint32_t irq, uint32_t priority)
{
	uint32_t key;

	if (irq <= RISCV_MAX_GENERIC_IRQ) {
		return;
	}

	if ((irq >= SWERV_PIC_MAX_ID) || (irq < RISCV_MAX_GENERIC_IRQ)) {
		return;
	}

	if (priority >= SWERV_PIC_MAX_PRIO) {
		return;
	}

	key = irq_lock();
	swerv_pic_write(SWERV_PIC_meipl(irq - RISCV_MAX_GENERIC_IRQ), priority);
	irq_unlock(key);
}

int swerv_pic_get_irq(void)
{
	return save_irq;
}

static void swerv_pic_irq_handler(const void *arg)
{
	uint32_t tmp;
	uint32_t irq;
	struct _isr_table_entry *ite;

	/* trigger the capture of the interrupt source ID */
	__asm__ swerv_pic_writecsr(meicpct, 0);

	__asm__ swerv_pic_readcsr(meihap, tmp);
	irq = (tmp >> 2) & 0xff;

	save_irq = irq;

	if (irq == 0U || irq >= 64) {
		z_irq_spurious(NULL);
	}
	irq += RISCV_MAX_GENERIC_IRQ;

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = (struct _isr_table_entry *)&_sw_isr_table[irq];
	if (ite->isr) {
		ite->isr(ite->arg);
	}

	swerv_pic_write(SWERV_PIC_meigwclr(irq), 0);
}

static int swerv_pic_init(void)
{
	int i;

	/* Init priority order to 0, 0=lowest to 15=highest */
	swerv_pic_write(SWERV_PIC_mpiccfg, 0);

	/* Ensure that all interrupts are disabled initially */
	for (i = 1; i < SWERV_PIC_MAX_ID; i++) {
		swerv_pic_write(SWERV_PIC_meie(i), 0);
	}

	/* Set priority of each interrupt line to 0 initially */
	for (i = 1; i < SWERV_PIC_MAX_ID; i++) {
		swerv_pic_write(SWERV_PIC_meipl(i), 15);
	}

	/* Set property of each interrupt line to level-triggered/high */
	for (i = 1; i < SWERV_PIC_MAX_ID; i++) {
		swerv_pic_write(SWERV_PIC_meigwctrl(i), (0<<1)|(0<<0));
	}

	/* clear pending of each interrupt line */
	for (i = 1; i < SWERV_PIC_MAX_ID; i++) {
		swerv_pic_write(SWERV_PIC_meigwclr(i), 0);
	}

	/* No interrupts masked */
	__asm__ swerv_pic_writecsr(meipt, 0);
	__asm__ swerv_pic_writecsr(meicidpl, 0);
	__asm__ swerv_pic_writecsr(meicurpl, 0);

	/* Setup IRQ handler for SweRV PIC driver */
	IRQ_CONNECT(RISCV_MACHINE_EXT_IRQ,
		    0,
		    swerv_pic_irq_handler,
		    NULL,
		    0);

	/* Enable IRQ for SweRV PIC driver */
	irq_enable(RISCV_MACHINE_EXT_IRQ);

	return 0;
}

void arch_irq_enable(unsigned int irq)
{
	uint32_t mie;

	if (irq > RISCV_MAX_GENERIC_IRQ) {
		swerv_pic_irq_enable(irq);
		return;
	}

	/*
	 * CSR mie register is updated using atomic instruction csrrs
	 * (atomic read and set bits in CSR register)
	 */
	__asm__ volatile ("csrrs %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
}

void arch_irq_disable(unsigned int irq)
{
	uint32_t mie;

	if (irq > RISCV_MAX_GENERIC_IRQ) {
		swerv_pic_irq_disable(irq);
		return;
	}

	/*
	 * Use atomic instruction csrrc to disable device interrupt in mie CSR.
	 * (atomic read and clear bits in CSR register)
	 */
	__asm__ volatile ("csrrc %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));
};

int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t mie;

	if (irq > RISCV_MAX_GENERIC_IRQ)
		return swerv_pic_irq_is_enabled(irq);

	__asm__ volatile ("csrr %0, mie" : "=r" (mie));

	return !!(mie & (1 << irq));
}

SYS_INIT(swerv_pic_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
