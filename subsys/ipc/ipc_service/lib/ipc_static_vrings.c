/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/ipc_static_vrings.h>
#include <zephyr/cache.h>

#define SHM_DEVICE_NAME		"sram0.shm"

#define RPMSG_VQ_0		(0) /* TX virtqueue queue index */
#define RPMSG_VQ_1		(1) /* RX virtqueue queue index */

static void ipc_virtio_notify(struct virtqueue *vq)
{
	struct ipc_static_vrings *vr;

	vr = CONTAINER_OF(vq->vq_dev, struct ipc_static_vrings, vdev);

	if (vr->notify_cb) {
		vr->notify_cb(vq, vr->priv);
	}
}

static void ipc_virtio_set_features(struct virtio_device *vdev, uint32_t features)
{
	/* No need for implementation */
}

static void ipc_virtio_set_status(struct virtio_device *p_vdev, unsigned char status)
{
	struct ipc_static_vrings *vr;

	if (p_vdev->role != VIRTIO_DEV_DRIVER) {
		return;
	}

	vr = CONTAINER_OF(p_vdev, struct ipc_static_vrings, vdev);

	sys_write8(status, vr->status_reg_addr);
	sys_cache_data_flush_range((void *) vr->status_reg_addr, sizeof(status));
}

static uint32_t ipc_virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

static unsigned char ipc_virtio_get_status(struct virtio_device *p_vdev)
{
	struct ipc_static_vrings *vr;
	uint8_t ret;

	vr = CONTAINER_OF(p_vdev, struct ipc_static_vrings, vdev);

	ret = VIRTIO_CONFIG_STATUS_DRIVER_OK;

	if (p_vdev->role == VIRTIO_DEV_DEVICE) {
		sys_cache_data_invd_range((void *) vr->status_reg_addr, sizeof(ret));
		ret = sys_read8(vr->status_reg_addr);
	}

	return ret;
}

const static struct virtio_dispatch dispatch = {
	.get_status = ipc_virtio_get_status,
	.get_features = ipc_virtio_get_features,
	.set_status = ipc_virtio_set_status,
	.set_features = ipc_virtio_set_features,
	.notify = ipc_virtio_notify,
};

static int libmetal_setup(struct ipc_static_vrings *vr)
{
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	struct metal_device *device;
	int err;

	err = metal_init(&metal_params);
	if (err != 0) {
		return err;
	}

	err = metal_register_generic_device(&vr->shm_device);
	if (err != 0) {
		return err;
	}

	err = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (err != 0) {
		return err;
	}

	vr->shm_io = metal_device_io_region(device, 0);
	if (vr->shm_io == NULL) {
		return err;
	}

	return 0;
}

static int libmetal_teardown(struct ipc_static_vrings *vr)
{
	vr->shm_io = 0;

	metal_device_close(&vr->shm_device);

	metal_finish();

	return 0;
}

static int vq_setup(struct ipc_static_vrings *vr, unsigned int role)
{
	vr->vq[RPMSG_VQ_0] = virtqueue_allocate(vr->vring_size);
	if (vr->vq[RPMSG_VQ_0] == NULL) {
		return -ENOMEM;
	}

	vr->vq[RPMSG_VQ_1] = virtqueue_allocate(vr->vring_size);
	if (vr->vq[RPMSG_VQ_1] == NULL) {
		return -ENOMEM;
	}

	vr->rvrings[RPMSG_VQ_0].io = vr->shm_io;
	vr->rvrings[RPMSG_VQ_0].info.vaddr = (void *) vr->tx_addr;
	vr->rvrings[RPMSG_VQ_0].info.num_descs = vr->vring_size;
	vr->rvrings[RPMSG_VQ_0].info.align = MEM_ALIGNMENT;
	vr->rvrings[RPMSG_VQ_0].vq = vr->vq[RPMSG_VQ_0];

	vr->rvrings[RPMSG_VQ_1].io = vr->shm_io;
	vr->rvrings[RPMSG_VQ_1].info.vaddr = (void *) vr->rx_addr;
	vr->rvrings[RPMSG_VQ_1].info.num_descs = vr->vring_size;
	vr->rvrings[RPMSG_VQ_1].info.align = MEM_ALIGNMENT;
	vr->rvrings[RPMSG_VQ_1].vq = vr->vq[RPMSG_VQ_1];

	vr->vdev.role = role;

	vr->vdev.vrings_num = VRING_COUNT;
	vr->vdev.func = &dispatch;
	vr->vdev.vrings_info = &vr->rvrings[0];

	return 0;
}

static int vq_teardown(struct ipc_static_vrings *vr, unsigned int role)
{
	memset(&vr->vdev, 0, sizeof(struct virtio_device));

	memset(&(vr->rvrings[RPMSG_VQ_1]), 0, sizeof(struct virtio_vring_info));
	memset(&(vr->rvrings[RPMSG_VQ_0]), 0, sizeof(struct virtio_vring_info));

	virtqueue_free(vr->vq[RPMSG_VQ_1]);
	virtqueue_free(vr->vq[RPMSG_VQ_0]);

	return 0;
}

int ipc_static_vrings_init(struct ipc_static_vrings *vr, unsigned int role)
{
	int err = 0;

	if (!vr) {
		return -EINVAL;
	}

	vr->shm_device.name = SHM_DEVICE_NAME;
	vr->shm_device.num_regions = 1;
	vr->shm_physmap[0] = vr->shm_addr;

	metal_io_init(vr->shm_device.regions, (void *) vr->shm_addr,
		      vr->shm_physmap, vr->shm_size, -1, 0, NULL);

	err = libmetal_setup(vr);
	if (err != 0) {
		return err;
	}

	return vq_setup(vr, role);
}

int ipc_static_vrings_deinit(struct ipc_static_vrings *vr, unsigned int role)
{
	int err;

	err = vq_teardown(vr, role);
	if (err != 0) {
		return err;
	}

	err = libmetal_teardown(vr);
	if (err != 0) {
		return err;
	}

	metal_io_finish(vr->shm_device.regions);

	return 0;
}
