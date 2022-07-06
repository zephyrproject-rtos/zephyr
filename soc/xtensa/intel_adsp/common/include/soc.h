/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_COMMON_SOC_H_
#define ZEPHYR_SOC_INTEL_ADSP_COMMON_SOC_H_

#include <string.h>
#include <errno.h>
#include <zephyr/arch/xtensa/cache.h>

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

extern char _text_start[];
extern char _text_end[];
extern char _imr_start[];
extern char _imr_end[];
extern char _end[];
extern char _heap_sentry[];
extern char _cached_start[];
extern char _cached_end[];

extern void soc_trace_init(void);
extern void z_soc_irq_init(void);
extern void z_soc_irq_enable(uint32_t irq);
extern void z_soc_irq_disable(uint32_t irq);
extern int z_soc_irq_is_enabled(unsigned int irq);

extern void z_soc_mp_asm_entry(void);
extern void soc_mp_startup(uint32_t cpu);
extern void soc_start_core(int cpu_num);

extern bool soc_cpus_active[CONFIG_MP_NUM_CPUS];

/* Legacy cache APIs still used in a few places */
#define SOC_DCACHE_FLUSH(addr, size)		\
	z_xtensa_cache_flush((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	z_xtensa_cache_inv((addr), (size))
#define z_soc_cached_ptr(p) arch_xtensa_cached_ptr(p)
#define z_soc_uncached_ptr(p) arch_xtensa_uncached_ptr(p)

/**
 * @brief Halts and offlines a running CPU
 *
 * Enables power gating on the specified CPU, which cannot be the
 * current CPU or CPU 0.  The CPU must be idle; no application threads
 * may be runnable on it when this function is called (or at least the
 * CPU must be guaranteed to reach idle in finite time without
 * deadlock).  Actual CPU shutdown can only happen in the context of
 * the idle thread, and synchronization is an application
 * responsibility.  This function will hang if the other CPU fails to
 * reach idle.
 *
 * @note On older cAVS hardware, core power is controlled by the host.
 * This function must still be called for OS bookkeeping, but it is
 * insufficient without application coordination (and careful
 * synchronization!) with the host x86 environment.
 *
 * @param id CPU to halt, not current cpu or cpu 0
 * @return 0 on success, -EINVAL on error
 */
int soc_adsp_halt_cpu(int id);

static inline bool intel_adsp_ptr_executable(const void *p)
{
	return (p >= (void *)_text_start && p <= (void *)_text_end) ||
		(p >= (void *)_imr_start && p <= (void *)_imr_end);
}

static inline bool intel_adsp_ptr_is_sane(uint32_t sp)
{
	return ((char *)sp >= _end && (char *)sp <= _heap_sentry) ||
		((char *)sp >= _cached_start && (char *)sp <= _cached_end) ||
		(sp >= (CONFIG_IMR_MANIFEST_ADDR - CONFIG_ISR_STACK_SIZE)
		 && sp <= CONFIG_IMR_MANIFEST_ADDR);
}

static ALWAYS_INLINE void z_idelay(int n)
{
	while (n--) {
		__asm__ volatile("nop");
	}
}

/* memcopy used by boot loader */
static ALWAYS_INLINE void bmemcpy(void *dest, void *src, size_t bytes)
{
	uint32_t *d = (uint32_t *)dest;
	uint32_t *s = (uint32_t *)src;

	z_xtensa_cache_inv(src, bytes);
	for (size_t i = 0; i < (bytes >> 2); i++)
		d[i] = s[i];

	z_xtensa_cache_flush(dest, bytes);
}

/* bzero used by bootloader */
static ALWAYS_INLINE void bbzero(void *dest, size_t bytes)
{
	uint32_t *d = (uint32_t *)dest;

	for (size_t i = 0; i < (bytes >> 2); i++)
		d[i] = 0;

	z_xtensa_cache_flush(dest, bytes);
}


#endif /* ZEPHYR_SOC_INTEL_ADSP_COMMON_SOC_H_ */
