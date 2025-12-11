/*
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eth_ivshmem_priv.h"

#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>

#include <stdatomic.h>
#include <string.h>

/* These defines must match on the peer */
#define ETH_IVSHMEM_VRING_ALIGNMENT 64
#define ETH_IVSHMEM_FRAME_SIZE(len) ROUND_UP(18 + (len), L1_CACHE_BYTES)

#define VRING_FLUSH(x)		sys_cache_data_flush_range(&(x), sizeof(x))
#define VRING_INVALIDATE(x)	sys_cache_data_invd_range(&(x), sizeof(x))

static int calc_vring_size(
		size_t section_size, uint16_t *vring_desc_len,
		uint32_t *vring_header_size);
static uint32_t tx_buffer_advance(uint32_t max_len, uint32_t *position, uint32_t *len);
static int tx_clean_used(struct eth_ivshmem_queue *q);
static int get_rx_avail_desc_idx(struct eth_ivshmem_queue *q, uint16_t *avail_desc_idx);

int eth_ivshmem_queue_init(
		struct eth_ivshmem_queue *q, uintptr_t tx_shmem,
		uintptr_t rx_shmem, size_t shmem_section_size)
{
	memset(q, 0, sizeof(*q));

	uint16_t vring_desc_len;
	uint32_t vring_header_size;
	int res = calc_vring_size(shmem_section_size, &vring_desc_len, &vring_header_size);

	if (res != 0) {
		return res;
	}

	q->desc_max_len = vring_desc_len;
	q->vring_data_max_len = shmem_section_size - vring_header_size;
	q->vring_header_size = vring_header_size;
	q->tx.shmem = (void *)tx_shmem;
	q->rx.shmem = (void *)rx_shmem;

	/* Init vrings */
	vring_init(&q->tx.vring, vring_desc_len, q->tx.shmem, ETH_IVSHMEM_VRING_ALIGNMENT);
	vring_init(&q->rx.vring, vring_desc_len, q->rx.shmem, ETH_IVSHMEM_VRING_ALIGNMENT);

	/* Swap "used" pointers.
	 * This is done so that each peer only ever writes to its output section,
	 * while maintaining vring code consistency elsewhere in this file.
	 */
	struct vring_used *tmp_used = q->tx.vring.used;

	q->tx.vring.used = q->rx.vring.used;
	q->rx.vring.used = tmp_used;

	eth_ivshmem_queue_reset(q);

	return 0;
}

void eth_ivshmem_queue_reset(struct eth_ivshmem_queue *q)
{
	q->tx.desc_head = 0;
	q->tx.desc_len = 0;
	q->tx.data_head = 0;
	q->tx.data_tail = 0;
	q->tx.data_len = 0;
	q->tx.avail_idx = 0;
	q->tx.used_idx = 0;
	q->tx.pending_data_head = 0;
	q->tx.pending_data_len = 0;
	q->rx.avail_idx = 0;
	q->rx.used_idx = 0;

	memset(q->tx.shmem, 0, q->vring_header_size);

	/* Init TX ring descriptors */
	for (unsigned int i = 0; i < q->tx.vring.num - 1; i++) {
		q->tx.vring.desc[i].next = i + 1;
	}
	q->tx.vring.desc[q->tx.vring.num - 1].next = 0;
}

int eth_ivshmem_queue_tx_get_buff(struct eth_ivshmem_queue *q, void **data, size_t len)
{
	/* Clean used TX buffers */
	int res = tx_clean_used(q);

	if (res != 0) {
		return res;
	}

	if (q->tx.desc_len >= q->desc_max_len) {
		return -ENOBUFS;
	}

	uint32_t head = q->tx.data_head;
	uint32_t consumed_len = len;
	uint32_t new_head = tx_buffer_advance(q->vring_data_max_len, &head, &consumed_len);

	if (q->vring_data_max_len - q->tx.data_len < consumed_len) {
		return -ENOBUFS;
	}

	struct vring_desc *tx_desc = &q->tx.vring.desc[q->tx.desc_head];

	tx_desc->addr = q->vring_header_size + head;
	tx_desc->len = len;
	tx_desc->flags = 0;
	VRING_FLUSH(*tx_desc);

	*data = (uint8_t *)q->tx.shmem + q->vring_header_size + head;

	q->tx.pending_data_head = new_head;
	q->tx.pending_data_len = q->tx.data_len + consumed_len;

	return 0;
}

int eth_ivshmem_queue_tx_commit_buff(struct eth_ivshmem_queue *q)
{
	/* Ensure that a TX buffer is pending */
	if (q->tx.pending_data_len == 0) {
		return -EINVAL;
	}

	uint16_t desc_head = q->tx.desc_head;

	q->tx.desc_len++;
	q->tx.desc_head = (q->tx.desc_head + 1) % q->desc_max_len;

	q->tx.data_head = q->tx.pending_data_head;
	q->tx.data_len = q->tx.pending_data_len;

	q->tx.vring.avail->ring[q->tx.avail_idx % q->desc_max_len] = desc_head;

	VRING_FLUSH(q->tx.vring.avail->ring[q->tx.avail_idx % q->desc_max_len]);
	atomic_thread_fence(memory_order_seq_cst);

	q->tx.avail_idx++;
	q->tx.vring.avail->idx = q->tx.avail_idx;

	VRING_FLUSH(q->tx.vring.avail->idx);

	q->tx.pending_data_len = 0;

	return 0;
}

