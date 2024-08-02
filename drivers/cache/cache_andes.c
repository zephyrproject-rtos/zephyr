/*
 * Copyright (c) 2024 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc_v5.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cache_andes, CONFIG_CACHE_LOG_LEVEL);

/* L1 CCTL Command */
#define CCTL_L1D_VA_INVAL	0
#define CCTL_L1D_VA_WB		1
#define CCTL_L1D_VA_WBINVAL	2
#define CCTL_L1D_WBINVAL_ALL	6
#define CCTL_L1D_WB_ALL		7
#define CCTL_L1I_VA_INVAL	8
#define CCTL_L1D_INVAL_ALL	23
#define CCTL_L1I_IX_INVAL	24

/* mcache_ctl bitfield */
#define MCACHE_CTL_IC_EN	BIT(0)
#define MCACHE_CTL_DC_EN	BIT(1)
#define MCACHE_CTL_CCTL_SUEN	BIT(8)
#define MCACHE_CTL_DC_COHEN	BIT(19)
#define MCACHE_CTL_DC_COHSTA	BIT(20)

/* micm_cfg bitfield */
#define MICM_CFG_ISET		BIT_MASK(3)
#define MICM_CFG_IWAY_SHIFT	3
#define MICM_CFG_ISZ_SHIFT	6

/* mdcm_cfg bitfield */
#define MDCM_CFG_DSZ_SHIFT	6

/* mmsc_cfg bitfield */
#define MMSC_CFG_CCTLCSR	BIT(16)
#define MMSC_CFG_VCCTL_2	BIT(19)
#define MMSC_CFG_MSC_EXT	BIT(31)
#define MMSC_CFG_RVARCH		BIT64(52)

/* mmsc_cfg2 bitfield */
#define MMSC_CFG2_RVARCH	BIT(20)

/* mrvarch_cfg bitfield */
#define MRVARCH_CFG_SMEPMP	BIT(4)

#define K_CACHE_WB		BIT(0)
#define K_CACHE_INVD		BIT(1)
#define K_CACHE_WB_INVD		(K_CACHE_WB | K_CACHE_INVD)

struct cache_config {
	uint32_t instr_line_size;
	uint32_t data_line_size;
	uint32_t l2_cache_size;
	uint32_t l2_cache_inclusive;
};

static struct cache_config cache_cfg;
static struct k_spinlock lock;

#if DT_NODE_HAS_COMPAT_STATUS(DT_INST(0, andestech_l2c), andestech_l2c, okay)
#include "cache_andes_l2.h"
#else
static ALWAYS_INLINE void nds_l2_cache_enable(void) { }
static ALWAYS_INLINE void nds_l2_cache_disable(void) { }
static ALWAYS_INLINE int nds_l2_cache_range(void *addr, size_t size, int op) { return 0; }
static ALWAYS_INLINE int nds_l2_cache_all(int op) { return 0; }
static ALWAYS_INLINE int nds_l2_cache_is_inclusive(void) { return 0; }
static ALWAYS_INLINE int nds_l2_cache_init(void) { return 0; }
#endif /* DT_NODE_HAS_COMPAT_STATUS(DT_INST(0, andestech_l2c), andestech_l2c, okay) */

static ALWAYS_INLINE int nds_cctl_range_operations(void *addr, size_t size, int line_size, int cmd)
{
	unsigned long last_byte, align_addr;
	unsigned long status = csr_read(mstatus);

	last_byte = (unsigned long)addr + size - 1;
	align_addr = ROUND_DOWN(addr, line_size);

	/*
	 * In memory access privilige U mode, applications should use ucctl CSRs
	 * for VA type commands.
	 */
	if ((status & MSTATUS_MPRV) && !(status & MSTATUS_MPP)) {
		while (align_addr <= last_byte) {
			csr_write(NDS_UCCTLBEGINADDR, align_addr);
			csr_write(NDS_UCCTLCOMMAND, cmd);
			align_addr += line_size;
		}
	} else {
		while (align_addr <= last_byte) {
			csr_write(NDS_MCCTLBEGINADDR, align_addr);
			csr_write(NDS_MCCTLCOMMAND, cmd);
			align_addr += line_size;
		}
	}

	return 0;
}

