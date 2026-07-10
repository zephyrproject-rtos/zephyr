/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SD Device Packet Management
 *
 * This file provides packet allocation, management, and reference counting
 * APIs for SD device data transfer operations.
 */

#ifndef SDEV_PKT_H
#define SDEV_PKT_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>

/**
 * @brief SD Device Packet APIs
 * @defgroup sd_dev_pkt SD Device Packet APIs
 * @ingroup sd_dev_interface
 * @{
 */

/** Size of the packet pool */
#define SDEV_PKT_POOL_SIZE CONFIG_SDEV_PKT_POOL_SIZE

/** Size of each packet buffer in bytes */
#define SDEV_PKT_BUF_SIZE CONFIG_SDEV_PKT_BUF_SIZE

/** DMA alignment requirement in bytes */
#define SDEV_DMA_ALIGN CONFIG_SDEV_DMA_ALIGN

/**
 * @brief SD device packet direction
 *
 * Defines the direction of data transfer for a packet.
 */
typedef enum {
	/** Transmit packet (device to host) */
	SDEV_PKT_TX = 0,
	/** Receive packet (host to device) */
	SDEV_PKT_RX,
} sd_dev_pkt_dir_t;

/**
 * @brief SD device packet structure
 *
 * This structure represents a packet wrapper for SD device data transfer.
 * The actual data buffer is managed externally and referenced by the
 * data pointer. The structure is aligned to DMA requirements.
 */
typedef struct __aligned(SDEV_DMA_ALIGN)
{
	/** Node for use in linked lists (FIFO, etc.) */
	sys_snode_t node;

	/** Pointer to the data buffer */
	uint8_t *data;

	/** Valid data length in bytes */
	uint16_t len;

	/** Packet direction (TX or RX) */
	uint8_t dir;

	/** Reference counter for packet sharing */
	atomic_t ref;

#ifdef CONFIG_SDIO_DEV
	/** SDIO function number (1-7) */
	uint8_t fn;
#endif
}
sd_dev_pkt_t;

/**
 * @brief Allocate a new packet
 *
 * This function allocates a packet from the packet pool and assigns
 * it a data buffer. The packet reference count is initialized to 1.
 *
 * @param dir Packet direction (SDEV_PKT_TX or SDEV_PKT_RX)
 *
 * @return Pointer to allocated packet on success, NULL if pool is empty
 */
sd_dev_pkt_t *sd_dev_pkt_alloc(sd_dev_pkt_dir_t dir);

/**
 * @brief Free a packet
 *
 * This function decrements the packet reference count and returns it
 * to the pool when the count reaches zero.
 *
 * @param pkt Pointer to packet to free
 */
void sd_dev_pkt_free(sd_dev_pkt_t *pkt);

/**
 * @brief get a packet (increment reference count)
 *
 * This function increments the reference count of a packet, allowing
 * multiple users to share the same packet.
 *
 * @param pkt Pointer to packet to get
 *
 * @return Pointer to the same packet with incremented reference count
 */
sd_dev_pkt_t *sd_dev_pkt_get(sd_dev_pkt_t *pkt);

/**
 * @}
 */

#endif /* SDEV_PKT_H */
