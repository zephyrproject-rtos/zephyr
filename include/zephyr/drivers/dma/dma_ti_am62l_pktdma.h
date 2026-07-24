/**
 * TI AM62L PKTDMA driver API
 *
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Author: Siddharth Vadapalli <s-vadapalli@ti.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_AM62L_PKTDMA_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_AM62L_PKTDMA_H_

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
 *
 * The PKTDMA driver is a standard Zephyr DMA driver and is configured via
 * the dma_config() API. However, since the standard struct dma_config does not
 * have fields for ring buffer addresses or the ASEL value which are PKTDMA
 * hardware specific, these parameters are carried in this structure,
 * pointed to by dma_config.user_data.
 *
 * Channel direction is conveyed by dma_config.channel_direction:
 *   - MEMORY_TO_PERIPHERAL : TX channel
 *   - PERIPHERAL_TO_MEMORY : RX channel
 *
 * New work is submitted per-packet by calling dma_reload():
 *   - TX: dma_reload(dev, chan, src=(uintptr_t)desc, 0, 0)
 *   - RX: dma_reload(dev, chan, 0, dst=(uintptr_t)desc, 0)
 *
 * The dma_callback registered in dma_config is called once per completed
 * descriptor from ISR context.  The interrupt is disabled before the callback
 * fires. Consumer should call dma_resume() after draining all completions.
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
	/** Consumer's user data associated with DMA Callback*/
	const void *cb_user_data;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_AM62L_PKTDMA_H_ */