static ALWAYS_INLINE int nds_l1i_cache_all(int op)
{
	unsigned long sets, ways, end;
	unsigned long status = csr_read(mstatus);

	if (csr_read(NDS_MMSC_CFG) & MMSC_CFG_VCCTL_2) {
		/*
		 * In memory access privilige U mode, applications can only use
		 * VA type commands for specific range.
		 */
		if ((status & MSTATUS_MPRV) && !(status & MSTATUS_MPP)) {
			return -ENOTSUP;
		}
	}

	if (op == K_CACHE_INVD) {
		sets = 0x40 << (csr_read(NDS_MICM_CFG) & MICM_CFG_ISET);
		ways = ((csr_read(NDS_MICM_CFG) >> MICM_CFG_IWAY_SHIFT) & BIT_MASK(3)) + 1;
		end = ways * sets * cache_cfg.instr_line_size;

		for (int i = 0; i < end; i += cache_cfg.instr_line_size) {
			csr_write(NDS_MCCTLBEGINADDR, i);
			csr_write(NDS_MCCTLCOMMAND, CCTL_L1I_IX_INVAL);
		}
	}

	return 0;
}

static ALWAYS_INLINE int nds_l1d_cache_all(int op)
{
	unsigned long status = csr_read(mstatus);

	if (csr_read(NDS_MMSC_CFG) & MMSC_CFG_VCCTL_2) {
		/*
		 * In memory access privilige U mode, applications can only use
		 * VA type commands for specific range.
		 */
		if ((status & MSTATUS_MPRV) && !(status & MSTATUS_MPP)) {
			return -ENOTSUP;
		}
	}

	switch (op) {
	case K_CACHE_WB:
		csr_write(NDS_MCCTLCOMMAND, CCTL_L1D_WB_ALL);
		break;
	case K_CACHE_INVD:
		csr_write(NDS_MCCTLCOMMAND, CCTL_L1D_INVAL_ALL);
		break;
	case K_CACHE_WB_INVD:
		csr_write(NDS_MCCTLCOMMAND, CCTL_L1D_WBINVAL_ALL);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static ALWAYS_INLINE int nds_l1i_cache_range(void *addr, size_t size, int op)
{
	unsigned long cmd;

	if (op == K_CACHE_INVD) {
		cmd = CCTL_L1I_VA_INVAL;
		nds_cctl_range_operations(addr, size, cache_cfg.instr_line_size, cmd);
	}

	return 0;
}

static ALWAYS_INLINE int nds_l1d_cache_range(void *addr, size_t size, int op)
{
	unsigned long cmd;

	switch (op) {
	case K_CACHE_WB:
		cmd = CCTL_L1D_VA_WB;
		break;
	case K_CACHE_INVD:
		cmd = CCTL_L1D_VA_INVAL;
		break;
	case K_CACHE_WB_INVD:
		cmd = CCTL_L1D_VA_WBINVAL;
		break;
	default:
		return -ENOTSUP;
	}

	nds_cctl_range_operations(addr, size, cache_cfg.data_line_size, cmd);

	return 0;
}

void cache_data_enable(void)
{
	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		return;
	}

	K_SPINLOCK(&lock) {
		nds_l2_cache_enable();

		/* Enable D-cache coherence management */
		csr_set(NDS_MCACHE_CTL, MCACHE_CTL_DC_COHEN);

		/* Check if CPU support CM or not. */
		if (csr_read(NDS_MCACHE_CTL) & MCACHE_CTL_DC_COHEN) {
			/* Wait for cache coherence enabling completed */
			while (!(csr_read(NDS_MCACHE_CTL) & MCACHE_CTL_DC_COHSTA)) {
				;
			}
		}

		/* Enable D-cache */
		csr_set(NDS_MCACHE_CTL, MCACHE_CTL_DC_EN);
	}
}

void cache_data_disable(void)
{
	unsigned long status = csr_read(mstatus);

	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		return;
	}

	if (csr_read(NDS_MMSC_CFG) & MMSC_CFG_VCCTL_2) {
		if ((status & MSTATUS_MPRV) && !(status & MSTATUS_MPP)) {
			if (!cache_cfg.l2_cache_inclusive) {
				return;
			}
		}
	}

	K_SPINLOCK(&lock) {
		if (cache_cfg.l2_cache_inclusive) {
			nds_l2_cache_all(K_CACHE_WB_INVD);
		} else {
			nds_l1d_cache_all(K_CACHE_WB_INVD);
			nds_l2_cache_all(K_CACHE_WB_INVD);
		}

		csr_clear(NDS_MCACHE_CTL, MCACHE_CTL_DC_EN);

		/* Check if CPU support CM or not. */
		if (csr_read(NDS_MCACHE_CTL) & MCACHE_CTL_DC_COHSTA) {
			csr_clear(NDS_MCACHE_CTL, MCACHE_CTL_DC_COHEN);
			/* Wait for cache coherence disabling completed */
			while (csr_read(NDS_MCACHE_CTL) & MCACHE_CTL_DC_COHSTA) {
				;
			}
		}
		nds_l2_cache_disable();
	}
}

