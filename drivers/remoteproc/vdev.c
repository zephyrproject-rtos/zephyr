/*
 * Copyright (c) 2024 Maurer Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	openamp_vdev

#include <stdio.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/remoteproc.h>
#include <zephyr/drivers/remoteproc/vdev.h>

#include <openamp/virtio.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_virtio.h>
#include <openamp/rpmsg_virtio.h>


#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(openamp_remoteproc, CONFIG_REMOTEPROC_LOG_LEVEL);

#define DT_DRV_COMPAT openamp_vdev

struct vdev_config {
	struct mbox_dt_spec mbox_tx;
	struct mbox_dt_spec mbox_rx;
	const char *vring0_name;
	const char *vring1_name;
	const char *buffer_name;
	unsigned int idx;
};

struct vdev_data {
	struct virtio_device *vdev;
	struct rpmsg_virtio_device rvdev;
	struct k_thread thread;
	struct k_sem sem;
	k_timeout_t poll_time;
	K_KERNEL_STACK_MEMBER(stack, CONFIG_REMOTEPROC_THREAD_STACK_SIZE);

};

static int rpvdev_notify(void *priv, uint32_t id)
{
	const struct device *dev = priv;
	const struct vdev_config *config = dev->config;

	return mbox_send_dt(&config->mbox_tx, NULL);
}


static void vdev_reset_callback(struct virtio_device *vdev)
{
	LOG_INF("vdev_reset_callback");
}

static void rpmsg_ns_bind_callback(struct rpmsg_device *rdev,
								 const char *name, uint32_t dest)
{
	LOG_INF("rpmsg_ns_bind_callback");
}


static void mbox_callback(const struct device *dev, uint32_t channel,
						  void *user_data, struct mbox_msg *msg_data)
{
	struct k_sem *sem =	user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(msg_data);
	k_sem_give(sem);
}

static void vdev_rx_thread(void *dev_ptr, void *p2, void *p3)
{
	const struct device *dev = dev_ptr;
	struct vdev_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);


	for (;;) {
		k_sem_take(&data->sem, data->poll_time);
		rproc_virtio_notified(data->vdev, RSC_NOTIFY_ID_ANY);
	}
}

static void dump_io_region(const char *name, const struct metal_io_region *region)
{
	LOG_INF("metal_io %s va=%p pa=%lx size=0x%x %lx", name, region->virt,
		  *region->physmap,  region->size, region->page_shift);
}

static int vdev_init(const struct device *dev)
{
	const struct vdev_config *config = dev->config;
	struct vdev_data *data = dev->data;
	struct fw_rsc_vdev *fw_vdev0 = remoteproc_get_vdev(config->idx);
	struct fw_rsc_vdev_vring *fw_vring0 = remoteproc_get_vring(config->idx, 0);
	struct fw_rsc_vdev_vring *fw_vring1 = remoteproc_get_vring(config->idx, 1);
	const struct fw_rsc_carveout *buffer_carveout;
	const struct fw_rsc_carveout *vring0_carveout;
	const struct fw_rsc_carveout *vring1_carveout;
	struct metal_io_region *rsc_table_io;
	struct metal_io_region *vring0_io;
	struct metal_io_region *vring1_io;
	struct metal_io_region *buffer_io;
	int ret;
	k_tid_t tid;

	LOG_INF("vdev_init");

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

	rsc_table_io = remoteproc_get_io_region();

	buffer_carveout = remoteproc_get_carveout_by_name(config->buffer_name);
	if (!buffer_carveout) {
		LOG_ERR("buffer_carveout not found %s", config->buffer_name);
		return -1;
	}

	buffer_io = remoteproc_get_carveout_io_region(buffer_carveout);
	if (!buffer_io) {
		LOG_ERR("buffer_io not found");
		return -1;
	}

	vring0_carveout = remoteproc_get_carveout_by_name(config->vring0_name);
	if (!vring0_carveout) {
		LOG_ERR("vring0_carveout not found");
		return -1;
	}

	vring0_io = remoteproc_get_carveout_io_region(vring0_carveout);
	if (!vring0_io) {
		LOG_ERR("vring0_io not found");
		return -1;
	}

	vring1_carveout = remoteproc_get_carveout_by_name(config->vring1_name);
	if (!vring1_carveout) {
		LOG_ERR("vring1_carveout not found");
		return -1;
	}

	vring1_io = remoteproc_get_carveout_io_region(vring1_carveout);
	if (!vring1_io) {
		LOG_ERR("vring1_io not found");
		return -1;
	}

	dump_io_region("buffer_io", buffer_io);
	dump_io_region("vring0_io", vring0_io);
	dump_io_region("vring1_io", vring1_io);
	data->vdev = rproc_virtio_create_vdev(VIRTIO_DEV_DEVICE,
								  VDEV_ID, fw_vdev0,
								  rsc_table_io, (void *)dev,
								  rpvdev_notify,
								  vdev_reset_callback);

	if (!data->vdev) {
		LOG_ERR("rproc_virtio_create_vdev failed");
		return -1;
	}

	ret = rproc_virtio_init_vring(data->vdev, 0, fw_vring0->notifyid,
								  (void *)fw_vring0->da, vring0_io,
								  fw_vring0->num, fw_vring0->align);
	if (ret) {
		LOG_ERR("failed to init vring 0");
		return ret;
	}

	ret = rproc_virtio_init_vring(data->vdev, 1, fw_vring1->notifyid,
								  (void *)fw_vring1->da, vring1_io,
								  fw_vring1->num, fw_vring1->align);
	if (ret) {
		LOG_ERR("failed to init vring 0");
		return ret;
	}

	rproc_virtio_wait_remote_ready(data->vdev);

	ret = rpmsg_init_vdev(&data->rvdev, data->vdev, rpmsg_ns_bind_callback, buffer_io, NULL);

	if (ret) {
		LOG_ERR("rpmsg_init_vdev failed\r\n");
		return ret;
	}

	tid = k_thread_create(&data->thread, data->stack,
						  K_THREAD_STACK_SIZEOF(data->stack),
						  vdev_rx_thread, (void *)dev, NULL,
						  NULL, CONFIG_REMOTEPROC_THREAD_PRIORITY,
						  0, K_NO_WAIT);

	k_thread_name_set(tid, "vdev");

	return 0;
}

struct rpmsg_device *vdev_get_rpmsg_device(const struct device *dev)
{
	struct vdev_data *data = dev->data;

	return rpmsg_virtio_get_rpmsg_device(&data->rvdev);
}

#define DEFINE_VIRTIO_DEVICE(i)  \
static const struct vdev_config vdev_config_##i = {  \
	.mbox_tx = MBOX_DT_SPEC_INST_GET(i, tx),  \
	.mbox_rx = MBOX_DT_SPEC_INST_GET(i, rx),  \
	.vring0_name = DT_INST_PROP_BY_PHANDLE(i, vring0_io, zephyr_memory_region),  \
	.vring1_name = DT_INST_PROP_BY_PHANDLE(i, vring1_io, zephyr_memory_region),  \
	.buffer_name = DT_INST_PROP_BY_PHANDLE(i, buffer_io, zephyr_memory_region),  \
	.idx = i,  \
	};  \
	static struct vdev_data vdev_data_##i = { };  \
	DEVICE_DT_INST_DEFINE(i,  \
	&vdev_init,  \
	NULL,  \
	&vdev_data_##i,  \
	&vdev_config_##i,  \
	POST_KERNEL,  \
	CONFIG_REMOTEPROC_INIT_PRIORITY,  \
	NULL);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_VIRTIO_DEVICE)
