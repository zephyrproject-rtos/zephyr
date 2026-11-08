/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "fake_device.h"

/* Include the private driver without instantiating a hardware device. */
#define CONFIG_ETHERNET_LOG_LEVEL        0
#define CONFIG_ETH_VIRTIO_NET_RX_BUFFERS 4
#define CONFIG_ETH_VIRTIO_NET_TX_BUFFERS 4

static struct virtq *tx_queue(const struct device *dev, uint16_t index)
{
	ARG_UNUSED(index);

	/* The shared software transport serves one queue, numbered zero. */
	return virtio_get_virtqueue(dev, 0);
}

static bool complete_on_notify;

static void tx_notify(const struct device *dev, uint16_t index)
{
	ARG_UNUSED(index);

	virtio_notify_virtqueue(dev, 0);
	if (complete_on_notify) {
		fake_device_complete_request(
			dev, fake_device_last_submitted_head(fake_device_get_attached_queue(dev)),
			0);
	}
}

#define virtio_notify_virtqueue tx_notify
#define virtio_get_virtqueue    tx_queue
#include "eth_virtio_net.c"
#undef virtio_get_virtqueue
#undef virtio_notify_virtqueue

static const void *const kept_symbols[] __unused = {
	(void *)virtnet_dev_init,
	(void *)&virtnet_api,
};

NET_BUF_POOL_FIXED_DEFINE(payload_pool, 24, 128, 0, NULL);
NET_PKT_SLAB_DEFINE(packet_pool, 8);

static struct virtnet_data data;
static struct virtnet_config config;
static struct device netdev = {.data = &data, .config = &config};
static struct virtq vq;

static void reset_queue(uint16_t size)
{
	memset(&data, 0, sizeof(data));
	complete_on_notify = false;
	config.vdev = fake_device_a();
	zassert_ok(virtq_create(&vq, size));
	fake_device_attach_queue(config.vdev, &vq);
	k_fifo_init(&data.tx_free);
	for (size_t i = 0; i < ARRAY_SIZE(data.tx); i++) {
		data.tx[i].free = &data.tx_free;
		memset(data.tx[i].buf, 0xa5, sizeof(data.tx[i].buf));
		k_fifo_put(&data.tx_free, &data.tx[i]);
	}
}

static void free_queue(void)
{
	zassert_equal(atomic_get(&vq.free_desc_n), vq.num);
	zassert_equal(k_mem_slab_num_used_get(&packet_pool), 0);
	for (size_t i = 0; i < ARRAY_SIZE(data.tx); i++) {
		zassert_not_null(k_fifo_get(&data.tx_free, K_NO_WAIT));
	}
	zassert_is_null(k_fifo_get(&data.tx_free, K_NO_WAIT));
	fake_device_attach_queue(config.vdev, NULL);
	virtq_free(&vq);
}

static struct net_pkt *packet(size_t len)
{
	struct net_pkt *pkt = net_pkt_alloc_from_slab(&packet_pool, K_NO_WAIT);

	zassert_not_null(pkt);
	while (len > 0U) {
		struct net_buf *frag = net_buf_alloc(&payload_pool, K_NO_WAIT);
		size_t part = MIN(len, 128U);

		zassert_not_null(frag);
		memset(net_buf_add(frag, part), 0x42, part);
		net_pkt_frag_add(pkt, frag);
		len -= part;
	}
	net_pkt_cursor_init(pkt);
	return pkt;
}

static uint16_t head_at(uint16_t index)
{
	return sys_le16_to_cpu(vq.avail->ring[index % vq.num]);
}

static uint16_t chain_length(uint16_t head)
{
	uint16_t count = 1U;

	while ((sys_le16_to_cpu(vq.desc[head].flags) & VIRTQ_DESC_F_NEXT) != 0U) {
		head = sys_le16_to_cpu(vq.desc[head].next);
		zassert_true(++count <= vq.num);
	}
	return count;
}

