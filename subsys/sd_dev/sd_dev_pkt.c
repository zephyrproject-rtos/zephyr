/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SD Device Packet Management
 *
 * This file implements packet allocation, deallocation, and reference
 * counting for SD device TX/RX operations. It manages memory pools for
 * packet structures and data buffers with DMA alignment requirements.
 */

#include <zephyr/sd_dev/sd_dev_pkt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sd_dev, LOG_LEVEL_INF);

/**
 * @name Memory Pool Definitions
 * @{
 */

/**
 * @brief Calculate packet object size with DMA alignment
 *
 * Rounds up the packet structure size to meet DMA alignment requirements.
 */
#define SDEV_PKT_OBJ_SIZE ROUND_UP(sizeof(sd_dev_pkt_t), CONFIG_SDEV_DMA_ALIGN)

/**
 * @brief TX packet structure memory pool
 *
 * Memory slab for allocating TX packet structures with DMA alignment.
 */
K_MEM_SLAB_DEFINE(sd_dev_tx_pkt_pool, SDEV_PKT_OBJ_SIZE, SDEV_PKT_POOL_SIZE, SDEV_DMA_ALIGN);

/**
 * @brief RX packet structure memory pool
 *
 * Memory slab for allocating RX packet structures with DMA alignment.
 */
K_MEM_SLAB_DEFINE(sd_dev_rx_pkt_pool, SDEV_PKT_OBJ_SIZE, SDEV_PKT_POOL_SIZE, SDEV_DMA_ALIGN);

/**
 * @brief RX data buffer memory pool
 *
 * Memory slab for allocating RX data buffers with DMA alignment.
 * TX buffers are managed by the upper layer.
 */
K_MEM_SLAB_DEFINE(sd_dev_rx_data_pool, SDEV_PKT_BUF_SIZE, SDEV_PKT_POOL_SIZE, SDEV_DMA_ALIGN);

/**
 * @}
 */

/**
 * @name Packet Management APIs
 * @{
 */

/**
 * @brief Allocate an SD device packet
 *
 * This function allocates a packet structure and associated data buffer
 * (for RX only) from the appropriate memory pools. The packet is initialized
 * with a reference count of 1.
 *
 * For RX packets:
 * - Allocates both packet structure and data buffer
 * - Data buffer is DMA-aligned
 *
 * For TX packets:
 * - Allocates only packet structure
 * - Data buffer pointer is NULL (managed by upper layer)
 *
 * @param dir Packet direction (SDEV_PKT_RX or SDEV_PKT_TX)
 *
 * @retval Pointer to allocated packet on success
 * @retval NULL if allocation fails or invalid direction
 */
sd_dev_pkt_t *sd_dev_pkt_alloc(sd_dev_pkt_dir_t dir)
{
	sd_dev_pkt_t *pkt = NULL;
	uint8_t *buf = NULL;
	uint32_t cycle = k_cycle_get_32();
	uint32_t time_us_start = k_cyc_to_us_near32(cycle);
	uint32_t time_us_end, time_dis;

	/* Validate direction */
	if (dir != SDEV_PKT_RX && dir != SDEV_PKT_TX) {
		return NULL;
	}

	if (dir == SDEV_PKT_RX) {
		/* Allocate packet structure */
		if (k_mem_slab_alloc(&sd_dev_rx_pkt_pool, (void **)&pkt, K_NO_WAIT) != 0) {
			return NULL;
		}

		/* Allocate RX data buffer */
		if (k_mem_slab_alloc(&sd_dev_rx_data_pool, (void **)&buf, K_NO_WAIT) != 0) {
			k_mem_slab_free(&sd_dev_rx_pkt_pool, pkt);
			return NULL;
		}

		pkt->data = buf;
	} else { /* TX packet */
		/* Allocate packet structure */
		if (k_mem_slab_alloc(&sd_dev_tx_pkt_pool, (void **)&pkt, K_NO_WAIT) != 0) {
			return NULL;
		}

		/* TX buffer is managed by upper layer */
		pkt->data = NULL;
	}

	pkt->dir = dir;

	/* Initialize reference count to 1 */
	atomic_set(&pkt->ref, 1);

	cycle = k_cycle_get_32();
	time_us_end = k_cyc_to_us_near32(cycle);
	time_dis = time_us_end - time_us_start;

	return pkt;
}

/**
 * @brief Free an SD device packet
 *
 * This function decrements the packet's reference count and frees the
 * packet resources when the count reaches zero. Uses atomic operations
 * to ensure thread-safety.
 *
 * For RX packets:
 * - Frees both data buffer and packet structure
 *
 * For TX packets:
 * - Frees only packet structure (data managed by upper layer)
 *
 * @param pkt Pointer to packet to free (NULL is safe)
 *
 * @note This function is safe to call from multiple contexts due to
 *       atomic reference counting.
 */
void sd_dev_pkt_free(sd_dev_pkt_t *pkt)
{
	if (!pkt) {
		return;
	}

	/*
	 * atomic_dec() returns the value BEFORE decrement.
	 * If it returns 1, this is the last reference.
	 */
	if (atomic_dec(&pkt->ref) == 1) {
		if (pkt->dir == SDEV_PKT_RX) {
			if (pkt->data) {
				k_mem_slab_free(&sd_dev_rx_data_pool, pkt->data);
			}
			k_mem_slab_free(&sd_dev_rx_pkt_pool, pkt);
		} else if (pkt->dir == SDEV_PKT_TX) {
			k_mem_slab_free(&sd_dev_tx_pkt_pool, pkt);
		} else {
			LOG_ERR("Unknown packet direction: %d", pkt->dir);
			k_mem_slab_free(&sd_dev_rx_pkt_pool, pkt);
		}
	}
}

/**
 * @brief Clone an SD device packet (increment reference count)
 *
 * This function creates a shallow copy of the packet by incrementing
 * its reference count. The packet data is shared between all clones.
 * Each clone must be freed separately with sd_dev_pkt_free().
 *
 * @param pkt Pointer to packet to clone
 *
 * @retval Pointer to the same packet (with incremented ref count)
 * @retval NULL if pkt is NULL
 *
 * @note The returned pointer is the same as the input pointer.
 *       This function does not allocate new memory.
 * @note Reference counter overflow is checked with assertion.
 */
sd_dev_pkt_t *sd_dev_pkt_get(sd_dev_pkt_t *pkt)
{
	if (!pkt) {
		return NULL;
	}

	/* Optional: protect against overflow */
	__ASSERT(atomic_get(&pkt->ref) < INT32_MAX, "Reference counter overflow");

	atomic_inc(&pkt->ref);

	return pkt;
}

/**
 * @}
 */
