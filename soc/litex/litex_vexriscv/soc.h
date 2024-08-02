/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV32_LITEX_VEXRISCV_SOC_H_
#define __RISCV32_LITEX_VEXRISCV_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/arch/riscv/sys_io.h>

#ifndef _ASMLANGUAGE
/* CSR access helpers */

static inline unsigned char litex_read8(unsigned long addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH >= 8
	return sys_read8(addr);
#else
#error CSR data width less than 8
#endif
}

static inline unsigned short litex_read16(unsigned long addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
	return (sys_read8(addr) << 8)
		| sys_read8(addr + 0x4);
#elif CONFIG_LITEX_CSR_DATA_WIDTH >= 16
	return sys_read16(addr);
#else
#error Unsupported CSR data width
#endif
}

static inline unsigned int litex_read32(unsigned long addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
	return (sys_read8(addr) << 24)
		| (sys_read8(addr + 0x4) << 16)
		| (sys_read8(addr + 0x8) << 8)
		| sys_read8(addr + 0xc);
#elif CONFIG_LITEX_CSR_DATA_WIDTH >= 32
	return sys_read32(addr);
#else
#error Unsupported CSR data width
#endif
}

static inline uint64_t litex_read64(unsigned long addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
	return (((uint64_t)sys_read8(addr)) << 56)
		| ((uint64_t)sys_read8(addr + 0x4) << 48)
		| ((uint64_t)sys_read8(addr + 0x8) << 40)
		| ((uint64_t)sys_read8(addr + 0xc) << 32)
		| ((uint64_t)sys_read8(addr + 0x10) << 24)
		| ((uint64_t)sys_read8(addr + 0x14) << 16)
		| ((uint64_t)sys_read8(addr + 0x18) << 8)
		| (uint64_t)sys_read8(addr + 0x1c);
#elif CONFIG_LITEX_CSR_DATA_WIDTH == 32
	return ((uint64_t)sys_read32(addr) << 32) | (uint64_t)sys_read32(addr + 0x4);
#elif CONFIG_LITEX_CSR_DATA_WIDTH >= 64
	return sys_read64(addr);
#else
#error Unsupported CSR data width
#endif
}

static inline void litex_write8(unsigned char value, unsigned long addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH >= 8
	sys_write8(value, addr);
#else
#error CSR data width less than 8
#endif
}

static inline void litex_write16(unsigned short value, unsigned long addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
	sys_write8(value >> 8, addr);
	sys_write8(value, addr + 0x4);
#elif CONFIG_LITEX_CSR_DATA_WIDTH >= 16
	sys_write16(value, addr);
#else
#error Unsupported CSR data width
#endif
}

static inline void litex_write32(unsigned int value, unsigned long addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
	sys_write8(value >> 24, addr);
	sys_write8(value >> 16, addr + 0x4);
	sys_write8(value >> 8, addr + 0x8);
	sys_write8(value, addr + 0xC);
#elif CONFIG_LITEX_CSR_DATA_WIDTH >= 32
	sys_write32(value, addr);
#else
#error Unsupported CSR data width
#endif
}

/*
 * Operates on uint32_t values only
 * Size is in bytes and meaningful are 1, 2 or 4
 * Address must be aligned to 4 bytes
 */
static inline void litex_write(uint32_t addr, uint32_t size, uint32_t value)
{
	switch (size) {
	case 1:
		litex_write8(value, addr);
		break;
	case 2:
		litex_write16(value, addr);
		break;
	case 4:
		litex_write32(value, addr);
		break;
	default:
		break;
	}
}

/*
 * Operates on uint32_t values only
 * Size is in bytes and meaningful are 1, 2 or 4
 * Address must be aligned to 4 bytes
 */
static inline uint32_t litex_read(uint32_t addr, uint32_t size)
{
	switch (size) {
	case 1:
		return litex_read8(addr);
	case 2:
		return litex_read16(addr);
	case 4:
		return litex_read32(addr);
	default:
		return 0;
	}
}

#endif /* _ASMLANGUAGE */

#endif /* __RISCV32_LITEX_VEXRISCV_SOC_H_ */