int eth_ivshmem_queue_rx(struct eth_ivshmem_queue *q, const void **data, size_t *len)
{
	*data = NULL;
	*len = 0;

	uint16_t avail_desc_idx;
	int res = get_rx_avail_desc_idx(q, &avail_desc_idx);

	if (res != 0) {
		return res;
	}

	struct vring_desc *desc = &q->rx.vring.desc[avail_desc_idx];

	VRING_INVALIDATE(*desc);

	uint64_t offset = desc->addr - q->vring_header_size;
	uint32_t rx_len = desc->len;

	if (offset > q->vring_data_max_len ||
		rx_len > q->vring_data_max_len ||
		offset > q->vring_data_max_len - rx_len) {
		return -EINVAL;
	}

	*data = (uint8_t *)q->rx.shmem + q->vring_header_size + offset;
	*len = desc->len;

	return 0;
}

int eth_ivshmem_queue_rx_complete(struct eth_ivshmem_queue *q)
{
	uint16_t avail_desc_idx;
	int res = get_rx_avail_desc_idx(q, &avail_desc_idx);

	if (res != 0) {
		return res;
	}

	uint16_t used_idx = q->rx.used_idx % q->desc_max_len;

	q->rx.used_idx++;
	q->rx.vring.used->ring[used_idx].id = avail_desc_idx;
	q->rx.vring.used->ring[used_idx].len = 1;
	VRING_FLUSH(q->rx.vring.used->ring[used_idx]);
	atomic_thread_fence(memory_order_seq_cst);

	q->rx.vring.used->idx = q->rx.used_idx;
	VRING_FLUSH(q->rx.vring.used->idx);
	atomic_thread_fence(memory_order_seq_cst);

	q->rx.avail_idx++;
	vring_avail_event(&q->rx.vring) = q->rx.avail_idx;
	VRING_FLUSH(vring_avail_event(&q->rx.vring));

	return 0;
}

/**
 * Calculates the vring descriptor length and header size.
 * This must match what is calculated by the peer.
 */
static int calc_vring_size(
		size_t section_size, uint16_t *vring_desc_len,
		uint32_t *vring_header_size)
{
	static const int eth_min_mtu = 68;
	uint32_t header_size;
	int16_t desc_len;

	for (desc_len = 4096; desc_len > 32; desc_len >>= 1) {
		header_size = vring_size(desc_len, ETH_IVSHMEM_VRING_ALIGNMENT);
		header_size = ROUND_UP(header_size, ETH_IVSHMEM_VRING_ALIGNMENT);
		if (header_size < section_size / 8) {
			break;
		}
	}

	if (header_size > section_size) {
		return -EINVAL;
	}

	uint32_t vring_data_size = section_size - header_size;

	if (vring_data_size < 4 * eth_min_mtu) {
		return -EINVAL;
	}

	*vring_desc_len = desc_len;
	*vring_header_size = header_size;

	return 0;
}

static uint32_t tx_buffer_advance(uint32_t max_len, uint32_t *position, uint32_t *len)
{
	uint32_t aligned_len = ETH_IVSHMEM_FRAME_SIZE(*len);
	uint32_t contiguous_len = max_len - *position;

	*len = aligned_len;
	if (aligned_len > contiguous_len) {
		/* Wrap back to zero */
		*position = 0;
		*len += contiguous_len;
	}

	return *position + aligned_len;
}

static int tx_clean_used(struct eth_ivshmem_queue *q)
{
	while (true) {
		VRING_INVALIDATE(q->tx.vring.used->idx);
		if (q->tx.used_idx == q->tx.vring.used->idx) {
			break;
		}

		struct vring_used_elem *used = &q->tx.vring.used->ring[
				q->tx.used_idx % q->desc_max_len];

		atomic_thread_fence(memory_order_seq_cst);
		VRING_INVALIDATE(*used);

		if (used->id >= q->desc_max_len || used->len != 1) {
			return -EINVAL;
		}

		struct vring_desc *desc = &q->tx.vring.desc[used->id];

		uint64_t offset = desc->addr - q->vring_header_size;
		uint32_t len = desc->len;

		uint32_t tail = q->tx.data_tail;
		uint32_t consumed_len = len;
		uint32_t new_tail = tx_buffer_advance(q->vring_data_max_len, &tail, &consumed_len);

		if (consumed_len > q->tx.data_len ||
			offset != tail) {
			return -EINVAL;
		}

		q->tx.data_tail = new_tail;
		q->tx.data_len -= consumed_len;
		q->tx.desc_len--;
		q->tx.used_idx++;
	}
	return 0;
}

static int get_rx_avail_desc_idx(struct eth_ivshmem_queue *q, uint16_t *avail_desc_idx)
{
	atomic_thread_fence(memory_order_seq_cst);
	VRING_INVALIDATE(q->rx.vring.avail->idx);

	uint16_t avail_idx = q->rx.vring.avail->idx;

	if (avail_idx == q->rx.avail_idx) {
		return -EWOULDBLOCK;
	}

	VRING_INVALIDATE(q->rx.vring.avail->ring[q->rx.avail_idx % q->desc_max_len]);
	*avail_desc_idx = q->rx.vring.avail->ring[q->rx.avail_idx % q->desc_max_len];
	if (*avail_desc_idx >= q->desc_max_len) {
		return -EINVAL;
	}

	return 0;
}
