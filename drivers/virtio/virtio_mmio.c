/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include "virtio_common.h"

#define DT_DRV_COMPAT virtio_mmio

LOG_MODULE_REGISTER(virtio_mmio, CONFIG_VIRTIO_LOG_LEVEL);

#define VIRTIO_MMIO_MAGIC             0x74726976
#define VIRTIO_MMIO_SUPPORTED_VERSION 2
#define VIRTIO_MMIO_INVALID_DEVICE_ID 0

#define DEV_CFG(dev)  ((const struct virtio_mmio_config *)(dev)->config)
#define DEV_DATA(dev) ((struct virtio_mmio_data *)(dev)->data)

struct virtio_mmio_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);

	struct virtq *virtqueues;
	uint16_t virtqueue_count;

	struct k_spinlock isr_lock;
	struct k_spinlock notify_lock;
};

struct virtio_mmio_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
};

static inline uint32_t virtio_mmio_read32(const struct device *dev, uint32_t offset)
{
	const mem_addr_t reg = DEVICE_MMIO_NAMED_GET(dev, reg_base) + offset;
	uint32_t val;

	barrier_dmem_fence_full();
	val = sys_read32(reg);
	return sys_le32_to_cpu(val);
}

static inline void virtio_mmio_write32(const struct device *dev, uint32_t offset, uint32_t val)
{
	const mem_addr_t reg = DEVICE_MMIO_NAMED_GET(dev, reg_base) + offset;

	sys_write32(sys_cpu_to_le32(val), reg);
	barrier_dmem_fence_full();
}

static void virtio_mmio_isr(const struct device *dev)
{
	struct virtio_mmio_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->isr_lock);
	const uint32_t isr_status = virtio_mmio_read32(dev, VIRTIO_MMIO_INTERRUPT_STATUS);

	virtio_mmio_write32(dev, VIRTIO_MMIO_INTERRUPT_ACK, isr_status);
	virtio_isr(dev, isr_status, data->virtqueue_count);

	k_spin_unlock(&data->isr_lock, key);
}

struct virtq *virtio_mmio_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	struct virtio_mmio_data *data = dev->data;

	return queue_idx < data->virtqueue_count ? &data->virtqueues[queue_idx] : NULL;
}

static void virtio_mmio_notify_queue(const struct device *dev, uint16_t queue_idx)
{
	struct virtio_mmio_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->notify_lock);

	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_NOTIFY, queue_idx);

	k_spin_unlock(&data->notify_lock, key);
}

static void *virtio_mmio_get_device_specific_config(const struct device *dev)
{
	const mem_addr_t reg = DEVICE_MMIO_NAMED_GET(dev, reg_base) + VIRTIO_MMIO_CONFIG;

	barrier_dmem_fence_full();

	return (void *)reg;
}

static bool virtio_mmio_read_device_feature_bit(const struct device *dev, int bit)
{
	virtio_mmio_write32(dev, VIRTIO_MMIO_DEVICE_FEATURES_SEL, bit / 32);
	const uint32_t val = virtio_mmio_read32(dev, VIRTIO_MMIO_DEVICE_FEATURES);

	return !!(val & BIT(bit % 32));
}

static void virtio_mmio_write_driver_feature_bit(const struct device *dev, int bit, bool value)
{
	const uint32_t mask = sys_cpu_to_le32(BIT(bit % 32));

	virtio_mmio_write32(dev, VIRTIO_MMIO_DRIVER_FEATURES_SEL, bit / 32);

	const uint32_t regval = virtio_mmio_read32(dev, VIRTIO_MMIO_DRIVER_FEATURES);

	if (value) {
		virtio_mmio_write32(dev, VIRTIO_MMIO_DRIVER_FEATURES, regval | mask);
	} else {
		virtio_mmio_write32(dev, VIRTIO_MMIO_DRIVER_FEATURES, regval & ~mask);
	}
}

