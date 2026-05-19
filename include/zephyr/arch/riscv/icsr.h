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

/**
 * @brief Read an indirect CSR register atomically
 * @param index The CSR index to read
 * @return The value of the indirect CSR register
 */
static inline unsigned long micsr_read(unsigned int index)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read(MIREG);

	irq_unlock(key);
	return val;
}

/**
 * @brief Write an indirect CSR register atomically
 * @param index The CSR index to write
 * @param value The value to write to the indirect CSR register
 */
static inline void micsr_write(unsigned int index, unsigned long value)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_write(MIREG, value);

	irq_unlock(key);
}

/**
 * @brief Set bits in an indirect CSR register atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to set
 */
static inline void micsr_set(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_set(MIREG, mask);

	irq_unlock(key);
}

/**
 * @brief Clear bits in an indirect CSR register atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to clear
 */
static inline void micsr_clear(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_clear(MIREG, mask);

	irq_unlock(key);
}

/**
 * @brief Read an indirect CSR register and set bits atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to set
 * @return The previous value of the indirect CSR register
 */
static inline unsigned long micsr_read_set(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read_set(MIREG, mask);

	irq_unlock(key);
	return val;
}

/**
 * @brief Read an indirect CSR register and clear bits atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to clear
 * @return The previous value of the indirect CSR register
 */
static inline unsigned long micsr_read_clear(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read_clear(MIREG, mask);

	irq_unlock(key);
	return val;
}

/**
 * @brief Read an indirect CSR register via MIREG2 atomically
 * @param index The CSR index to read
 * @return The value of the indirect CSR register
 */
static inline unsigned long micsr2_read(unsigned int index)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read(MIREG2);

	irq_unlock(key);
	return val;
}

/**
 * @brief Write an indirect CSR register via MIREG2 atomically
 * @param index The CSR index to write
 * @param value The value to write to the indirect CSR register
 */
static inline void micsr2_write(unsigned int index, unsigned long value)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_write(MIREG2, value);

	irq_unlock(key);
}

/**
 * @brief Set bits in an indirect CSR register via MIREG2 atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to set
 */
static inline void micsr2_set(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_set(MIREG2, mask);

	irq_unlock(key);
}

/**
 * @brief Clear bits in an indirect CSR register via MIREG2 atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to clear
 */
static inline void micsr2_clear(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	csr_clear(MIREG2, mask);

	irq_unlock(key);
}

/**
 * @brief Read an indirect CSR register and set bits via MIREG2 atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to set
 * @return The previous value of the indirect CSR register
 */
static inline unsigned long micsr2_read_set(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read_set(MIREG2, mask);

	irq_unlock(key);
	return val;
}

/**
 * @brief Read an indirect CSR register and clear bits via MIREG2 atomically
 * @param index The CSR index to modify
 * @param mask The bitmask of bits to clear
 * @return The previous value of the indirect CSR register
 */
static inline unsigned long micsr2_read_clear(unsigned int index, unsigned long mask)
{
	unsigned int key = irq_lock();

	csr_write(MISELECT, index);
	unsigned long val = csr_read_clear(MIREG2, mask);

	irq_unlock(key);
	return val;
}

#endif /* CONFIG_RISCV_ISA_EXT_SMCSRIND */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ICSR_H_ */
