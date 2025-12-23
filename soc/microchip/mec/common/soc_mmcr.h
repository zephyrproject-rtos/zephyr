/*
 * Copyright (c) 2025 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Microchip MEC MCU family memory mapped control register access
 *
 */

#ifndef _SOC_MICROCHIP_MEC_COMMON_MMCR_H_
#define _SOC_MICROCHIP_MEC_COMMON_MMCR_H_

#include <zephyr/sys/sys_io.h> /* mem_addr_t definition */

/* Zephyr only provides 32-bit version of these routines. We need these for memory
 * mapped control registers located on 16 and 8 bit address boundaries.
 */

static ALWAYS_INLINE void soc_set_bit8(mem_addr_t addr, unsigned int bit)
{
	uint8_t temp = *(volatile uint8_t *)addr;

	*(volatile uint8_t *)addr = temp | (1U << bit);
}

static ALWAYS_INLINE void soc_clear_bit8(mem_addr_t addr, unsigned int bit)
{
	uint8_t temp = *(volatile uint8_t *)addr;

	*(volatile uint8_t *)addr = temp & ~(1U << bit);
}

static ALWAYS_INLINE int soc_test_bit8(mem_addr_t addr, unsigned int bit)
{
	uint8_t temp = *(volatile uint8_t *)addr;

	return temp & (1U << bit);
}

static ALWAYS_INLINE void soc_set_bits8(mem_addr_t addr, unsigned int mask)
{
	uint8_t temp = *(volatile uint8_t *)addr;

	*(volatile uint8_t *)addr = temp | mask;
}

static ALWAYS_INLINE void soc_clear_bits8(mem_addr_t addr, unsigned int mask)
{
	uint8_t temp = *(volatile uint8_t *)addr;

	*(volatile uint8_t *)addr = temp & ~mask;
}

static ALWAYS_INLINE int soc_test_and_set_bit8(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = soc_test_bit8(addr, bit);
	soc_set_bit8(addr, bit);

	return ret;
}

static ALWAYS_INLINE int soc_test_and_clear_bit8(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = soc_test_bit8(addr, bit);
	soc_clear_bit8(addr, bit);

	return ret;
}

static ALWAYS_INLINE void soc_set_bit16(mem_addr_t addr, unsigned int bit)
{
	uint16_t temp = *(volatile uint16_t *)addr;

	*(volatile uint16_t *)addr = temp | (1U << bit);
}

static ALWAYS_INLINE void soc_clear_bit16(mem_addr_t addr, unsigned int bit)
{
	uint16_t temp = *(volatile uint16_t *)addr;

	*(volatile uint16_t *)addr = temp & ~(1U << bit);
}

static ALWAYS_INLINE int soc_test_bit16(mem_addr_t addr, unsigned int bit)
{
	uint16_t temp = *(volatile uint16_t *)addr;

	return temp & (1U << bit);
}

static ALWAYS_INLINE void soc_set_bits16(mem_addr_t addr, unsigned int mask)
{
	uint16_t temp = *(volatile uint16_t *)addr;

	*(volatile uint16_t *)addr = temp | mask;
}

static ALWAYS_INLINE void soc_clear_bits16(mem_addr_t addr, unsigned int mask)
{
	uint16_t temp = *(volatile uint16_t *)addr;

	*(volatile uint16_t *)addr = temp & ~mask;
}

static ALWAYS_INLINE int soc_test_and_set_bit16(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = soc_test_bit16(addr, bit);
	soc_set_bit16(addr, bit);

	return ret;
}

static ALWAYS_INLINE int soc_test_and_clear_bit16(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = soc_test_bit16(addr, bit);
	soc_clear_bit16(addr, bit);

	return ret;
}

#endif /* SOC_MICROCHIP_MEC_COMMON_MMCR_H_ */
