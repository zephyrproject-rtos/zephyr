/*
 * Copyright (c) 2024 Maurer Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openamp_rpmsg_virtio_device

#include <stdio.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/openamp.h>

#include <openamp/virtio.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_virtio.h>
#include <openamp/rpmsg_virtio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(openamp_driver, CONFIG_OPENAMP_DRIVER_LOG_LEVEL);

struct rvdev_config {
	struct mbox_dt_spec mbox_tx;
	struct mbox_dt_spec mbox_rx;
	const char *tx_mr_name;
	const char *rx_mr_name;
	const char *buffer_mr_name;
	unsigned int idx;
};

struct rvdev_data {
	struct virtio_device *vdev;
	struct rpmsg_virtio_device rvdev;
	struct k_thread thread;
	struct k_sem sem;
	k_timeout_t poll_time;
	struct metal_io_region mr_rsc_table;
	struct metal_io_region mr_vring_tx;
	struct metal_io_region mr_vring_rx;
	struct metal_io_region mr_buffer;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_RPMSG_VDEV_THREAD_STACK_SIZE);
};

static int rvdev_notify(void *priv, uint32_t id)
{
	const struct device *dev = priv;
	const struct rvdev_config *config = dev->config;

	return mbox_send_dt(&config->mbox_tx, NULL);
}

static void rvdev_reset_callback(struct virtio_device *vdev)
{
	LOG_INF("vdev_reset_callback");
}

static void rpmsg_ns_bind_callback(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	LOG_INF("rpmsg_ns_bind_callback");
}

static void mbox_callback(const struct device *dev, uint32_t channel, void *user_data,
			  struct mbox_msg *msg_data)
{
	struct k_sem *sem = user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(msg_data);
	k_sem_give(sem);
}

static void rvdev_rx_thread(void *dev_ptr, void *p2, void *p3)
{
	const struct device *dev = dev_ptr;
	struct rvdev_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		k_sem_take(&data->sem, data->poll_time);
		rproc_virtio_notified(data->vdev, RSC_NOTIFY_ID_ANY);
	}
}

static int rvdev_init(const struct device *dev)
{
	const struct rvdev_config *config = dev->config;
	struct rvdev_data *data = dev->data;
	void *rsc_table = openamp_get_rsc_table();
	size_t rsc_table_size = openamp_get_rsc_table_size();
	const struct fw_rsc_vdev *rsc_vdev = openamp_get_vdev(config->idx);
	const struct fw_rsc_carveout *rsc_co_vring_tx =
		openamp_get_carveout_by_name(config->tx_mr_name);
	const struct fw_rsc_carveout *rsc_co_vring_rx =
		openamp_get_carveout_by_name(config->rx_mr_name);
	const struct fw_rsc_carveout *rsc_co_buffer =
		openamp_get_carveout_by_name(config->buffer_mr_name);
	const struct fw_rsc_vdev_vring *rsc_vrings[2];

	int ret;
	k_tid_t tid;

	if (!rsc_table || !rsc_vdev || !rsc_co_vring_tx || !rsc_co_vring_rx || !rsc_co_buffer) {
		LOG_ERR("resource table incomplete");
		return -ENOENT;
	}

	if (rsc_vdev->num_of_vrings != 2) {
		LOG_ERR("invalid number of vrings in resource table %u", rsc_vdev->num_of_vrings);
		return -EBADE;
	}

	rsc_vrings[0] = &rsc_vdev->vring[0];

	rsc_vrings[1] = &rsc_vdev->vring[1];

	data->poll_time = K_MSEC(10);

	ret = k_sem_init(&data->sem, 0, 1);

	if (ret) {
		LOG_ERR("k_sem_init failed %d", ret);
		return ret;
	}

	ret = mbox_register_callback_dt(&config->mbox_rx, mbox_callback, &data->sem);

	if (ret) {
		LOG_ERR("mbox_register_callback_dt failed %d", ret);
		return ret;
	}

	ret = mbox_set_enabled_dt(&config->mbox_rx, true);

	if (ret) {
		LOG_ERR("mbox_set_enabled_dt failed %d", ret);
		return ret;
	}

	metal_io_init(&data->mr_rsc_table, rsc_table, (metal_phys_addr_t *)rsc_table,
		      rsc_table_size, -1, 0, NULL);

	metal_io_init(&data->mr_vring_tx, (void *)rsc_co_vring_tx->da,
		      (metal_phys_addr_t *)(void *)&rsc_co_vring_tx->pa, rsc_co_vring_tx->len, -1,
		      rsc_co_vring_tx->flags, NULL);

	metal_io_init(&data->mr_vring_rx, (void *)rsc_co_vring_rx->da,
		      (metal_phys_addr_t *)(void *)&rsc_co_vring_rx->pa, rsc_co_vring_rx->len, -1,
		      rsc_co_vring_rx->flags, NULL);

	metal_io_init(&data->mr_buffer, (void *)rsc_co_buffer->da,
		      (metal_phys_addr_t *)(void *)&rsc_co_buffer->pa, rsc_co_buffer->len, -1,
		      rsc_co_buffer->flags, NULL);

	data->vdev = rproc_virtio_create_vdev(VIRTIO_DEV_DEVICE, 0xFF, (void *)rsc_vdev,
					      &data->mr_rsc_table, (void *)dev, rvdev_notify,
					      rvdev_reset_callback);

	if (!data->vdev) {
		LOG_ERR("rproc_virtio_create_vdev failed");
		return -1;
	}

	ret = rproc_virtio_init_vring(data->vdev, 0, rsc_vrings[0]->notifyid,
				      (void *)rsc_vrings[0]->da, &data->mr_vring_tx,
				      rsc_vrings[0]->num, rsc_vrings[0]->align);
	if (ret) {
		LOG_ERR("failed to init vring 0");
		return ret;
	}

	ret = rproc_virtio_init_vring(data->vdev, 1, rsc_vrings[1]->notifyid,
				      (void *)rsc_vrings[1]->da, &data->mr_vring_rx,
				      rsc_vrings[1]->num, rsc_vrings[1]->align);
	if (ret) {
		LOG_ERR("failed to init vring 0");
		return ret;
	}

	rproc_virtio_wait_remote_ready(data->vdev);

	ret = rpmsg_init_vdev(&data->rvdev, data->vdev, rpmsg_ns_bind_callback, &data->mr_buffer,
			      NULL);

	if (ret) {
		LOG_ERR("rpmsg_init_vdev failed\r\n");
		return ret;
	}

	tid = k_thread_create(&data->thread, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
			      rvdev_rx_thread, (void *)dev, NULL, NULL,
			      CONFIG_RPMSG_VDEV_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, "rvdev");

	return 0;
}

struct rpmsg_device *openamp_get_rpmsg_device(const struct device *dev)
{
	struct rvdev_data *data = dev->data;

	return rpmsg_virtio_get_rpmsg_device(&data->rvdev);
}

#define VDEV_VRING_BY_NAME(inst, name) DT_INST_PHANDLE_BY_NAME(inst, vrings, name)
#define VDEV_VRING_MEMORY_REGION_BY_NAME(inst, name)                                               \
	DT_PROP(VDEV_VRING_BY_NAME(inst, name), memory_region)

#define VDEV_DT_MBOX_SPEC_BY_NAME(inst, name)                                                      \
	{                                                                                          \
		.dev = DEVICE_DT_GET(                                                              \
			DT_PHANDLE_BY_IDX(VDEV_VRING_BY_NAME(inst, name), mboxes, 0)),             \
		.channel_id = DT_PHA_BY_IDX(VDEV_VRING_BY_NAME(inst, name), mboxes, 0, channel),   \
	}

#define DEFINE_RPMSG_VIRTIO_DEVICE(i)                                                              \
	static const struct rvdev_config rvdev_config_##i = {                                      \
		.idx = i,                                                                          \
		.mbox_tx = VDEV_DT_MBOX_SPEC_BY_NAME(i, tx),                                       \
		.mbox_rx = VDEV_DT_MBOX_SPEC_BY_NAME(i, rx),                                       \
		.tx_mr_name =                                                                      \
			DT_PROP(VDEV_VRING_MEMORY_REGION_BY_NAME(i, tx), zephyr_memory_region),    \
		.rx_mr_name =                                                                      \
			DT_PROP(VDEV_VRING_MEMORY_REGION_BY_NAME(i, rx), zephyr_memory_region),    \
		.buffer_mr_name = DT_PROP(DT_INST_PROP(i, memory_region), zephyr_memory_region),   \
	};                                                                                         \
	static struct rvdev_data rvdev_data_##i = {};                                              \
	DEVICE_DT_INST_DEFINE(i, &rvdev_init, NULL, &rvdev_data_##i, &rvdev_config_##i,            \
			      POST_KERNEL, CONFIG_RPMSG_VDEV_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_RPMSG_VIRTIO_DEVICE)