static void assert_direct(struct net_pkt *pkt, uint16_t head)
{
	uint16_t index = head;
	uint16_t count = 1U;
	uint8_t zero[sizeof(struct _virtio_net_hdr)] = {0};
	struct virtnet_tx *tx = vq.recv_cbs[head].opaque;

	zassert_equal(sys_le64_to_cpu(vq.desc[index].addr), k_mem_phys_addr(tx->buf));
	zassert_equal(sys_le32_to_cpu(vq.desc[index].len), sizeof(struct _virtio_net_hdr));
	zassert_mem_equal(tx->buf, zero, sizeof(zero));
	for (struct net_buf *frag = pkt->frags; frag != NULL; frag = frag->frags) {
		if (frag->len == 0U) {
			continue;
		}
		zassert_true((sys_le16_to_cpu(vq.desc[index].flags) & VIRTQ_DESC_F_NEXT) != 0U);
		index = sys_le16_to_cpu(vq.desc[index].next);
		zassert_equal(sys_le64_to_cpu(vq.desc[index].addr), k_mem_phys_addr(frag->data));
		zassert_equal(sys_le32_to_cpu(vq.desc[index].len), frag->len);
		count++;
	}
	zassert_equal(chain_length(head), count);
	/* No payload byte was copied into the fallback storage. */
	for (size_t i = sizeof(struct _virtio_net_hdr); i < sizeof(tx->buf); i++) {
		zassert_equal(tx->buf[i], 0xa5);
	}
}

static void assert_copied(struct net_pkt *pkt, uint16_t head)
{
	struct virtnet_tx *tx = vq.recv_cbs[head].opaque;
	uint8_t expected[NET_ETH_MAX_FRAME_SIZE];
	size_t len = net_pkt_get_len(pkt);

	zassert_equal(chain_length(head), 1);
	zassert_equal(sys_le64_to_cpu(vq.desc[head].addr), k_mem_phys_addr(tx->buf));
	zassert_equal(sys_le32_to_cpu(vq.desc[head].len), len + sizeof(struct _virtio_net_hdr));
	zassert_equal(net_buf_linearize(expected, sizeof(expected), pkt->frags, 0, len), len);
	zassert_mem_equal(tx->buf + sizeof(struct _virtio_net_hdr), expected, len);
}

ZTEST(virtio_net_tx, test_full_frame_direct_and_packet_lifetime)
{
	reset_queue(32);
	struct net_pkt *pkt = packet(NET_ETH_MAX_FRAME_SIZE);
	struct net_pkt_cursor before;

	net_pkt_set_overwrite(pkt, true);
	zassert_ok(net_pkt_skip(pkt, 1));
	before = pkt->cursor;
	zassert_ok(virtnet_send(&netdev, pkt));
	zassert_equal(atomic_get(&pkt->atomic_ref), 2);
	assert_direct(pkt, head_at(0));
	zassert_mem_equal(&pkt->cursor, &before, sizeof(before));
	net_pkt_unref(pkt);
	zassert_equal(k_mem_slab_num_used_get(&packet_pool), 1);
	fake_device_complete_request(config.vdev, head_at(0), 0);
	free_queue();
}

ZTEST(virtio_net_tx, test_fragment_budget_falls_back)
{
	reset_queue(32);
	struct net_pkt *pkt = packet(0);

	for (uint16_t i = 0; i < VIRTIO_NET_TX_DESCRIPTORS; i++) {
		struct net_buf *frag = net_buf_alloc(&payload_pool, K_NO_WAIT);

		zassert_not_null(frag);
		net_buf_add_u8(frag, i);
		net_pkt_frag_add(pkt, frag);
	}
	net_pkt_cursor_init(pkt);
	struct net_pkt_cursor before;

	net_pkt_set_overwrite(pkt, true);
	zassert_ok(net_pkt_skip(pkt, 1));
	before = pkt->cursor;
	zassert_ok(virtnet_send(&netdev, pkt));
	assert_copied(pkt, head_at(0));
	zassert_mem_equal(&pkt->cursor, &before, sizeof(before));
	net_pkt_unref(pkt);
	fake_device_complete_request(config.vdev, head_at(0), 0);
	free_queue();
}

ZTEST(virtio_net_tx, test_small_queue_falls_back)
{
	reset_queue(1);
	struct net_pkt *pkt = packet(256);

	zassert_ok(virtnet_send(&netdev, pkt));
	assert_copied(pkt, head_at(0));
	net_pkt_unref(pkt);
	fake_device_complete_request(config.vdev, head_at(0), 0);
	free_queue();
}

