/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV32_LITEX_VEXRISCV_SOC_H_
#define __RISCV32_LITEX_VEXRISCV_SOC_H_

#include "../riscv-privilege/common/soc_common.h"
#include <devicetree.h>

#define LITEX_SUBREG_SIZE          0x1
#define LITEX_SUBREG_SIZE_BIT      (LITEX_SUBREG_SIZE * 8)

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE              DT_REG_ADDR(DT_INST(0, mmio_sram))
#define RISCV_RAM_SIZE              DT_REG_SIZE(DT_INST(0, mmio_sram))

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

static inline void litex_write(volatile uint32_t *reg, uint32_t reg_size, uint32_t val)
{
	uint32_t shifted_data, i;
	volatile uint32_t *reg_addr;

	for (i = 0; i < reg_size; ++i) {
		shifted_data = val >> ((reg_size - i - 1) *
					LITEX_SUBREG_SIZE_BIT);
		reg_addr = ((volatile uint32_t *) reg) + i;
		*(reg_addr) = shifted_data;
	}
}

static inline uint32_t litex_read(volatile uint32_t *reg, uint32_t reg_size)
{
	uint32_t shifted_data, i, result = 0;

	for (i = 0; i < reg_size; ++i) {
		shifted_data = *(reg + i) << ((reg_size - i - 1) *
						LITEX_SUBREG_SIZE_BIT);
		result |= shifted_data;
	}

	return result;
}

#endif /* _ASMLANGUAGE */

#endif /* __RISCV32_LITEX_VEXRISCV_SOC_H_ */