void cache_instr_enable(void)
{
	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		return;
	}

	csr_set(NDS_MCACHE_CTL, MCACHE_CTL_IC_EN);
}

void cache_instr_disable(void)
{
	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		return;
	}

	csr_clear(NDS_MCACHE_CTL, MCACHE_CTL_IC_EN);
}

int cache_data_invd_all(void)
{
	unsigned long ret = 0;

	K_SPINLOCK(&lock) {
		if (cache_cfg.l2_cache_inclusive) {
			ret |= nds_l2_cache_all(K_CACHE_WB);
			ret |= nds_l2_cache_all(K_CACHE_INVD);
		} else {
			ret |= nds_l1d_cache_all(K_CACHE_WB);
			ret |= nds_l2_cache_all(K_CACHE_WB);
			ret |= nds_l2_cache_all(K_CACHE_INVD);
			ret |= nds_l1d_cache_all(K_CACHE_INVD);
		}
	}

	return ret;
}

int cache_data_invd_range(void *addr, size_t size)
{
	unsigned long ret = 0;

	K_SPINLOCK(&lock) {
		if (cache_cfg.l2_cache_inclusive) {
			ret |= nds_l2_cache_range(addr, size, K_CACHE_INVD);
		} else {
			ret |= nds_l2_cache_range(addr, size, K_CACHE_INVD);
			ret |= nds_l1d_cache_range(addr, size, K_CACHE_INVD);
		}
	}

	return ret;
}

int cache_instr_invd_all(void)
{
	unsigned long ret = 0;

	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_RISCV_PMP)) {
		/*  CCTL IX type command is not to RISC-V Smepmp */
		if (IS_ENABLED(CONFIG_64BIT)) {
			if (csr_read(NDS_MMSC_CFG) & MMSC_CFG_RVARCH) {
				if (csr_read(NDS_MRVARCH_CFG) & MRVARCH_CFG_SMEPMP) {
					return -ENOTSUP;
				}
			}
		} else {
			if ((csr_read(NDS_MMSC_CFG) & MMSC_CFG_MSC_EXT) &&
				(csr_read(NDS_MMSC_CFG2) & MMSC_CFG2_RVARCH)) {
				if (csr_read(NDS_MRVARCH_CFG) & MRVARCH_CFG_SMEPMP) {
					return -ENOTSUP;
				}
			}
		}
	}

	K_SPINLOCK(&lock) {
		ret |= nds_l1i_cache_all(K_CACHE_INVD);
	}

	return ret;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	unsigned long ret = 0;

	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		ARG_UNUSED(addr);
		ARG_UNUSED(size);

		return -ENOTSUP;
	}

	K_SPINLOCK(&lock) {
		ret |= nds_l1i_cache_range(addr, size, K_CACHE_INVD);
	}

	return ret;
}

int cache_data_flush_all(void)
{
	unsigned long ret = 0;

	K_SPINLOCK(&lock) {
		if (cache_cfg.l2_cache_inclusive) {
			ret |= nds_l2_cache_all(K_CACHE_WB);
		} else {
			ret |= nds_l1d_cache_all(K_CACHE_WB);
			ret |= nds_l2_cache_all(K_CACHE_WB);
		}
	}

	return ret;
}

int cache_data_flush_range(void *addr, size_t size)
{
	unsigned long ret = 0;

	K_SPINLOCK(&lock) {
		if (cache_cfg.l2_cache_inclusive) {
			ret |= nds_l2_cache_range(addr, size, K_CACHE_WB);
		} else {
			ret |= nds_l1d_cache_range(addr, size, K_CACHE_WB);
			ret |= nds_l2_cache_range(addr, size, K_CACHE_WB);
		}
	}

	return ret;
}

