/*
 * Copyright (c) 2018, NXP
 * Copyright (c) 2018, Nordic Semiconductor ASA
 * Copyright (c) 2018, Linaro Limited
 * Copyright (c) 2018, Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ipm.h>
#include <misc/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openamp/open_amp.h>
#include <metal/device.h>

#include "common.h"

#define APP_TASK_STACK_SIZE (1024)
K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

static int shutdown_req;
static unsigned int virtio_status;

static struct device *ipm_handle;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };
static struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.bus = NULL,
	.num_regions = 1,
	{
		{
			.virt	    = (void *) SHM_START_ADDR,
			.physmap    = shm_physmap,
			.size	    = SHM_SIZE,
			.page_shift = 0xffffffff,
			.page_mask  = 0xffffffff,
			.mem_flags  = 0,
			.ops	    = { NULL },
		},
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

static volatile unsigned int received_data;

static struct virtio_vring_info rvrings[2] = {
	[0] = {
		.info.align = VRING_ALIGNMENT,
	},
	[1] = {
		.info.align = VRING_ALIGNMENT,
	},
};

static struct virtio_device vdev;
static struct rpmsg_virtio_device rvdev;
static struct metal_io_region *io;
static struct virtqueue *vq[2];

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return virtio_status;
}

static u32_t virtio_get_features(struct virtio_device *vdev)
{
	return 1 << VIRTIO_RPMSG_F_NS;
}

static void virtio_set_features(struct virtio_device *vdev,
				u32_t features)
{
}

static void virtio_notify(struct virtqueue *vq)
{
	/* Aligned with the i.MX RPMsg Linux Driver where the bit 16 is
	 * used to notify the message direction.
	 */
	u32_t vq_id = ((vq->vq_queue_index)&0x1) << 16;

	ipm_send(ipm_handle, 0, RPMSG_MU_CHANNEL, &vq_id, sizeof(vq_id));
}

struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.get_features = virtio_get_features,
	.set_features = virtio_set_features,
	.notify = virtio_notify,
};

static K_SEM_DEFINE(data_sem, 0, 1);
static K_SEM_DEFINE(data_rx_sem, 0, 1);
static K_SEM_DEFINE(ept_sem, 0, 1);

struct rpmsg_endpoint my_ept;
struct rpmsg_endpoint *ep = &my_ept;

static void platform_ipm_callback(void *context, u32_t id, volatile void *data)
{
	/* After receiving the first ipm message from the Linux master we assume
	 * that the driver status is OK since the i.MX RPMsg Linux Driver
	 * doesn't implement a set_status callback function.
	 */
	if (!virtio_status) {
		virtio_status = VIRTIO_CONFIG_STATUS_DRIVER_OK;
	}

	k_sem_give(&data_sem);
}

int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, u32_t src, void *priv)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(src);
	char payload[RPMSG_BUFFER_SIZE];

	/* Send data back to master */
	memset(payload, 0, RPMSG_BUFFER_SIZE);
	memcpy(payload, data, len);

	printk("echo message: %s\n", payload);
	if (rpmsg_send(ept, (char *)data, len) < 0) {
		printk("rpmsg_send failed\n");
		goto destroy_ept;
	}

	k_sem_give(&data_rx_sem);

	return RPMSG_SUCCESS;

destroy_ept:
	shutdown_req = 1;
	k_sem_give(&data_rx_sem);
	return RPMSG_SUCCESS;
}


static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	ARG_UNUSED(ept);
	rpmsg_destroy_ept(ep);
}

static void loopback_messages(void)
{
	while (k_sem_take(&data_rx_sem, K_NO_WAIT) != 0) {
		int status = k_sem_take(&data_sem, K_FOREVER);

		if (status == 0) {
			virtqueue_notification(vq[1]);
		}
	}
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int status = 0;
	struct metal_device *device;
	struct rpmsg_device *rdev;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	shutdown_req = 0;
	virtio_status = 0;

	status = metal_init(&metal_params);
	if (status != 0) {
		printk("metal_init failed: %d\n", status);
		return;
	}

	status = metal_register_generic_device(&shm_device);
	if (status != 0) {
		printk("Couldn't register shared memory device: %d\n", status);
		metal_finish();
		return;
	}

	status = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (status != 0) {
		printk("metal_device_open failed: %d\n", status);
		metal_finish();
		return;
	}

	io = metal_device_io_region(device, 0);
	if (io == NULL) {
		printk("metal_device_io_region failed\n");
		metal_finish();
		return;
	}

	/* setup IPM */
	ipm_handle = device_get_binding(IPM_LABEL);
	if (ipm_handle == NULL) {
		printk("device_get_binding %s failed\n", IPM_LABEL);
		metal_finish();
		return;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_handle, 1);
	if (status != 0) {
		printk("ipm_set_enabled failed: %d\n", status);
		metal_finish();
		return;
	}

	/* setup vdev */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (vq[0] == NULL) {
		printk("vq[0] virtqueue_allocate size %d failed\n", VRING_SIZE);
		metal_finish();
		return;
	}
	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (vq[1] == NULL) {
		printk("vq[1] virtqueue_allocate size %d failed\n", VRING_SIZE);
		virtqueue_free(vq[0]);
		metal_finish();
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
		printk("rpmsg_init_vdev failed: %d\n", status);
		virtqueue_free(vq[0]);
		virtqueue_free(vq[1]);
		metal_finish();
		return;
	}
	rdev = rpmsg_virtio_get_rpmsg_device(&rvdev);

	status = rpmsg_create_ept(ep, rdev, RPMSG_CHAN_NAME, RPMSG_ADDR_ANY,
				  RPMSG_ADDR_ANY, endpoint_cb,
				  rpmsg_service_unbind);
	if (status != 0) {
		printk("rpmsg_create_ept failed: %d\n", status);
		goto _cleanup;
	}

	printk("\r\nOpenAMP remote echo demo started\r\n");

	while (1) {

		loopback_messages();

		if (shutdown_req) {
			printk("shutting down\n");
			goto _cleanup;
		}
	}

_cleanup:
	rpmsg_deinit_vdev(&rvdev);
	metal_finish();

	printk("OpenAMP remote echo demo ended.\n");
}

void main(void)
{
	printk("Starting application thread!\n");
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
