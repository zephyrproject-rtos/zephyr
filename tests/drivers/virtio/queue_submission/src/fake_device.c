/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>

#include "virtio_common.h"
#include "fake_device.h"

struct fake_device_data {
	struct virtq *vq;
};

static struct fake_device_data fake_device_a_data;
static struct fake_device_data fake_device_b_data;

static struct virtq *fake_device_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	struct fake_device_data *data = dev->data;

	if (queue_idx != 0U) {
		return NULL;
	}

	return data->vq;
}

static void fake_device_notify_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(queue_idx);
}

static void *fake_device_get_device_specific_config(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NULL;
}

static bool fake_device_read_device_feature_bit(const struct device *dev, int bit)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(bit);

	return false;
}

static int fake_device_write_driver_feature_bit(const struct device *dev, int bit, bool value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(bit);
	ARG_UNUSED(value);

	return 0;
}

static int fake_device_commit_feature_bits(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int fake_device_init_virtqueues(const struct device *dev, uint16_t num_queues,
				       virtio_enumerate_queues cb, void *opaque)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(num_queues);
	ARG_UNUSED(cb);
	ARG_UNUSED(opaque);

	return 0;
}

static void fake_device_finalize_init(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static DEVICE_API(virtio, fake_device_api) = {
	.get_virtqueue = fake_device_get_virtqueue,
	.notify_virtqueue = fake_device_notify_virtqueue,
	.get_device_specific_config = fake_device_get_device_specific_config,
	.read_device_feature_bit = fake_device_read_device_feature_bit,
	.write_driver_feature_bit = fake_device_write_driver_feature_bit,
	.commit_feature_bits = fake_device_commit_feature_bits,
	.init_virtqueues = fake_device_init_virtqueues,
	.finalize_init = fake_device_finalize_init,
};

DEVICE_DEFINE(fake_virtio_a, "virtio-fake-a", NULL, NULL, &fake_device_a_data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_device_api);
DEVICE_DEFINE(fake_virtio_b, "virtio-fake-b", NULL, NULL, &fake_device_b_data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_device_api);

const struct device *fake_device_a(void)
{
	return DEVICE_GET(fake_virtio_a);
}

const struct device *fake_device_b(void)
{
	return DEVICE_GET(fake_virtio_b);
}

void fake_device_attach_queue(const struct device *dev, struct virtq *vq)
{
	struct fake_device_data *data = dev->data;

	data->vq = vq;
}

struct virtq *fake_device_get_attached_queue(const struct device *dev)
{
	struct fake_device_data *data = dev->data;

	return data->vq;
}

void fake_device_add_completion(const struct device *dev, uint16_t head, uint32_t len)
{
	struct virtq *vq = fake_device_get_attached_queue(dev);
	uint16_t idx = sys_le16_to_cpu(vq->used->idx);
	struct virtq_used_elem *elem = &vq->used->ring[idx % vq->num];

	elem->id = sys_cpu_to_le32(head);
	elem->len = sys_cpu_to_le32(len);
	barrier_dmem_fence_full();
	vq->used->idx = sys_cpu_to_le16(idx + 1U);
}

struct fake_device_irq {
	const struct device *dev;
};

static void fake_device_irq_handler(const void *arg)
{
	const struct fake_device_irq *irq = arg;

	virtio_isr(irq->dev, VIRTIO_QUEUE_INTERRUPT, 1);
}

void fake_device_complete_request(const struct device *dev, uint16_t head, uint32_t len)
{
	struct fake_device_irq irq = {.dev = dev};

	fake_device_add_completion(dev, head, len);
	irq_offload(fake_device_irq_handler, &irq);
}

uint16_t fake_device_last_submitted_head(const struct virtq *vq)
{
	uint16_t idx = sys_le16_to_cpu(vq->avail->idx);

	__ASSERT_NO_MSG(vq->num != 0U);
	__ASSERT_NO_MSG(idx != 0U);

	return sys_le16_to_cpu(vq->avail->ring[(idx - 1U) & (vq->num - 1U)]);
}