int cache_data_flush_and_invd_all(void)
{
	unsigned long ret = 0;

	K_SPINLOCK(&lock) {
		if (cache_cfg.l2_cache_size) {
			if (cache_cfg.l2_cache_inclusive) {
				ret |= nds_l2_cache_all(K_CACHE_WB_INVD);
			} else {
				ret |= nds_l1d_cache_all(K_CACHE_WB);
				ret |= nds_l2_cache_all(K_CACHE_WB_INVD);
				ret |= nds_l1d_cache_all(K_CACHE_INVD);
			}
		} else {
			ret |= nds_l1d_cache_all(K_CACHE_WB_INVD);
		}
	}

	return ret;
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	unsigned long ret = 0;

	K_SPINLOCK(&lock) {
		if (cache_cfg.l2_cache_size) {
			if (cache_cfg.l2_cache_inclusive) {
				ret |= nds_l2_cache_range(addr, size, K_CACHE_WB_INVD);
			} else {
				ret |= nds_l1d_cache_range(addr, size, K_CACHE_WB);
				ret |= nds_l2_cache_range(addr, size, K_CACHE_WB_INVD);
				ret |= nds_l1d_cache_range(addr, size, K_CACHE_INVD);
			}
		} else {
			ret |= nds_l1d_cache_range(addr, size, K_CACHE_WB_INVD);
		}
	}

	return ret;
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
size_t cache_data_line_size_get(void)
{
	return cache_cfg.data_line_size;
}
#endif /* defined(CONFIG_DCACHE_LINE_SIZE_DETECT) */

#if defined(CONFIG_ICACHE_LINE_SIZE_DETECT)
size_t cache_instr_line_size_get(void)
{
	return cache_cfg.instr_line_size;
}
#endif /* defined(CONFIG_ICACHE_LINE_SIZE_DETECT) */

static int andes_cache_init(void)
{
	unsigned long line_size;

	if (IS_ENABLED(CONFIG_ICACHE)) {
		line_size = (csr_read(NDS_MICM_CFG) >> MICM_CFG_ISZ_SHIFT) & BIT_MASK(3);

		if (line_size == 0) {
			LOG_ERR("Platform doesn't support I-cache, "
				"please disable CONFIG_ICACHE");
		}
#if defined(CONFIG_ICACHE_LINE_SIZE_DETECT)
		/* Icache line size */
		if (line_size <= 5) {
			cache_cfg.instr_line_size = 1 << (line_size + 2);
		} else {
			LOG_ERR("Unknown line size of I-cache");
		}
#elif (CONFIG_ICACHE_LINE_SIZE != 0)
		cache_cfg.instr_line_size = CONFIG_ICACHE_LINE_SIZE;
#elif DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), i_cache_line_size)
		cache_cfg.instr_line_size =
			DT_PROP(DT_PATH(cpus, cpu_0), "i_cache_line_size");
#else
		LOG_ERR("Please specific the i-cache-line-size "
			"CPU0 property of the DT");
#endif /* defined(CONFIG_ICACHE_LINE_SIZE_DETECT) */
	}

	if (IS_ENABLED(CONFIG_DCACHE)) {
		line_size = (csr_read(NDS_MDCM_CFG) >> MDCM_CFG_DSZ_SHIFT) & BIT_MASK(3);
		if (line_size == 0) {
			LOG_ERR("Platform doesn't support D-cache, "
				"please disable CONFIG_DCACHE");
		}
#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
		/* Dcache line size */
		if (line_size <= 5) {
			cache_cfg.data_line_size = 1 << (line_size + 2);
		} else {
			LOG_ERR("Unknown line size of D-cache");
		}
#elif (CONFIG_DCACHE_LINE_SIZE != 0)
		cache_cfg.data_line_size = CONFIG_DCACHE_LINE_SIZE;
#elif DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), d_cache_line_size)
		cache_cfg.data_line_size =
			DT_PROP(DT_PATH(cpus, cpu_0), "d_cache_line_size");
#else
		LOG_ERR("Please specific the d-cache-line-size "
			"CPU0 property of the DT");
#endif /* defined(CONFIG_DCACHE_LINE_SIZE_DETECT) */
	}

	if (!(csr_read(NDS_MMSC_CFG) & MMSC_CFG_CCTLCSR)) {
		LOG_ERR("Platform doesn't support I/D cache operation");
	}

	if (csr_read(NDS_MMSC_CFG) & MMSC_CFG_VCCTL_2) {
		if (IS_ENABLED(CONFIG_PMP_STACK_GUARD)) {
			csr_set(NDS_MCACHE_CTL, MCACHE_CTL_CCTL_SUEN);
		}
	}

	cache_cfg.l2_cache_size = nds_l2_cache_init();
	cache_cfg.l2_cache_inclusive = nds_l2_cache_is_inclusive();

	return 0;
}

SYS_INIT(andes_cache_init, PRE_KERNEL_1, CONFIG_CACHE_ANDES_INIT_PRIORITY);
