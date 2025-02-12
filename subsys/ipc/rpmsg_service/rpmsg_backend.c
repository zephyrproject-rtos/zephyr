/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rpmsg_backend.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <openamp/open_amp.h>
#include <metal/device.h>

#define LOG_MODULE_NAME rpmsg_backend
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_RPMSG_SERVICE_LOG_LEVEL);

/* Configuration defines */
#if !DT_HAS_CHOSEN(zephyr_ipc_shm)
#error "Module requires definition of shared memory for rpmsg"
#endif

#define MASTER IS_ENABLED(CONFIG_RPMSG_SERVICE_MODE_MASTER)

#if MASTER
#define VIRTQUEUE_ID 0
#define RPMSG_ROLE RPMSG_HOST
#else
#define VIRTQUEUE_ID 1
#define RPMSG_ROLE RPMSG_REMOTE
#endif

/* Configuration defines */

#define VRING_COUNT		    2
#define VRING_RX_ADDRESS	(VDEV_START_ADDR + SHM_SIZE - VDEV_STATUS_SIZE)
#define VRING_TX_ADDRESS	(VDEV_START_ADDR + SHM_SIZE)
#define VRING_ALIGNMENT		4
#define VRING_SIZE		    16

#define IPM_WORK_QUEUE_STACK_SIZE CONFIG_RPMSG_SERVICE_WORK_QUEUE_STACK_SIZE
#define IPM_WORK_QUEUE_PRIORITY   K_HIGHEST_APPLICATION_THREAD_PRIO

K_THREAD_STACK_DEFINE(ipm_stack_area, IPM_WORK_QUEUE_STACK_SIZE);

struct k_work_q ipm_work_q;

/* End of configuration defines */

#if defined(CONFIG_RPMSG_SERVICE_DUAL_IPM_SUPPORT)
static const struct device *const ipm_tx_handle =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc_tx));
static const struct device *const ipm_rx_handle =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc_rx));
#elif defined(CONFIG_RPMSG_SERVICE_SINGLE_IPM_SUPPORT)
static const struct device *const ipm_handle =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));
#endif

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
static struct virtqueue *vqueue[2];

static struct k_work ipm_work;

static unsigned char ipc_virtio_get_status(struct virtio_device *vdev)
{
#if MASTER
	return VIRTIO_CONFIG_STATUS_DRIVER_OK;
#else
	return sys_read8(VDEV_STATUS_ADDR);
#endif
}

static void ipc_virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, VDEV_STATUS_ADDR);
}

static uint32_t ipc_virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

static void ipc_virtio_set_features(struct virtio_device *vdev, uint32_t features)
{
}

static void ipc_virtio_notify(struct virtqueue *vq)
{
	int status;

#if defined(CONFIG_RPMSG_SERVICE_DUAL_IPM_SUPPORT)
	status = ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
#elif defined(CONFIG_RPMSG_SERVICE_SINGLE_IPM_SUPPORT)

#if defined(CONFIG_SOC_MPS2_AN521) || \
	defined(CONFIG_SOC_V2M_MUSCA_B1)
	uint32_t current_core = sse_200_platform_get_cpu_id();

	status = ipm_send(ipm_handle, 0, current_core ? 0 : 1, 0, 1);
#elif defined(CONFIG_IPM_STM32_HSEM)
	/* No data transfer, only doorbell. */
	status = ipm_send(ipm_handle, 0, 0, NULL, 0);
#else
	/* The IPM interface is unclear on whether or not ipm_send
	 * can be called with NULL as data, thus, drivers might cause
	 * problems if you do. To avoid problems, we always send some
	 * dummy data, unless the IPM driver cannot transfer data.
	 * Ref: #68741
	 */
	uint32_t dummy_data = 0x55005500;

	status = ipm_send(ipm_handle, 0, 0, &dummy_data, sizeof(dummy_data));
#endif /* #if defined(CONFIG_SOC_MPS2_AN521) */

#endif

	if (status != 0) {
		LOG_ERR("ipm_send failed to notify: %d", status);
	}
}

const struct virtio_dispatch dispatch = {
	.get_status = ipc_virtio_get_status,
	.set_status = ipc_virtio_set_status,
	.get_features = ipc_virtio_get_features,
	.set_features = ipc_virtio_set_features,
	.notify = ipc_virtio_notify,
};

