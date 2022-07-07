/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openamp/open_amp.h>
#include <metal/device.h>

#include "common.h"

#define READ_BUFFER		(1)

#define APP_TASK_STACK_SIZE	(1024)

static const struct device *ipm_tx_handle;
static const struct device *ipm_rx_handle;

static struct virtqueue *vq[2];

static volatile struct payload *received_data;

#if READ_BUFFER
static volatile uint8_t *buffer;
static int max_payload_size;
#endif

static K_SEM_DEFINE(data_sem, 0, 1);

static struct rpmsg_endpoint ep;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };
static struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.bus = NULL,
	.num_regions = 1,
	{
		{
			.virt       = (void *) SHM_START_ADDR,
			.physmap    = shm_physmap,
			.size       = SHM_SIZE,
			.page_shift = 0xffffffff,
			.page_mask  = 0xffffffff,
			.mem_flags  = 0,
			.ops        = { NULL },
		},
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

static struct virtio_vring_info rvrings[2] = {
	[0] = {
		.info.align = VRING_ALIGNMENT,
	},
	[1] = {
		.info.align = VRING_ALIGNMENT,
	},
};

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return sys_read8(VDEV_STATUS_ADDR);
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

static void virtio_notify(struct virtqueue *vq)
{
	ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
}

static struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.get_features = virtio_get_features,
	.notify = virtio_notify,
};

static void platform_ipm_callback(const struct device *dev, void *context,
				  uint32_t id, volatile void *data)
{
	k_sem_give(&data_sem);
}

static int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		       size_t len, uint32_t src, void *priv)
{
	received_data = ((struct payload *) data);

	/* We do some work here */

#if READ_BUFFER
	memcpy((void *) buffer, (void *) received_data->data, received_data->size);
#endif

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;

	rpmsg_destroy_ept(&ep);
}

static void receive_message(void)
{
	k_sem_take(&data_sem, K_FOREVER);

	virtqueue_notification(vq[1]);
}

static void receive_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	struct rpmsg_virtio_device rvdev;
	struct metal_device *device;
	struct metal_io_region *io;
	struct rpmsg_device *rdev;
	struct virtio_device vdev;
	int status = 0;

	printk("\r\nOpenAMP[remote] demo started\r\n");

	status = metal_init(&metal_params);
	if (status != 0) {
		printk("metal_init: failed - error code %d\n", status);
		return;
	}

	status = metal_register_generic_device(&shm_device);
	if (status != 0) {
		printk("Couldn't register shared memory device: %d\n", status);
		return;
	}

	status = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (status != 0) {
		printk("metal_device_open failed: %d\n", status);
		return;
	}

	io = metal_device_io_region(device, 0);
	if (io == NULL) {
		printk("metal_device_io_region failed to get region\n");
		return;
	}

	/* setup IPM */
	ipm_tx_handle = device_get_binding(CONFIG_OPENAMP_IPC_DEV_TX_NAME);
	if (ipm_tx_handle == NULL) {
		printk("device_get_binding failed to find device\n");
		return;
	}

	ipm_rx_handle = device_get_binding(CONFIG_OPENAMP_IPC_DEV_RX_NAME);
	if (ipm_rx_handle == NULL) {
		printk("device_get_binding failed to find device\n");
		return;
	}

	ipm_register_callback(ipm_rx_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_rx_handle, 1);
	if (status != 0) {
		printk("ipm_set_enabled failed\n");
		return;
	}

	/* setup vdev */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (vq[0] == NULL) {
		printk("virtqueue_allocate failed to alloc vq[0]\n");
		return;
	}
	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (vq[1] == NULL) {
		printk("virtqueue_allocate failed to alloc vq[1]\n");
		return;
	}

	vdev.role = RPMSG_REMOTE;
	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	rvrings[0].io = io;
	rvrings[0].info.vaddr = (void *)VRING_TX_ADDRESS;
	rvrings[0].info.num_descs = VRING_SIZE;
	rvrings[0].info.align = VRING_ALIGNMENT;
	rvrings[0].vq = vq[0];

	rvrings[1].io = io;
	rvrings[1].info.vaddr = (void *)VRING_RX_ADDRESS;
	rvrings[1].info.num_descs = VRING_SIZE;
	rvrings[1].info.align = VRING_ALIGNMENT;
	rvrings[1].vq = vq[1];

	vdev.vrings_info = &rvrings[0];

	/* setup rvdev */
	status = rpmsg_init_vdev(&rvdev, &vdev, NULL, io, NULL);
	if (status != 0) {
		printk("rpmsg_init_vdev failed %d\n", status);
		return;
	}

	rdev = rpmsg_virtio_get_rpmsg_device(&rvdev);

	status = rpmsg_create_ept(&ep, rdev, "k", RPMSG_ADDR_ANY,
			RPMSG_ADDR_ANY, endpoint_cb, rpmsg_service_unbind);
	if (status != 0) {
		printk("rpmsg_create_ept failed %d\n", status);
		return;
	}

#if READ_BUFFER
	max_payload_size = rpmsg_virtio_get_buffer_size(rdev);
	if (max_payload_size < 0) {
		printk("No available buffer size\n");
		return;
	}

	buffer = (uint8_t *) metal_allocate_memory(max_payload_size - sizeof(struct payload));
	if (!buffer) {
		printk("memory allocation failed\n");
		return;
	}
#endif

	while (1) {
		receive_message();
	}

	/* How did we get here? */
}
K_THREAD_DEFINE(thread_receive_id, APP_TASK_STACK_SIZE, receive_task, NULL, NULL, NULL,
		K_PRIO_PREEMPT(1), 0, 0);
