/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INC_SOC_H
#define __INC_SOC_H

#include <string.h>
#include <errno.h>
#include <arch/xtensa/cache.h>

/* macros related to interrupt handling */
#define XTENSA_IRQ_NUM_SHIFT			0
#define CAVS_IRQ_NUM_SHIFT			8
#define XTENSA_IRQ_NUM_MASK			0xff
#define CAVS_IRQ_NUM_MASK			0xff

/*
 * IRQs are mapped on 2 levels. 3rd and 4th level are left as 0x00.
 *
 * 1. Peripheral Register bit offset.
 * 2. CAVS logic bit offset.
 */
#define XTENSA_IRQ_NUMBER(_irq) \
	((_irq >> XTENSA_IRQ_NUM_SHIFT) & XTENSA_IRQ_NUM_MASK)
#define CAVS_IRQ_NUMBER(_irq) \
	(((_irq >> CAVS_IRQ_NUM_SHIFT) & CAVS_IRQ_NUM_MASK) - 1)

/* Macro that aggregates the bi-level interrupt into an IRQ number */
#define SOC_AGGREGATE_IRQ(cavs_irq, core_irq)		\
	( \
	 ((core_irq & XTENSA_IRQ_NUM_MASK) << XTENSA_IRQ_NUM_SHIFT) | \
	 (((cavs_irq + 1) & CAVS_IRQ_NUM_MASK) << CAVS_IRQ_NUM_SHIFT) \
	)

#define CAVS_L2_AGG_INT_LEVEL2			DT_IRQN(DT_INST(0, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL3			DT_IRQN(DT_INST(1, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL4			DT_IRQN(DT_INST(2, intel_cavs_intc))
#define CAVS_L2_AGG_INT_LEVEL5			DT_IRQN(DT_INST(3, intel_cavs_intc))

#define CAVS_ICTL_INT_CPU_OFFSET(x)		(0x40 * x)

#define IOAPIC_EDGE				0
#define IOAPIC_HIGH				0

/* I2S */
#define I2S_CAVS_IRQ(i2s_num)			\
	SOC_AGGREGATE_IRQ(0, (i2s_num), CAVS_L2_AGG_INT_LEVEL5)

#define I2S0_CAVS_IRQ				I2S_CAVS_IRQ(0)
#define I2S1_CAVS_IRQ				I2S_CAVS_IRQ(1)
#define I2S2_CAVS_IRQ				I2S_CAVS_IRQ(2)
#define I2S3_CAVS_IRQ				I2S_CAVS_IRQ(3)

#define SSP_MN_DIV_SIZE				(8)
#define SSP_MN_DIV_BASE(x)			\
	(0x00078D00 + ((x) * SSP_MN_DIV_SIZE))

#define PDM_BASE				DMIC_BASE

/* DSP Wall Clock Timers (0 and 1) */
#define DSP_WCT_IRQ(x) \
	SOC_AGGREGATE_IRQ((22 + x), CAVS_L2_AGG_INT_LEVEL2)

#define DSP_WCT_CS_TA(x)			BIT(x)
#define DSP_WCT_CS_TT(x)			BIT(4 + x)

/* Attribute macros to place code and data into IMR memory */
#define __imr __in_section_unique(imr)
#define __imrdata __in_section_unique(imrdata)

extern void z_soc_irq_enable(uint32_t irq);
extern void z_soc_irq_disable(uint32_t irq);
extern int z_soc_irq_is_enabled(unsigned int irq);

/* Legacy SOC-level API still used in a few drivers */
#define SOC_DCACHE_FLUSH(addr, size)		\
	z_xtensa_cache_flush((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	z_xtensa_cache_inv((addr), (size))

/**
 * @brief Return uncached pointer to a RAM address
 *
 * The Intel ADSP architecture maps all addressable RAM (of all types)
 * twice, in two different 512MB segments regions whose L1 cache
 * settings can be controlled independently.  So for any given
 * pointer, it is possible to convert it to and from a cached version.
 *
 * This function takes a pointer to any addressible object (either in
 * cacheable memory or not) and returns a pointer that can be used to
 * refer to the same memory while bypassing the L1 data cache.  Data
 * in the L1 cache will not be inspected nor modified by the access.
 *
 * @see z_soc_cached_ptr()
 *
 * @param p A pointer to a valid C object
 * @return A pointer to the same object bypassing the L1 dcache
 */
static inline void *z_soc_uncached_ptr(void *p)
{
	return ((void *)(((size_t)p) & ~0x20000000));
}

/**
 * @brief Return cached pointer to a RAM address
 *
 * This function takes a pointer to any addressible object (either in
 * cacheable memory or not) and returns a pointer that can be used to
 * refer to the same memory through the L1 data cache.  Data read
 * through the resulting pointer will reflect locally cached values on
 * the current CPU if they exist, and writes will go first into the
 * cache and be written back later.
 *
 * @see z_soc_uncached_ptr()
 *
 * @param p A pointer to a valid C object
 * @return A pointer to the same object via the L1 dcache

 */
static inline void *z_soc_cached_ptr(void *p)
{
	return ((void *)(((size_t)p) | 0x20000000));
}

#endif /* __INC_SOC_H */
