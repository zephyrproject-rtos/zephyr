/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V indirect CSR (iCSR) access helpers with IRQ locking
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ICSR_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ICSR_H_

#include <zephyr/arch/riscv/csr.h>
#include <zephyr/irq.h>

#ifdef CONFIG_RISCV_ISA_EXT_SMCSRIND

static inline unsigned long micsr_read(unsigned int index)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read(MIREG);

	irq_unlock(key);
	return val;
}

static inline void micsr_write(unsigned int index, unsigned long value)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_write(MIREG, value);

	irq_unlock(key);
}

static inline void micsr_set(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_set(MIREG, mask);

	irq_unlock(key);
}

static inline void micsr_clear(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_clear(MIREG, mask);

	irq_unlock(key);
}

static inline unsigned long micsr_read_set(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read_set(MIREG, mask);

	irq_unlock(key);
	return val;
}

static inline unsigned long micsr_read_clear(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read_clear(MIREG, mask);

	irq_unlock(key);
	return val;
}

#endif /* CONFIG_RISCV_ISA_EXT_SMCSRIND */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ICSR_H_ */