static void ipm_callback_process(struct k_work *work)
{
	virtqueue_notification(vqueue[VIRTQUEUE_ID]);
}

static void ipm_callback(const struct device *dev,
						void *context, uint32_t id,
						volatile void *data)
{
	(void)dev;

	LOG_DBG("Got callback of id %u", id);
	/* TODO: Separate workqueue is needed only
	 * for serialization master (app core)
	 *
	 * Use sysworkq to optimize memory footprint
	 * for serialization slave (net core)
	 */
	k_work_submit_to_queue(&ipm_work_q, &ipm_work);
}

int rpmsg_backend_init(struct metal_io_region **io, struct virtio_device *vdev)
{
	int32_t                  err;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	struct metal_device     *device;

	/* Start IPM workqueue */
	k_work_queue_start(&ipm_work_q, ipm_stack_area,
			   K_THREAD_STACK_SIZEOF(ipm_stack_area),
			   IPM_WORK_QUEUE_PRIORITY, NULL);
	k_thread_name_set(&ipm_work_q.thread, "ipm_work_q");

	/* Setup IPM workqueue item */
	k_work_init(&ipm_work, ipm_callback_process);

	/* Libmetal setup */
	err = metal_init(&metal_params);
	if (err) {
		LOG_ERR("metal_init: failed - error code %d", err);
		return err;
	}

	err = metal_register_generic_device(&shm_device);
	if (err) {
		LOG_ERR("Couldn't register shared memory device: %d", err);
		return err;
	}

	err = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (err) {
		LOG_ERR("metal_device_open failed: %d", err);
		return err;
	}

	*io = metal_device_io_region(device, 0);
	if (!*io) {
		LOG_ERR("metal_device_io_region failed to get region");
		return err;
	}

	/* IPM setup */
#if defined(CONFIG_RPMSG_SERVICE_DUAL_IPM_SUPPORT)
	if (!device_is_ready(ipm_tx_handle)) {
		LOG_ERR("IPM TX device is not ready");
		return -ENODEV;
	}

	if (!device_is_ready(ipm_rx_handle)) {
		LOG_ERR("IPM RX device is not ready");
		return -ENODEV;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, NULL);

	err = ipm_set_enabled(ipm_rx_handle, 1);
	if (err != 0) {
		LOG_ERR("Could not enable IPM interrupts and callbacks for RX");
		return err;
	}

#elif defined(CONFIG_RPMSG_SERVICE_SINGLE_IPM_SUPPORT)
	if (!device_is_ready(ipm_handle)) {
		LOG_ERR("IPM device is not ready");
		return -ENODEV;
	}

	ipm_register_callback(ipm_handle, ipm_callback, NULL);

	err = ipm_set_enabled(ipm_handle, 1);
	if (err != 0) {
		LOG_ERR("Could not enable IPM interrupts and callbacks");
		return err;
	}
#endif

	/* Virtqueue setup */
	vqueue[0] = virtqueue_allocate(VRING_SIZE);
	if (!vqueue[0]) {
		LOG_ERR("virtqueue_allocate failed to alloc vqueue[0]");
		return -ENOMEM;
	}

	vqueue[1] = virtqueue_allocate(VRING_SIZE);
	if (!vqueue[1]) {
		LOG_ERR("virtqueue_allocate failed to alloc vqueue[1]");
		return -ENOMEM;
	}

	rvrings[0].io = *io;
	rvrings[0].info.vaddr = (void *)VRING_TX_ADDRESS;
	rvrings[0].info.num_descs = VRING_SIZE;
	rvrings[0].info.align = VRING_ALIGNMENT;
	rvrings[0].vq = vqueue[0];

	rvrings[1].io = *io;
	rvrings[1].info.vaddr = (void *)VRING_RX_ADDRESS;
	rvrings[1].info.num_descs = VRING_SIZE;
	rvrings[1].info.align = VRING_ALIGNMENT;
	rvrings[1].vq = vqueue[1];

	vdev->role = RPMSG_ROLE;
	vdev->vrings_num = VRING_COUNT;
	vdev->func = &dispatch;
	vdev->vrings_info = &rvrings[0];

	return 0;
}

#if MASTER
/* Make sure we clear out the status flag very early (before we bringup the
 * secondary core) so the secondary core see's the proper status
 */
int init_status_flag(void)
{
	ipc_virtio_set_status(NULL, 0);

	return 0;
}

SYS_INIT(init_status_flag, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* MASTER */