static bool virtio_mmio_read_status_bit(const struct device *dev, int bit)
{
	const uint32_t val = virtio_mmio_read32(dev, VIRTIO_MMIO_STATUS);

	return !!(val & BIT(bit));
}

static void virtio_mmio_write_status_bit(const struct device *dev, int bit)
{
	const uint32_t mask = sys_cpu_to_le32(BIT(bit));
	const uint32_t val = virtio_mmio_read32(dev, VIRTIO_MMIO_STATUS);

	virtio_mmio_write32(dev, VIRTIO_MMIO_STATUS, val | mask);
}

static int virtio_mmio_write_driver_feature_bit_range_check(const struct device *dev, int bit,
							    bool value)
{
	if (!IN_RANGE(bit, DEV_TYPE_FEAT_RANGE_0_BEGIN, DEV_TYPE_FEAT_RANGE_0_END) &&
	    !IN_RANGE(bit, DEV_TYPE_FEAT_RANGE_1_BEGIN, DEV_TYPE_FEAT_RANGE_1_END)) {
		return -EINVAL;
	}

	virtio_mmio_write_driver_feature_bit(dev, bit, value);

	return 0;
}

static int virtio_mmio_commit_feature_bits(const struct device *dev)
{
	virtio_mmio_write_status_bit(dev, DEVICE_STATUS_FEATURES_OK);
	if (!virtio_mmio_read_status_bit(dev, DEVICE_STATUS_FEATURES_OK)) {
		return -EINVAL;
	}

	return 0;
}

static void virtio_mmio_reset(const struct device *dev)
{
	virtio_mmio_write32(dev, VIRTIO_MMIO_STATUS, 0);

	while (virtio_mmio_read_status_bit(dev, VIRTIO_MMIO_STATUS) != 0) {
	}
}

static int virtio_mmio_set_virtqueue(const struct device *dev, uint16_t virtqueue_n,
				     struct virtq *virtqueue)
{
	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_SEL, virtqueue_n);

	uint16_t max_queue_size = virtio_mmio_read32(dev, VIRTIO_MMIO_QUEUE_SIZE_MAX);

	if (max_queue_size < virtqueue->num) {
		LOG_ERR("%s doesn't support queue %d bigger than %d, tried to set "
			"one with size %d",
			dev->name, virtqueue_n, max_queue_size, virtqueue->num);
		return -EINVAL;
	}

	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_SIZE, virtqueue->num);
	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_DESC_LOW,
			    k_mem_phys_addr(virtqueue->desc) & UINT32_MAX);
	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_DESC_HIGH,
			    k_mem_phys_addr(virtqueue->desc) >> 32);
	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_AVAIL_LOW,
			    k_mem_phys_addr(virtqueue->avail) & UINT32_MAX);
	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_AVAIL_HIGH,
			    k_mem_phys_addr(virtqueue->avail) >> 32);
	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_USED_LOW,
			    k_mem_phys_addr(virtqueue->used) & UINT32_MAX);
	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_USED_HIGH,
			    k_mem_phys_addr(virtqueue->used) >> 32);

	virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_READY, 1);

	return 0;
}

static int virtio_mmio_set_virtqueues(const struct device *dev, uint16_t queue_count,
				      virtio_enumerate_queues cb, void *opaque)
{
	struct virtio_mmio_data *data = dev->data;

	data->virtqueues = k_malloc(queue_count * sizeof(struct virtq));
	if (!data->virtqueues) {
		LOG_ERR("failed to allocate virtqueue array");
		return -ENOMEM;
	}
	data->virtqueue_count = queue_count;

	int ret = 0;
	int created_queues = 0;
	int activated_queues = 0;

	for (int i = 0; i < queue_count; i++) {
		virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_SEL, i);

		const uint16_t queue_size =
			cb(i, virtio_mmio_read32(dev, VIRTIO_MMIO_QUEUE_SIZE_MAX), opaque);

		ret = virtq_create(&data->virtqueues[i], queue_size);
		if (ret != 0) {
			goto fail;
		}
		created_queues++;

		ret = virtio_mmio_set_virtqueue(dev, i, &data->virtqueues[i]);
		if (ret != 0) {
			goto fail;
		}
		activated_queues++;
	}

	return 0;

