/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/internal/mm.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "virtio_common.h"

#define VIRTQ_CONTROL_RX 2U
#define VIRTQ_CONTROL_TX 3U

static uint16_t port_rx_vq(uint32_t port)
{
	return port != 0U ? (uint16_t)((port + 1U) * 2U) : 0U;
}

static uint16_t port_tx_vq(uint32_t port)
{
	return (uint16_t)(port_rx_vq(port) + 1U);
}

#define CTL_DEVICE_REMOVE 2U
#define CTL_CONSOLE_PORT 4U
#define CTL_PORT_OPEN 6U

#define VIRTIO_CONSOLE_MAX_PORTS 32U
#define FAKE_MAX_QUEUES 66U
#define FAKE_MAX_QUEUE_SIZE 64U

struct console_dev_cfg {
	uint16_t cols;
	uint16_t rows;
	uint32_t max_nr_ports;
	uint32_t emerg_wr;
};

struct console_control {
	uint32_t port;
	uint16_t event;
	uint16_t value;
	char name[CONFIG_UART_VIRTIO_CONSOLE_NAME_BUFSIZE];
};

struct fake_virtio_data {
	struct virtq queues[FAKE_MAX_QUEUES];
	uint16_t queue_count;
};

static struct fake_virtio_data fake_data;

static const struct device *const vdev = DEVICE_DT_GET(DT_NODELABEL(fake_virtio));
static const struct device *const console = DEVICE_DT_GET(DT_NODELABEL(fake_console));
static struct virtq *ctrl_rx;
static struct virtq *ctrl_tx;
static uint16_t queue_count;

static struct virtq *fake_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	struct fake_virtio_data *data = dev->data;

	return queue_idx < data->queue_count ? &data->queues[queue_idx] : NULL;
}

static void fake_notify_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	struct fake_virtio_data *data = dev->data;
	struct virtq *vq = &data->queues[queue_idx];
	uint16_t used;
	uint16_t avail;

	/* Only complete transmit queues; receive buffers are completed by the test. */
	if (queue_idx % 2U == 0U) {
		return;
	}

	used = sys_le16_to_cpu(vq->used->idx);
	avail = sys_le16_to_cpu(vq->avail->idx);

	while (used != avail) {
		uint16_t ring = used % vq->num;
		uint16_t head = sys_le16_to_cpu(vq->avail->ring[ring]);
		uint32_t len = sys_le32_to_cpu(vq->desc[head].len);

		vq->used->ring[ring].id = sys_cpu_to_le32(head);
		vq->used->ring[ring].len = sys_cpu_to_le32(len);
		used++;
	}

	barrier_dmem_fence_full();
	vq->used->idx = sys_cpu_to_le16(used);
}

static void *fake_get_device_specific_config(const struct device *dev)
{
	static struct console_dev_cfg fake_cfg = {
		.max_nr_ports = VIRTIO_CONSOLE_MAX_PORTS,
	};

	ARG_UNUSED(dev);

	return &fake_cfg;
}

static bool fake_read_device_feature_bit(const struct device *dev, int bit)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(bit);

	return true;
}

static int fake_write_driver_feature_bit(const struct device *dev, int bit, bool value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(bit);
	ARG_UNUSED(value);

	return 0;
}