ZTEST(virtio_net_tx, test_non_linear_mixed_fragments_fall_back)
{
#if defined(CONFIG_MMU)
	uint8_t *external = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);

	zassert_not_null(external);
	memset(external, 0x7c, 128);
#else
	static const uint8_t external[128] = {0x7c};
#endif
	reset_queue(32);
	struct net_pkt *pkt = packet(128);
	struct net_buf *frag =
		net_buf_alloc_with_data(&payload_pool, (void *)external, 128, K_NO_WAIT);

	zassert_false(virtio_buf_is_in_linear_mapping(external, 128));
	zassert_not_null(frag);
	net_pkt_frag_add(pkt, frag);
	zassert_ok(virtnet_send(&netdev, pkt));
	assert_copied(pkt, head_at(0));
	net_pkt_unref(pkt);
	fake_device_complete_request(config.vdev, head_at(0), 0);
	free_queue();
#if defined(CONFIG_MMU)
	k_mem_unmap(external, CONFIG_MMU_PAGE_SIZE);
#endif
}

struct irq_send {
	struct net_pkt *pkt;
	int result;
};

static void send_isr(const void *arg)
{
	struct irq_send *send = (struct irq_send *)arg;

	send->result = virtnet_send(&netdev, send->pkt);
}

ZTEST(virtio_net_tx, test_pending_slots_complete_out_of_order_and_reuse)
{
	reset_queue(32);
	uint16_t heads[CONFIG_ETH_VIRTIO_NET_TX_BUFFERS];

	for (size_t i = 0; i < ARRAY_SIZE(heads); i++) {
		struct net_pkt *pkt = packet(256);

		zassert_ok(virtnet_send(&netdev, pkt));
		heads[i] = head_at(i);
		assert_direct(pkt, heads[i]);
		net_pkt_unref(pkt);
	}
	struct irq_send send = {.pkt = packet(256)};

	irq_offload(send_isr, &send);
	zassert_equal(send.result, -EBUSY);
	zassert_equal(atomic_get(&send.pkt->atomic_ref), 1);
	fake_device_complete_request(config.vdev, heads[2], 0);
	zassert_ok(virtnet_send(&netdev, send.pkt));
	zassert_equal(vq.recv_cbs[head_at(4)].opaque, &data.tx[2]);
	net_pkt_unref(send.pkt);
	fake_device_complete_request(config.vdev, heads[0], 0);
	fake_device_complete_request(config.vdev, head_at(4), 0);
	fake_device_complete_request(config.vdev, heads[3], 0);
	fake_device_complete_request(config.vdev, heads[1], 0);
	free_queue();
}

ZTEST(virtio_net_tx, test_enqueue_failure_releases_slot_and_reference)
{
	reset_queue(2);
	struct net_pkt *pkt = packet(128);
	struct irq_send send = {.pkt = packet(128)};

	zassert_ok(virtnet_send(&netdev, pkt));
	irq_offload(send_isr, &send);
	zassert_equal(send.result, -EBUSY);
	zassert_equal(atomic_get(&send.pkt->atomic_ref), 1);
	zassert_equal(sys_le16_to_cpu(vq.avail->idx), 1);
	net_pkt_unref(pkt);
	fake_device_complete_request(config.vdev, head_at(0), 0);
	zassert_ok(virtnet_send(&netdev, send.pkt));
	net_pkt_unref(send.pkt);
	fake_device_complete_request(config.vdev, head_at(1), 0);
	free_queue();
}

static void send_thread(void *p1, void *p2, void *p3)
{
	struct irq_send *send = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	send->result = virtnet_send(&netdev, send->pkt);
}