fail:
	for (int j = 0; j < activated_queues; j++) {
		virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_SEL, j);
		virtio_mmio_write32(dev, VIRTIO_MMIO_QUEUE_READY, 0);
	}
	for (int j = 0; j < created_queues; j++) {
		virtq_free(&data->virtqueues[j]);
	}
	k_free(data->virtqueues);
	data->virtqueue_count = 0;

	return ret;
}

static int virtio_mmio_init_virtqueues(const struct device *dev, uint16_t num_queues,
				       virtio_enumerate_queues cb, void *opaque)
{
	return virtio_mmio_set_virtqueues(dev, num_queues, cb, opaque);
}

static void virtio_mmio_finalize_init(const struct device *dev)
{
	virtio_mmio_write_status_bit(dev, DEVICE_STATUS_DRIVER_OK);
}

static DEVICE_API(virtio, virtio_mmio_driver_api) = {
	.get_virtqueue = virtio_mmio_get_virtqueue,
	.notify_virtqueue = virtio_mmio_notify_queue,
	.get_device_specific_config = virtio_mmio_get_device_specific_config,
	.read_device_feature_bit = virtio_mmio_read_device_feature_bit,
	.write_driver_feature_bit = virtio_mmio_write_driver_feature_bit_range_check,
	.commit_feature_bits = virtio_mmio_commit_feature_bits,
	.init_virtqueues = virtio_mmio_init_virtqueues,
	.finalize_init = virtio_mmio_finalize_init,
};

static int virtio_mmio_init_common(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);

	const uint32_t magic = virtio_mmio_read32(dev, VIRTIO_MMIO_MAGIC_VALUE);

	if (magic != VIRTIO_MMIO_MAGIC) {
		LOG_ERR("Invalid magic value %x", magic);
		return -EINVAL;
	}

	const uint32_t version = virtio_mmio_read32(dev, VIRTIO_MMIO_VERSION);

	if (version != VIRTIO_MMIO_SUPPORTED_VERSION) {
		LOG_ERR("Invalid version %x", version);
		return -EINVAL;
	}

	const uint32_t dev_id = virtio_mmio_read32(dev, VIRTIO_MMIO_DEVICE_ID);

	if (dev_id == VIRTIO_MMIO_INVALID_DEVICE_ID) {
		LOG_ERR("Invalid device id %x", dev_id);
		return -EINVAL;
	}

	LOG_DBG("VENDOR_ID = %x", virtio_mmio_read32(dev, VIRTIO_MMIO_VENDOR_ID));

	virtio_mmio_reset(dev);

	virtio_mmio_write_status_bit(dev, DEVICE_STATUS_ACKNOWLEDGE);
	virtio_mmio_write_status_bit(dev, DEVICE_STATUS_DRIVER);

	virtio_mmio_write_driver_feature_bit(dev, VIRTIO_F_VERSION_1, true);

	return 0;
};

#define VIRTIO_MMIO_DEFINE(inst)                                                                   \
	static struct virtio_mmio_data virtio_mmio_data##inst;                                     \
	static struct virtio_mmio_config virtio_mmio_config##inst = {                              \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(inst)),                           \
	};                                                                                         \
	static int virtio_mmio_init##inst(const struct device *dev)                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), virtio_mmio_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		int ret = virtio_mmio_init_common(dev);                                            \
		irq_enable(DT_INST_IRQN(inst));                                                    \
		return ret;                                                                        \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, virtio_mmio_init##inst, NULL, &virtio_mmio_data##inst,         \
			      &virtio_mmio_config##inst, POST_KERNEL, 0, &virtio_mmio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_MMIO_DEFINE)
