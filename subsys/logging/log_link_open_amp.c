/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <ipm.h>
#include <sys/byteorder.h>
#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>
#include <logging/log_link.h>
#include <logging/log.h>
#include "log_link_open_amp.h"

LOG_MODULE_REGISTER(log_link_open_amp, 2);

static K_SEM_DEFINE(sync_sem, 0, 1);

static struct device *ipm_tx_handle;
static struct device *ipm_rx_handle;

/* Configuration defines */

#define SHM_START_ADDR      (DT_IPC_SHM_BASE_ADDRESS + 0x400)
#define SHM_SIZE            0x7c00
#define SHM_DEVICE_NAME     "sram0.shm"

#define VRING_COUNT         2
#define VRING_TX_ADDRESS    (SHM_START_ADDR + SHM_SIZE - 0x400)
#define VRING_RX_ADDRESS    (VRING_TX_ADDRESS - 0x400)
#define VRING_ALIGNMENT     4
#define VRING_SIZE          16

#define VDEV_STATUS_ADDR    DT_IPC_SHM_BASE_ADDRESS

#define IPC_TX_CHANNEL      0 // Channel used for application to notify network
#define IPC_RX_CHANNEL      1 // Channel used for network to notify application

/* End of configuration defines */

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };
static struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.bus = NULL,
	.num_regions = 1,
	.regions = {
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

static struct virtqueue *vq[2];
static struct rpmsg_endpoint ep;

log_link_open_amp_clbk_t rx_clbk;

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return VIRTIO_CONFIG_STATUS_DRIVER_OK;
}

static void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, VDEV_STATUS_ADDR);
}

static u32_t virtio_get_features(struct virtio_device *vdev)
{
	return 1 << VIRTIO_RPMSG_F_NS;
}

static void virtio_set_features(struct virtio_device *vdev, u32_t features)
{
	/* No need for implementation */
}

static void virtio_notify(struct virtqueue *vq)
{
	int status = ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
	if (status != 0) {
		LOG_ERR("ipm_send failed to notify: %d", status);
	}
}

const struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.set_features = virtio_set_features,
	.notify = virtio_notify,
};

static void ipm_callback(void * context, u32_t id, volatile void * data)
{
	LOG_DBG("Got callback of id %u", id);
	virtqueue_notification(vq[0]);
}

static int endpoint_cb(struct rpmsg_endpoint * ept,
                void *                  data,
                size_t                  len,
                u32_t                   src,
                void *                  priv)
{
	LOG_DBG("Received message of %u bytes.", len);
	LOG_HEXDUMP_DBG((uint8_t *)data, len, "Data:");

	rx_clbk(data, len);

	return RPMSG_SUCCESS;
}

void log_link_open_amp_send(u8_t *buf, size_t len)
{
	rpmsg_send(&ep, buf, len);
	LOG_INF("send done");
}

static void rpmsg_service_unbind(struct rpmsg_endpoint * ep)
{
	rpmsg_destroy_ept(ep);
}

void ns_bind_cb(struct rpmsg_device * rdev, const char * name, u32_t dest)
{
	LOG_INF("bind cb");
	(void)rpmsg_create_ept(&ep,
				rdev,
				name,
				RPMSG_ADDR_ANY,
				dest,
				endpoint_cb,
				rpmsg_service_unbind);

	k_sem_give(&sync_sem);
}

static int open_amp_init_internal(void)
{
	int status;

	static struct virtio_vring_info     rvrings[2];
	static struct rpmsg_virtio_shm_pool shpool;
	static struct virtio_device         vdev;
	static struct rpmsg_virtio_device   rvdev;
	static struct metal_io_region *     io;
	static struct metal_device *        device;

	/* Libmetal setup */
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	status = metal_init(&metal_params);
	if (status != 0) {
		LOG_ERR("metal_init: failed - error code %d", status);
		return status;
	}

	status = metal_register_generic_device(&shm_device);
	if (status != 0)
	{
		LOG_ERR("Couldn't register shared memory device: %d", status);
		return status;
	}

	status = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (status != 0) {
		LOG_ERR("metal_device_open failed: %d", status);
		return status;
	}

	io = metal_device_io_region(device, 0);
	if (io == NULL) {
		LOG_ERR("metal_device_io_region failed to get region");
		return status;
	}

	/* IPM setup */
	ipm_tx_handle = device_get_binding("IPM_1");
	if (!ipm_tx_handle) {
		LOG_ERR("Could not get TX IPM device handle");
		return -ENODEV;
	}

	ipm_rx_handle = device_get_binding("IPM_0");
	if (!ipm_rx_handle) {
		LOG_ERR("Could not get RX IPM device handle");
		return -ENODEV;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, NULL);

	/* Virtqueue setup */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (vq[0] == NULL) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[0]");
		return status;
	}

	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (vq[1] == NULL) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[1]");
		return status;
	}

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

	vdev.role = RPMSG_MASTER;
	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	vdev.vrings_info = &rvrings[0];

	rpmsg_virtio_init_shm_pool(&shpool, (void *)SHM_START_ADDR, SHM_SIZE);
	status = rpmsg_init_vdev(&rvdev, &vdev, ns_bind_cb, io, &shpool);
	if (status != 0) {
		LOG_ERR("rpmsg_init_vdev failed %d", status);
		return status;
	}

	/* Since we are using name service, we need to wait for a response
	* from NS setup and than we need to process it
	*/
	virtqueue_notification(vq[0]);

	/* Wait til nameservice ep is setup */
	LOG_DBG("Wait till nameservice ep is setup");
	//k_sleep(5);
	//return 0;
	k_sem_take(&sync_sem, K_FOREVER);

	return 0;
}

int log_link_open_amp_init(log_link_open_amp_clbk_t clbk)
{
	int err;

	rx_clbk = clbk;

	err = open_amp_init_internal();
	if (err) {
		return err;
	}

	LOG_DBG("err: %d", err);
	return 0;
}

