/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#ifdef CONFIG_DCACHE
#include <zephyr/cache.h>
#endif

#include <sxsymcrypt/hw.h>

#ifdef CONFIG_XIP
#define sx_attr __ramfunc
#else
#define sx_attr
#endif

#ifndef SX_CM_REGS_ADDR
#define SX_CM_REGS_ADDR ((char *)0xE8000000)
#endif

#ifndef SX_ADDR2BUS
#define SX_ADDR2BUS 0x00000000
#endif

#ifndef SX_TRNG_REGS_OFFSET
#define SX_TRNG_REGS_OFFSET 0x1000
#endif

struct sx_regs {
	char *devmem;
	int slotidx;
};

static const struct sx_regs hwregs[] = {
	{
		.devmem = (char *)(SX_CM_REGS_ADDR),
		.slotidx = 0,
	},
};

static const struct sx_regs trngregs[] = {
	{
		.devmem = (char *)(SX_CM_REGS_ADDR) + (SX_TRNG_REGS_OFFSET),
	},
};

static inline void sx_wrregx(struct sx_regs *regs, uint32_t addr, uint32_t val)
{
	return sys_write32(val, (mem_addr_t)(regs->devmem + addr));
}

static inline void sx_wrregx_addr(struct sx_regs *regs, uint32_t addr, size_t p)
{
#if __riscv_xlen == 32
	return sys_write32((uint32_t)p, (mem_addr_t)(regs->devmem + addr));
#else
	return sys_write64((uint64_t)p, (mem_addr_t)(regs->devmem + addr));
#endif
}

static inline uint32_t sx_rdregx(struct sx_regs *regs, uint32_t addr)
{
	return sys_read32((mem_addr_t)(regs->devmem + addr));
}

sx_attr struct sx_regs *sx_hw_find_regs(unsigned int idx)
{
	if (idx >= ARRAY_SIZE(hwregs)) {
		return NULL;
	}

	return (struct sx_regs *)&hwregs[idx];
}

sx_attr int sx_hw_idx_of_regs(struct sx_regs *regs)
{
	return regs - hwregs;
}

sx_attr struct sx_regs *sx_hw_find_trng_regs(unsigned int idx)
{
	if (idx >= ARRAY_SIZE(trngregs)) {
		return NULL;
	}

	return (struct sx_regs *)&trngregs[idx];
}

sx_attr void sx_wrreg(struct sx_regs *regs, uint32_t addr, uint32_t val)
{
	sx_wrregx(regs, addr, val);
}

sx_attr void sx_wrreg_addr(struct sx_regs *regs, uint32_t addr, struct sxdesc *p)
{
	sx_wrregx_addr(regs, addr, (size_t)p);
}

sx_attr char *sx_map_internal(struct sx_regs *regs, char *dma)
{
	(void)regs;
	return (char *)(dma) + (SX_ADDR2BUS);
}

sx_attr char *sx_map_usrdata(char *s)
{
	return s + (SX_ADDR2BUS);
}

sx_attr char *sx_map_dmadata(char *s)
{
	return s - (SX_ADDR2BUS);
}

sx_attr void sx_flush_tohw(struct sx_regs *regs, char *cpumem, size_t sz)
{
	(void)regs;
	(void)cpumem;
	(void)sz;
#ifdef CONFIG_DCACHE
	cache_data_flush_range((void *)cpumem, sz);
#endif
}

sx_attr void sx_flush_fromhw(struct sx_regs *regs, char *cpumem, size_t offset, size_t sz)
{
	(void)regs;
	(void)cpumem;
	(void)offset;
	(void)sz;
#ifdef CONFIG_DCACHE
	cache_data_invd_range((void *)cpumem, sz);
#endif
}

sx_attr uint32_t sx_rdreg(struct sx_regs *regs, uint32_t addr)
{
	return sx_rdregx(regs, addr);
}

sx_attr void sx_cmdma_wait(struct sx_regs *regs)
{
	(void)regs;
}
