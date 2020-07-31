/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/ipm.h>

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_hci_driver_nrf53
#include "common/log.h"

void bt_rpmsg_rx(uint8_t *data, size_t len);

static K_SEM_DEFINE(ready_sem, 0, 1);
static K_SEM_DEFINE(rx_sem, 0, 1);

static K_KERNEL_STACK_DEFINE(bt_rpmsg_rx_thread_stack,
			     CONFIG_BT_RPMSG_NRF53_RX_STACK_SIZE);
static struct k_thread bt_rpmsg_rx_thread_data;

static struct device *ipm_tx_handle;
static struct device *ipm_rx_handle;

/* Configuration defines */

#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)
#define SHM_BASE_ADDRESS    DT_REG_ADDR(SHM_NODE)

#define SHM_START_ADDR      (SHM_BASE_ADDRESS + 0x400)
#define SHM_SIZE            0x7c00
#define SHM_DEVICE_NAME     "sram0.shm"

BUILD_ASSERT((SHM_START_ADDR + SHM_SIZE - SHM_BASE_ADDRESS)
		<= DT_REG_SIZE(SHM_NODE),
	"Allocated size exceeds available shared memory reserved for IPC");

#define VRING_COUNT         2
#define VRING_TX_ADDRESS    (SHM_START_ADDR + SHM_SIZE - 0x400)
#define VRING_RX_ADDRESS    (VRING_TX_ADDRESS - 0x400)
#define VRING_ALIGNMENT     4
#define VRING_SIZE          16

#define VDEV_STATUS_ADDR    SHM_BASE_ADDRESS

BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE >= 1024,
	"Not enough heap memory for RPMsg queue allocation");

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

static void virtio_set_features(struct virtio_device *vdev, uint32_t features)
{
	/* No need for implementation */
}

static void virtio_notify(struct virtqueue *vq)
{
	int status;

	status = ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
	if (status != 0) {
		BT_ERR("ipm_send failed to notify: %d", status);
	}
}

const struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.set_features = virtio_set_features,
	.notify = virtio_notify,
};

static void ipm_callback(struct device *dev, void *context,
			 uint32_t id, volatile void *data)
{
	BT_DBG("Got callback of id %u", id);
	k_sem_give(&rx_sem);
}

static int endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
	uint32_t src, void *priv)
{
	BT_DBG("Received message of %u bytes.", len);
	BT_HEXDUMP_DBG((uint8_t *)data, len, "Data:");

	bt_rpmsg_rx(data, len);

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ep)
{
	rpmsg_destroy_ept(ep);
}

static void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	(void)rpmsg_create_ept(&ep,
				rdev,
				name,
				RPMSG_ADDR_ANY,
				dest,
				endpoint_cb,
				rpmsg_service_unbind);

	k_sem_give(&ready_sem);
}

static void bt_rpmsg_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		int status = k_sem_take(&rx_sem, K_FOREVER);

		if (status == 0) {
			virtqueue_notification(vq[0]);
		}
	}
}

int bt_rpmsg_platform_init(void)
{
	int err;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	static struct virtio_vring_info     rvrings[2];
	static struct rpmsg_virtio_shm_pool shpool;
	static struct virtio_device         vdev;
	static struct rpmsg_virtio_device   rvdev;
	static struct metal_io_region       *io;
	static struct metal_device          *device;

	/* Setup thread for RX data processing. */
	k_thread_create(&bt_rpmsg_rx_thread_data, bt_rpmsg_rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(bt_rpmsg_rx_thread_stack),
			bt_rpmsg_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RPMSG_NRF53_RX_PRIO),
			0, K_NO_WAIT);

	/* Libmetal setup */
	err = metal_init(&metal_params);
	if (err) {
		BT_ERR("metal_init: failed - error code %d", err);
		return err;
	}

	err = metal_register_generic_device(&shm_device);
	if (err) {
		BT_ERR("Couldn't register shared memory device: %d", err);
		return err;
	}

	err = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (err) {
		BT_ERR("metal_device_open failed: %d", err);
		return err;
	}

	io = metal_device_io_region(device, 0);
	if (!io) {
		BT_ERR("metal_device_io_region failed to get region");
		return -ENODEV;
	}

	/* IPM setup */
	ipm_tx_handle = device_get_binding("IPM_0");
	if (!ipm_tx_handle) {
		BT_ERR("Could not get TX IPM device handle");
		return -ENODEV;
	}

	ipm_rx_handle = device_get_binding("IPM_1");
	if (!ipm_rx_handle) {
		BT_ERR("Could not get RX IPM device handle");
		return -ENODEV;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, NULL);

	/* Virtqueue setup */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (!vq[0]) {
		BT_ERR("virtqueue_allocate failed to alloc vq[0]");
		return -ENOMEM;
	}

	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (!vq[1]) {
		BT_ERR("virtqueue_allocate failed to alloc vq[1]");
		return -ENOMEM;
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
	err = rpmsg_init_vdev(&rvdev, &vdev, ns_bind_cb, io, &shpool);
	if (err) {
		BT_ERR("rpmsg_init_vdev failed %d", err);
		return err;
	}

	/* Wait til nameservice ep is setup */
	k_sem_take(&ready_sem, K_FOREVER);

	return 0;
}

int bt_rpmsg_platform_send(struct net_buf *buf)
{
	return rpmsg_send(&ep, buf->data, buf->len);
}
