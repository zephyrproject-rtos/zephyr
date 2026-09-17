/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cache maintenance for xHCI DMA-visible buffers (shared by ring/bulk layers).
 */

#ifndef ZEPHYR_USB_XHCI_DMA_H
#define ZEPHYR_USB_XHCI_DMA_H

#include <stddef.h>
#include <stdint.h>

#include <zephyr/cache.h>

static inline size_t dwc3_dma_cache_line_size(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	size_t line = sys_cache_data_line_size_get();

	return (line != 0U) ? line : 64U;
#else
	return 1U;
#endif
}

static inline void dwc3_dma_flush(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	if (addr != NULL && size > 0U) {
		(void)sys_cache_data_flush_range(addr, size);
	}
#endif
}

static inline void dwc3_dma_invalidate(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	if (addr != NULL && size > 0U) {
		(void)sys_cache_data_invd_range(addr, size);
	}
#endif
}

/*
 * xhci_flush_cache / xhci_inval_cache: expand [addr, addr+size) to full
 * cache lines so partial-line DMA does not leave stale or clobber adjacent fields.
 */
static inline void dwc3_dma_flush_aligned(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	uintptr_t start;
	uintptr_t end;
	size_t line;

	if (addr == NULL || size == 0U) {
		return;
	}

	line = dwc3_dma_cache_line_size();
	start = (uintptr_t)addr & ~(line - 1U);
	end = ((uintptr_t)addr + size + line - 1U) & ~(line - 1U);
	(void)sys_cache_data_flush_range((void *)start, end - start);
#endif
}

static inline void dwc3_dma_invalidate_aligned(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	uintptr_t start;
	uintptr_t end;
	size_t line;

	if (addr == NULL || size == 0U) {
		return;
	}

	line = dwc3_dma_cache_line_size();
	start = (uintptr_t)addr & ~(line - 1U);
	end = ((uintptr_t)addr + size + line - 1U) & ~(line - 1U);
	(void)sys_cache_data_invd_range((void *)start, end - start);
#endif
}

static inline void dwc3_dma_prep_rx_aligned(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	if (addr != NULL && size > 0U) {
		(void)sys_cache_data_flush_and_invd_range(
			(void *)((uintptr_t)addr & ~(dwc3_dma_cache_line_size() - 1U)),
			(((uintptr_t)addr + size + dwc3_dma_cache_line_size() - 1U) &
			 ~(dwc3_dma_cache_line_size() - 1U)) -
				((uintptr_t)addr & ~(dwc3_dma_cache_line_size() - 1U)));
	}
#endif
}

#endif /* ZEPHYR_USB_XHCI_DMA_H */
