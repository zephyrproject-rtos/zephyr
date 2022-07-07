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
#include <zephyr/init.h>

#include <openamp/open_amp.h>
#include <metal/device.h>

#include "common.h"

static struct payload *s_payload;

#define APP_TASK_STACK_SIZE (1024)

static const struct device *ipm_tx_handle;
static const struct device *ipm_rx_handle;

static int max_payload_size;

static K_SEM_DEFINE(data_sem, 0, 1);
static K_SEM_DEFINE(ept_sem, 0, 1);

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
	return VIRTIO_CONFIG_STATUS_DRIVER_OK;
}

static void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, VDEV_STATUS_ADDR);
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

static void virtio_set_features(struct virtio_device *vdev,
				uint32_t features)
{
}

static void virtio_notify(struct virtqueue *vq)
{
	ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
}

static struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.set_features = virtio_set_features,
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
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;

	rpmsg_destroy_ept(&ep);
}

static void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	(void)rpmsg_create_ept(&ep, rdev, name,
			RPMSG_ADDR_ANY, dest,
			endpoint_cb,
			rpmsg_service_unbind);

	k_sem_give(&ept_sem);
}

static void send_pkt(struct rpmsg_virtio_device *rvdev)
{
	struct rpmsg_device *rpdev;
	int status;

	rpdev = rpmsg_virtio_get_rpmsg_device(rvdev);
	max_payload_size = rpmsg_virtio_get_buffer_size(rpdev);
	if (max_payload_size < 0) {
		printk("No available buffer size\n");
		return;
	}

	s_payload = (struct payload *) metal_allocate_memory(max_payload_size);
	if (!s_payload) {
		printk("memory allocation failed\n");
		return;
	}

	memset(s_payload->data, 0xA5, max_payload_size - sizeof(struct payload));

	s_payload->size = max_payload_size;
	s_payload->cnt = 0;

	while (1) {
		status = rpmsg_send(&ep, s_payload, max_payload_size);
		if (status < 0) {
			printk("%s failed with status %d\n", __func__, status);
			return;
		}

		s_payload->cnt++;
	}
}


static void send_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	struct rpmsg_virtio_shm_pool shpool;
	struct rpmsg_virtio_device rvdev;
	struct metal_device *device;
	struct metal_io_region *io;
	struct virtio_device vdev;
	struct virtqueue *vq[2];
	int status = 0;

	printk("\r\nOpenAMP[master] demo started\r\n");

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

	vdev.role = RPMSG_HOST;
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
	rpmsg_virtio_init_shm_pool(&shpool, (void *)SHM_START_ADDR, SHM_SIZE);
	status = rpmsg_init_vdev(&rvdev, &vdev, ns_bind_cb, io, &shpool);
	if (status != 0) {
		printk("rpmsg_init_vdev failed %d\n", status);
		return;
	}

	/* Since we are using name service, we need to wait for a response
	 * from NS setup and than we need to process it
	 */
	k_sem_take(&data_sem, K_FOREVER);
	virtqueue_notification(vq[0]);

	/* Wait til nameservice ep is setup */
	k_sem_take(&ept_sem, K_FOREVER);

	/* Start the flood */
	send_pkt(&rvdev);

	/* We should get here only on error */
	rpmsg_deinit_vdev(&rvdev);
	metal_finish();

	printk("OpenAMP demo ended.\n");
}
K_THREAD_DEFINE(thread_send_id, APP_TASK_STACK_SIZE, send_task, NULL, NULL, NULL,
		K_PRIO_PREEMPT(2), 0, 0);

static void check_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	unsigned long last_cnt = s_payload->cnt;
	unsigned long delta;

	while (1) {
		k_sleep(K_SECONDS(1));

		delta = s_payload->cnt - last_cnt;

		printk("Δpkt: %ld (%ld B/pkt) | throughput: %ld bit/s\n",
			delta, s_payload->size, delta * max_payload_size * 8);

		last_cnt = s_payload->cnt;
	}
}
K_THREAD_DEFINE(thread_check_id, APP_TASK_STACK_SIZE, check_task, NULL, NULL, NULL,
		K_PRIO_PREEMPT(1), 0, 1000);


/* Make sure we clear out the status flag very early (before we bringup the
 * secondary core) so the secondary core see's the proper status
 */
static int init_status_flag(const struct device *arg)
{
	virtio_set_status(NULL, 0);

	return 0;
}
SYS_INIT(init_status_flag, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
