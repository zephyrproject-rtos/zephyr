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
#include "fsl_soc_src.h"

/*
 * GPU (VG-Lite) resource definitions for RT1170.
 * Values are derived from NXP SDK_25_09 (MIMXRT1170-EVKB).
 */
#define CLOCK_ID_GPU         kCLOCK_Gpu2d
#define IRQ_ID_GPU           GPU2D_IRQn
#define GPU_REG_BASE         0x41800000u
#define RESET_ID_GPU         kSRC_DisplaySlice
#define POWERGATE_GPU_PG_DEV kSSARC_DISPLAYMIXPowerDomain
#define SPI1_GPU_CTL         0U /* Placeholder */

/* FlexSPI1 address range */
#define FLEXSPI1_BASE_ADDR 0x30000000U
#define FLEXSPI1_END_ADDR  0x30FFFFFFU /* 16 MB */

/*
 * The SDRAM region on RT1170 spans 64 MB starting at address 0x80000000
 * (0x8000_0000 – 0x83FF_FFFF) and is treated as a single contiguous
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

#define CACHEABLE_START   CONFIG_VGLITE_CACHEABLE_MEM_START
#define CACHEABLE_END     (CONFIG_VGLITE_CACHEABLE_MEM_START + CONFIG_VGLITE_CACHEABLE_MEM_SIZE - 1U)
#define SPI1_UNCACHE_ADDR CONFIG_VGLITE_UNCACHE_ADDR


/*
 *--------------------------------------------------------------------
 * SPI cache operation codes (RT1170 mapping)
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

/* PSRAM not used on this platform */
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
	const clock_root_config_t gc355ClockConfig = {
		.clockOff = false,
		.mux = kCLOCK_GC355_ClockRoot_MuxVideoPllOut,
		.div = 2,
	};

	CLOCK_SetRootClock(kCLOCK_Root_Gc355, &gc355ClockConfig);
	CLOCK_GetRootClockFreq(kCLOCK_Root_Gc355);
}

/* Clock control */
static inline void acts_clock_peripheral_enable(uint32_t clk_id)
{
	CLOCK_EnableClock(clk_id);
}

static inline void acts_clock_peripheral_disable(uint32_t clk_id)
{
	CLOCK_DisableClock(clk_id);
}

/* Reset control */
static inline void acts_reset_peripheral_assert(uint32_t reset_id)
{
	(void)reset_id;
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
 * Map SPI cache ops to global Cortex-M7 D-Cache maintenance on RT1170
 * ------------------------------------------------------------------
 */
void spi1_cache_ops(spi_cache_op_t op, void *addr, size_t size)
{
	(void)addr; /* Not used for global ops */
	(void)size;

	switch (op) {
	case SPI_CACHE_FLUSH_ALL:
		SCB_CleanDCache();
		break;

	case SPI_CACHE_FLUSH_INVALID_ALL:
		SCB_CleanInvalidateDCache();
		break;

	case SPI_WRITEBUF_FLUSH:
		SCB_CleanDCache();
		break;

	default:
		/* Unsupported op on RT1170 */
		break;
	}
}

#endif /* CONFIG_VGLITE */
#endif /* _nxp_gpu_interface_h_ */
