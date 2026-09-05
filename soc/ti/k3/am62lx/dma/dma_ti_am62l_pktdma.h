/*
 * TI AM62L PKTDMA driver API
 *
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Author: Siddharth Vadapalli <s-vadapalli@ti.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_TI_AM62L_PKTDMA_H_
#define DMA_TI_AM62L_PKTDMA_H_

#include <zephyr/drivers/dma.h>
#include <zephyr/sys/util.h>

/**
 * @brief The 4 bits corresponding to ASEL in a 64-bit address
 */
#define AM62L_ADDRESS_ASEL_MASK    GENMASK64(51, 48)

/**
 * @brief ASEL values 14 and 15 route DMA transactions through the ARM Accelerator
 * Coherency Port (ACP).  ACP transactions are fully coherent with the CPU cache,
 * so no explicit cache flush or invalidate is required for DMA buffers and
 * descriptors on coherent channels.
 */
#define AM62L_PKTDMA_IS_COHERENT(asel)  ((asel) == 14U || (asel) == 15U)

/**
 * @brief PKTDMA channel configuration passed via dma_config.user_data.
 */
struct ti_pktdma_chan_cfg {
	/** Virtual address of the forward ring buffer */
	uintptr_t fwd_ring_mem;

	/** Virtual address of the reverse/completion ring buffer */
	uintptr_t rev_ring_mem;

	/** Number of 8-byte entries in each ring */
	uint32_t ring_cnt;

	/**
	 * Address Space Select (ASEL) value (defaults to zero)
	 * 0  = non-coherent (cache flush/invalidate required)
	 * 14 or 15 = coherent (cache operations not required)
	 */
	uint8_t asel;

	/** Consumer's user data associated with DMA Callback */
	const void *cb_user_data;
};

#endif /* DMA_TI_AM62L_PKTDMA_H_ */