ZTEST(virtio_net_tx, test_full_queue_waits_for_whole_chain_without_copy)
{
	static K_THREAD_STACK_DEFINE(sender_stack, 2048);
	struct k_thread sender_thread;

	reset_queue(4);
	struct net_pkt *pkt = packet(384);
	struct irq_send send = {.pkt = packet(256)};

	zassert_ok(virtnet_send(&netdev, pkt));
	zassert_equal(atomic_get(&vq.free_desc_n), 0);
	k_thread_create(&sender_thread, sender_stack, K_THREAD_STACK_SIZEOF(sender_stack),
			send_thread, &send, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	k_timepoint_t deadline = sys_timepoint_calc(K_SECONDS(1));

	while (atomic_get(&vq.waiters) == 0 && !sys_timepoint_expired(deadline)) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(atomic_get(&vq.waiters), 1);
	zassert_equal(sys_le16_to_cpu(vq.avail->idx), 1);
	zassert_equal(atomic_get(&send.pkt->atomic_ref), 2);
	for (size_t i = sizeof(struct _virtio_net_hdr); i < sizeof(data.tx[1].buf); i++) {
		zassert_equal(data.tx[1].buf[i], 0xa5);
	}
	net_pkt_unref(pkt);
	fake_device_complete_request(config.vdev, head_at(0), 0);
	zassert_ok(k_thread_join(&sender_thread, K_SECONDS(1)));
	zassert_ok(send.result);
	assert_direct(send.pkt, head_at(1));
	net_pkt_unref(send.pkt);
	fake_device_complete_request(config.vdev, head_at(1), 0);
	free_queue();
}

ZTEST(virtio_net_tx, test_immediate_completion_and_empty_fragments)
{
	reset_queue(32);
	struct net_pkt *pkt = packet(128);
	struct net_buf *empty = net_buf_alloc(&payload_pool, K_NO_WAIT);

	zassert_not_null(empty);
	net_pkt_frag_insert(pkt, empty);
	complete_on_notify = true;
	for (size_t i = 0; i < 2U * ARRAY_SIZE(data.tx); i++) {
		zassert_ok(virtnet_send(&netdev, pkt));
		zassert_equal(atomic_get(&pkt->atomic_ref), 1);
		zassert_equal(atomic_get(&vq.free_desc_n), vq.num);
	}
	net_pkt_unref(pkt);
	free_queue();
}

ZTEST(virtio_net_tx, test_oversized_frame_rejected_before_slot_allocation)
{
	reset_queue(32);
	struct net_pkt *pkt = packet(NET_ETH_MAX_FRAME_SIZE + 1U);

	zassert_equal(virtnet_send(&netdev, pkt), -EMSGSIZE);
	zassert_equal(atomic_get(&pkt->atomic_ref), 1);
	zassert_equal(sys_le16_to_cpu(vq.avail->idx), 0);
	net_pkt_unref(pkt);
	free_queue();
}

ZTEST(virtio_net_tx, test_direct_fragment_budget_boundary)
{
	reset_queue(32);
	struct net_pkt *pkt = packet(0);

	for (uint16_t i = 1; i < VIRTIO_NET_TX_DESCRIPTORS; i++) {
		struct net_buf *frag = net_buf_alloc(&payload_pool, K_NO_WAIT);

		zassert_not_null(frag);
		net_buf_add_u8(frag, i);
		net_pkt_frag_add(pkt, frag);
	}
	zassert_ok(virtnet_send(&netdev, pkt));
	assert_direct(pkt, head_at(0));
	zassert_equal(chain_length(head_at(0)), VIRTIO_NET_TX_DESCRIPTORS);
	net_pkt_unref(pkt);
	fake_device_complete_request(config.vdev, head_at(0), 0);
	free_queue();
}

ZTEST(virtio_net_tx, test_queue_size_limits)
{
	for (uint32_t maximum = 0U; maximum <= UINT16_MAX; maximum++) {
		for (uint16_t index = 0U; index < 2U; index++) {
			uint16_t size = virtnet_enum_queues_cb(index, maximum, NULL);

			zassert_true(size <= maximum);
			zassert_true(size == 0U || (size <= KB(32) && IS_POWER_OF_TWO(size)));
			zassert_equal(size == 0U, maximum == 0U);
		}
	}
	zassert_equal(virtnet_enum_queues_cb(1, 1, NULL), 1);
	zassert_equal(virtnet_enum_queues_cb(1, 3, NULL), 2);
	zassert_equal(virtnet_enum_queues_cb(1, UINT16_MAX, NULL), 128);
}

ZTEST_SUITE(virtio_net_tx, NULL, NULL, NULL, NULL, NULL);
