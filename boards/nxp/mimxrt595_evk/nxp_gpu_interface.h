/*
 * Copyright 2019, 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _nxp_gpu_interface_h_
#define _nxp_gpu_interface_h_

#ifdef CONFIG_VGLITE /* Process only if VGLite is enabled */

#include "fsl_clock.h"

#ifdef CPU_MIMXRT1176_cm7
#include "fsl_soc_src.h"
#elif CPU_MIMXRT595SFFOC_cm33
#include "fsl_power.h"
#endif

/*
 * GPU (VG-Lite) resource definitions for RT595.
 * Values are derived from NXP SDK_25_09 (MIMXRT595).
 */
#define CLOCK_ID_GPU         kCLOCK_Gpu
#define IRQ_ID_GPU           GPU_IRQn
#define GPU_REG_BASE         0x40240000u
#define RESET_ID_GPU         kGPU_RST_SHIFT_RSTn
#define POWERGATE_GPU_PG_DEV 1U /* Placeholder */
#define SPI1_GPU_CTL         0U /* Placeholder */

/* FlexSPI1 address range */
#define FLEXSPI1_BASE_ADDR 0x18000000U
#define FLEXSPI1_END_ADDR  0x1c000000U /* 64 MB */

/*
 * The SDRAM region on RT595 spans 4.5 MB starting at address 0x20080000
 * (0x2008_0000 – 0x2050_0000) and is treated as a single contiguous
 * memory block.
 *
 * No separate aliases are defined for different cacheability attributes,
 * so cacheability conversions (cacheable ↔ non-cacheable ↔ WT/NA) resolve
 * to the same physical address.
 *
 * The helper functions below provide a consistent abstraction for these
 * conversions, ensuring portability across platforms where different
 * cacheability mappings may exist.
 */

/* New SRAM base and size for RT595: 0x20080000 length 0x480000 (4608 KB) */
#define CACHEABLE_START   CONFIG_VGLITE_CACHEABLE_MEM_START
#define CACHEABLE_END     (CONFIG_VGLITE_CACHEABLE_MEM_START + CONFIG_VGLITE_CACHEABLE_MEM_SIZE - 1U)
#define SPI1_UNCACHE_ADDR CONFIG_VGLITE_UNCACHE_ADDR
/*
 *--------------------------------------------------------------------
 * SPI cache operation codes (RT595 mapping)
 * --------------------------------------------------------------------
 */

typedef enum {
	SPI_CACHE_FLUSH_ALL = 1,         /* Clean entire D-Cache */
	SPI_CACHE_FLUSH_INVALID_ALL = 2, /* Clean + Invalidate entire D-Cache */
	SPI_WRITEBUF_FLUSH = 3           /* Clean buffer/D-Cache (optional case) */
} spi_cache_op_t;

/* Check if address lies within a range */
static inline int in_range(uintptr_t a, uintptr_t lo, uintptr_t hi)
{
	return (a >= lo) && (a <= hi);
}

/* Check if address is in NOR (FlexSPI) region */
static inline int buf_is_nor(void *addr)
{
	uintptr_t a = (uintptr_t)addr;

	return in_range(a, FLEXSPI1_BASE_ADDR, FLEXSPI1_END_ADDR);
}

/* No uncached alias defined for FlexSPI */
static inline int buf_is_nor_un(void *addr)
{
	(void)addr;
	return 0;
}

/* GPU power gating not supported separately */
void soc_powergate_set(uint32_t powergate, bool enable)
{
	(void)powergate;
	(void)enable;
}

bool buf_is_psram_cache(void *addr)
{
	(void)addr;
	return false;
}

bool buf_is_psram(void *addr)
{
	(void)addr;
	return false;
}

/* GPU clock handled by DisplayMix clocks */
static inline void clk_set_rate(uint32_t clk_id, uint32_t rate_hz)
{
	SYSCTL0->PDRUNCFG1_CLR = SYSCTL0_PDRUNCFG1_GPU_SRAM_APD_MASK;
	SYSCTL0->PDRUNCFG1_CLR = SYSCTL0_PDRUNCFG1_GPU_SRAM_PPD_MASK;
	POWER_ApplyPD();

	CLOCK_AttachClk(kMAIN_CLK_to_GPU_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivGpuClk, 2);
}

/* Clock control */
static inline void acts_clock_peripheral_enable(uint32_t clk_id)
{
	CLOCK_EnableClock(clk_id);
	CLOCK_EnableClock(kCLOCK_AxiSwitch);
}

static inline void acts_clock_peripheral_disable(uint32_t clk_id)
{
	CLOCK_DisableClock(clk_id);
	CLOCK_DisableClock(kCLOCK_AxiSwitch);
}

/* Reset control */
static inline void acts_reset_peripheral_assert(uint32_t reset_id)
{
	(void)reset_id;
	RESET_ClearPeripheralReset(kGPU_RST_SHIFT_RSTn);
	RESET_ClearPeripheralReset(kAXI_SWITCH_RST_SHIFT_RSTn);
}

static inline void acts_reset_peripheral_deassert(uint32_t reset_id)
{
	(void)reset_id;
}

/*
 * Cacheable → WT/NA cache (for GPU/CPU DMA)
 */
static inline void *cache_to_wt_wna_cache(void *addr)
{
	uintptr_t a = (uintptr_t)addr;

	if (a >= CACHEABLE_START && a <= CACHEABLE_END) {
		/* Already cacheable WT/NA region */
		return addr;
	}

	/* Outside cacheable range, map to WT/NA if needed */
	return NULL; /* or add logic if you have alias for WT/NA */
}

/*
 * WT/NA cache → normal cache
 */
static inline void *wt_wna_cache_to_cache(void *addr)
{
	uintptr_t a = (uintptr_t)addr;

	if (a >= CACHEABLE_START && a <= CACHEABLE_END) {
		return (void *)a; /* already cacheable */
	}

	return NULL; /* outside range */
}

/*
 * Cacheable → Non-cacheable
 */
static inline void *cache_to_uncache(void *addr)
{
	uintptr_t a = (uintptr_t)addr;
	/*
	 * Since uncacheable region is not defined,
	 * cache to uncache function returns cache address only
	 */
	if (a >= CACHEABLE_START && a <= CACHEABLE_END) {
		return (void *)a; /* No offset */
	}

	return NULL; /* already uncached or outside RAM */
}

/*
 * Non-cacheable → WT/NA cache
 */
static inline void *uncache_to_wt_wna_cache(void *addr)
{
	uintptr_t a = (uintptr_t)addr;

	return (void *)a; /* No offset */
}

/*
 * --------------------------------------------------------------------
 * Map SPI cache ops to global Cortex-M33 Cache maintenance on RT595
 * ------------------------------------------------------------------
 */
void spi1_cache_ops(spi_cache_op_t op, void *addr, size_t size)
{
	(void)addr; /* Not used for global ops */
	(void)size;

	switch (op) {
	case SPI_CACHE_FLUSH_ALL:
		CACHE64_CleanCache(CACHE64_CTRL0);
		break;

	case SPI_CACHE_FLUSH_INVALID_ALL:
		CACHE64_CleanInvalidateCache(CACHE64_CTRL0);
		break;

	case SPI_WRITEBUF_FLUSH:
		CACHE64_CleanCache(CACHE64_CTRL0);
		break;

	default:
		/* Unsupported op on RT595 */
		break;
	}
}

#endif /* CONFIG_VGLITE */
#endif /* _nxp_gpu_interface_h_ */