static int fake_commit_feature_bits(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int fake_init_virtqueues(const struct device *dev, uint16_t num_queues,
				virtio_enumerate_queues cb, void *opaque)
{
	struct fake_virtio_data *data = dev->data;

	if (num_queues > FAKE_MAX_QUEUES) {
		return -EINVAL;
	}

	for (uint16_t i = 0U; i < num_queues; i++) {
		uint16_t size = cb(i, FAKE_MAX_QUEUE_SIZE, opaque);
		int ret = virtq_create(&data->queues[i], size);

		if (ret != 0) {
			return ret;
		}
	}

	data->queue_count = num_queues;

	return 0;
}

static void fake_finalize_init(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static DEVICE_API(virtio, fake_virtio_api) = {
	.get_virtqueue = fake_get_virtqueue,
	.notify_virtqueue = fake_notify_virtqueue,
	.get_device_specific_config = fake_get_device_specific_config,
	.read_device_feature_bit = fake_read_device_feature_bit,
	.write_driver_feature_bit = fake_write_driver_feature_bit,
	.commit_feature_bits = fake_commit_feature_bits,
	.init_virtqueues = fake_init_virtqueues,
	.finalize_init = fake_finalize_init,
};

static int fake_virtio_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

DEVICE_DT_DEFINE(DT_NODELABEL(fake_virtio), fake_virtio_init, NULL, &fake_data, NULL,
		 POST_KERNEL, 0, &fake_virtio_api);

static uint16_t vq_avail_idx(struct virtq *vq)
{
	return sys_le16_to_cpu(vq->avail->idx);
}

static uint16_t vq_used_idx(struct virtq *vq)
{
	return sys_le16_to_cpu(vq->used->idx);
}

static void inject_control(uint32_t port, uint16_t event)
{
	uint16_t used = vq_used_idx(ctrl_rx);
	uint16_t avail = vq_avail_idx(ctrl_rx);
	uint16_t ring;
	uint16_t head;
	struct console_control *msg;

	zassert_true(avail != used, "no control RX buffer in flight");

	ring = used % ctrl_rx->num;
	head = sys_le16_to_cpu(ctrl_rx->avail->ring[ring]);
	msg = k_mem_virt_addr((uintptr_t)sys_le64_to_cpu(ctrl_rx->desc[head].addr));
	zassert_not_null(msg, "could not map the control buffer");

	memset(msg, 0, sizeof(*msg));
	msg->port = sys_cpu_to_le32(port);
	msg->event = sys_cpu_to_le16(event);
	msg->value = sys_cpu_to_le16(0U);

	ctrl_rx->used->ring[ring].id = sys_cpu_to_le32(head);
	ctrl_rx->used->ring[ring].len = sys_cpu_to_le32(sizeof(*msg));
	barrier_dmem_fence_full();
	ctrl_rx->used->idx = sys_cpu_to_le16(used + 1U);

	virtio_isr(vdev, VIRTIO_QUEUE_INTERRUPT, queue_count);
}

static struct console_control *last_tx_message(void)
{
	uint16_t avail = vq_avail_idx(ctrl_tx);
	uint16_t ring;
	uint16_t head;

	zassert_true(avail != 0U, "no control message was sent");

	ring = (avail - 1U) % ctrl_tx->num;
	head = sys_le16_to_cpu(ctrl_tx->avail->ring[ring]);
	return k_mem_virt_addr((uintptr_t)sys_le64_to_cpu(ctrl_tx->desc[head].addr));
}

static void check_invalid_port(uint32_t port)
{
	uint16_t q = port_rx_vq(port);
	struct virtq *rx_vq = NULL;
	uint16_t rx_before = 0U;
	uint16_t tx_before = vq_avail_idx(ctrl_tx);

	/*
	 * A wrapped index can name the control RX queue, which the injection
	 * re-arms itself. Only data RX queues are meaningful here.
	 */
	if (q < queue_count && q != VIRTQ_CONTROL_RX) {
		rx_vq = virtio_get_virtqueue(vdev, q);
		if (rx_vq != NULL) {
			rx_before = vq_avail_idx(rx_vq);
		}
	}

	inject_control(port, CTL_CONSOLE_PORT);

	zassert_equal(vq_avail_idx(ctrl_tx), tx_before,
		      "invalid port 0x%x sent a control message", port);
	if (rx_vq != NULL) {
		zassert_equal(vq_avail_idx(rx_vq), rx_before,
			      "invalid port 0x%x re-armed RX queue %u", port, q);
	}
}

ZTEST(uart_virtio_console_control, test_invalid_ports_rejected)
{
	check_invalid_port(32U);
	check_invalid_port(0x7fffU);
	check_invalid_port(0x80000000U);
}

ZTEST(uart_virtio_console_control, test_invalid_device_remove_rejected)
{
	struct virtq *tx31 = virtio_get_virtqueue(vdev, port_tx_vq(31U));
	uint16_t tx_before;

	zassert_not_null(tx31, "TX queue for port 31 is missing");

	/* Enable port 31 so its bit is set in console_ports. */
	inject_control(31U, CTL_CONSOLE_PORT);

	/*
	 * A DEVICE_REMOVE of 0xffffffdf must be rejected. Narrowed to int it
	 * becomes -33, which passes port < VIRTIO_CONSOLE_MAX_PORTS and whose
	 * masked shift aliases bit 31, clearing the port enabled above.
	 */
	inject_control(0xffffffdfU, CTL_DEVICE_REMOVE);

	tx_before = vq_avail_idx(tx31);
	uart_poll_out(console, 'a');
	zassert_equal(vq_avail_idx(tx31), tx_before + 1U,
		      "invalid DEVICE_REMOVE disabled port 31");

	/* A valid DEVICE_REMOVE clears the port as before. */
	inject_control(31U, CTL_DEVICE_REMOVE);

	tx_before = vq_avail_idx(tx31);
	uart_poll_out(console, 'b');
	zassert_equal(vq_avail_idx(tx31), tx_before,
		      "valid DEVICE_REMOVE did not disable port 31");
}

ZTEST(uart_virtio_console_control, test_valid_port_accepted)
{
	struct console_control *msg;
	uint16_t tx_before;

	/* Port 31 is the highest port accepted by the driver. */
	tx_before = vq_avail_idx(ctrl_tx);
	inject_control(31U, CTL_CONSOLE_PORT);
	zassert_equal(vq_avail_idx(ctrl_tx), tx_before + 1U,
		      "valid port 31 did not send PORT_OPEN");

	msg = last_tx_message();
	zassert_not_null(msg, "could not map the control message");
	zassert_equal(sys_le32_to_cpu(msg->port), 31U, "wrong port in control message");
	zassert_equal(sys_le16_to_cpu(msg->event), CTL_PORT_OPEN, "wrong control event");

	/* A lower valid port must behave the same way. */
	tx_before = vq_avail_idx(ctrl_tx);
	inject_control(1U, CTL_CONSOLE_PORT);
	zassert_equal(vq_avail_idx(ctrl_tx), tx_before + 1U,
		      "valid port 1 did not send PORT_OPEN");

	/* Leave the driver in the state the suite started with. */
	inject_control(31U, CTL_DEVICE_REMOVE);
	inject_control(1U, CTL_DEVICE_REMOVE);
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(vdev), "fake virtio device is not ready");
	zassert_true(device_is_ready(console), "virtio console is not ready");

	queue_count = fake_data.queue_count;
	zassert_equal(queue_count, FAKE_MAX_QUEUES, "unexpected queue count");

	ctrl_rx = virtio_get_virtqueue(vdev, VIRTQ_CONTROL_RX);
	ctrl_tx = virtio_get_virtqueue(vdev, VIRTQ_CONTROL_TX);
	zassert_not_null(ctrl_rx, "no control RX queue");
	zassert_not_null(ctrl_tx, "no control TX queue");

	/* Make the CONSOLE_PORT path set up data reception. */
	uart_irq_rx_enable(console);

	return NULL;
}

ZTEST_SUITE(uart_virtio_console_control, NULL, suite_setup, NULL, NULL, NULL);
