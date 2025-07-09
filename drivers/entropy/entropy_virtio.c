/**
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/logging/log.h>

#define VIRTIO_ENTROPY_QUEUE_IDX 0

#define DT_DRV_COMPAT virtio_device4

LOG_MODULE_REGISTER(virtio_entropy, CONFIG_ENTROPY_LOG_LEVEL);

struct entropy_virtio_config {
	const struct device *vdev;
};

struct entropy_virtio_data {
	struct k_sem sem;
	uint32_t received_len;
};

static void entropy_virtio_virtq_recv_cb(void *priv, uint32_t len)
{
	struct entropy_virtio_data *data = priv;

	data->received_len = len;
	k_sem_give(&data->sem);
}

static uint16_t entropy_virtio_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *unused)
{
	if (q_index == VIRTIO_ENTROPY_QUEUE_IDX) {
		return MIN(1, q_size_max);
	}

	return 0;
}

static int entropy_virtio_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	struct virtq_buf buf[] = {{.addr = buffer, .len = length}};
	const struct entropy_virtio_config *cfg = dev->config;
	struct entropy_virtio_data *data = dev->data;
	struct virtq *vq = virtio_get_virtqueue(cfg->vdev, VIRTIO_ENTROPY_QUEUE_IDX);
	int ret;

	if (!vq) {
		LOG_ERR("failed to get virtqueue %d", VIRTIO_ENTROPY_QUEUE_IDX);
		return -ENODEV;
	}

	data->received_len = 0;
	ret = virtq_add_buffer_chain(vq, buf, 1, 0, entropy_virtio_virtq_recv_cb, data, K_FOREVER);
	if (ret) {
		LOG_ERR("virtq_add_buffer_chain failed: %d", ret);
		return -EIO;
	}

	virtio_notify_virtqueue(cfg->vdev, VIRTIO_ENTROPY_QUEUE_IDX);

	k_sem_take(&data->sem, K_FOREVER);

	if (data->received_len != length) {
		LOG_ERR("insufficient number of values: %d/%d", data->received_len, length);
		return -EIO;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_virtio_api) = {
	.get_entropy = entropy_virtio_get_entropy,
};

static int entropy_virtio_init(const struct device *dev)
{
	const struct entropy_virtio_config *cfg = dev->config;
	struct entropy_virtio_data *data = dev->data;
	int ret;

	ret = virtio_commit_feature_bits(cfg->vdev);
	if (ret != 0) {
		return ret;
	}

	ret = virtio_init_virtqueues(cfg->vdev, 1, entropy_virtio_enum_queues_cb, NULL);
	if (ret) {
		LOG_ERR("virtio_init_virtqueues failed: %d", ret);
		return ret;
	}

	virtio_finalize_init(cfg->vdev);

	k_sem_init(&data->sem, 0, 1);

	LOG_DBG("virtio entropy driver initialized");
	return 0;
}

#define ENTROPY_VIRTIO_INST(n)                                                                     \
	static struct entropy_virtio_data entropy_virtio_data_##n;                                 \
	static const struct entropy_virtio_config entropy_virtio_config_##n = {                    \
		.vdev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, entropy_virtio_init, NULL, &entropy_virtio_data_##n,              \
			      &entropy_virtio_config_##n, POST_KERNEL,                             \
			      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_virtio_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_VIRTIO_INST);
