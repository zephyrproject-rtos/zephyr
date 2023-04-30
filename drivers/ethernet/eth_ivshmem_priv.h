/*
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_IVSHMEM_PRIV_H
#define ETH_IVSHMEM_PRIV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <openamp/virtio_ring.h>

struct eth_ivshmem_queue {
	struct {
		struct vring vring;
		void *shmem;
		uint16_t desc_head;
		uint16_t desc_len;
		uint32_t data_head;
		uint32_t data_tail;
		uint32_t data_len;
		uint16_t avail_idx;
		uint16_t used_idx;

		uint32_t pending_data_head;
		uint32_t pending_data_len;
	} tx;
	struct {
		struct vring vring;
		void *shmem;
		uint16_t avail_idx;
		uint16_t used_idx;
	} rx;
	uint16_t desc_max_len;
	uint32_t vring_header_size;
	uint32_t vring_data_max_len;
};

int eth_ivshmem_queue_init(
		struct eth_ivshmem_queue *q, uintptr_t shmem,
		size_t shmem_section_size, bool tx_buffer_first);
void eth_ivshmem_queue_reset(struct eth_ivshmem_queue *q);
int eth_ivshmem_queue_tx_get_buff(struct eth_ivshmem_queue *q, void **data, size_t len);
int eth_ivshmem_queue_tx_commit_buff(struct eth_ivshmem_queue *q);
int eth_ivshmem_queue_rx(struct eth_ivshmem_queue *q, const void **data, size_t *len);
int eth_ivshmem_queue_rx_complete(struct eth_ivshmem_queue *q);

#endif /* ETH_IVSHMEM_PRIV_H */
