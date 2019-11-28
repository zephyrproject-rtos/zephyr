/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV32_LITEX_VEXRISCV_SOC_H_
#define __RISCV32_LITEX_VEXRISCV_SOC_H_

#include "../riscv-privilege/common/soc_common.h"
#include <generated_dts_board.h>

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE              DT_INST_0_MMIO_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE              DT_INST_0_MMIO_SRAM_SIZE

#ifndef _ASMLANGUAGE
/* CSR access helpers */

static inline unsigned char litex_read8(unsigned long addr)
{
	return sys_read8(addr);
}

static inline unsigned short litex_read16(unsigned long addr)
{
	return (sys_read8(addr) << 8)
		| sys_read8(addr + 0x4);
}

static inline unsigned int litex_read32(unsigned long addr)
{
	return (sys_read8(addr) << 24)
		| (sys_read8(addr + 0x4) << 16)
		| (sys_read8(addr + 0x8) << 8)
		| sys_read8(addr + 0xc);
}

static inline void litex_write8(unsigned char value, unsigned long addr)
{
	sys_write8(value, addr);
}

static inline void litex_write16(unsigned short value, unsigned long addr)
{
	sys_write8(value >> 8, addr);
	sys_write8(value, addr + 0x4);
}

static inline void litex_write32(unsigned int value, unsigned long addr)
{
	sys_write8(value >> 24, addr);
	sys_write8(value >> 16, addr + 0x4);
	sys_write8(value >> 8, addr + 0x8);
	sys_write8(value, addr + 0xC);
}
#endif /* _ASMLANGUAGE */

#endif /* __RISCV32_LITEX_VEXRISCV_SOC_H_ */
