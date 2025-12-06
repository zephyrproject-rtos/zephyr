/*
 * Copyright (c) 2025 Ayush Singh, BeagleBoard.org
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <resource_table.h>
#include <openamp/open_amp.h>
#include <zephyr/drivers/ipm.h>
#include <addr_translation.h>

#define SHM_NODE       DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE       DT_REG_SIZE(SHM_NODE)

static metal_phys_addr_t shm_physmap = SHM_START_ADDR;
static const struct device *const ipm_handle = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));
static struct fw_resource_table *rsc_table;
static metal_phys_addr_t rsc_tab_physmap;
static struct metal_io_region shm_io_data;
static struct metal_io_region rsc_io_data;

static struct virtio_vring_info rvrings[2];

static void ipm_callback_process(struct k_work *work)
{
	if (rvrings[1].vq) {
		virtqueue_notification(rvrings[1].vq);
	}
}

K_WORK_DEFINE(ipm_work, ipm_callback_process);

static void rproc_virtio_virtqueue_notify(struct virtqueue *vq)
{
	struct virtio_vring_info *vring_info;
	struct virtio_device *vdev;
	unsigned int vq_id = vq->vq_queue_index;

	vdev = vq->vq_dev;
	vring_info = &vdev->vrings_info[vq_id];
	ipm_send(ipm_handle, 0, vring_info->notifyid, &vring_info->notifyid, 4);
}

static unsigned char rproc_virtio_get_status(struct virtio_device *vdev)
{
	struct fw_rsc_vdev *vdev_rsc = rsc_table_to_vdev(rsc_table);

	RSC_TABLE_INVALIDATE(vdev_rsc, sizeof(struct fw_rsc_vdev));

	return metal_io_read8(&rsc_io_data,
			      metal_io_virt_to_offset(&rsc_io_data, &vdev_rsc->status));
}

static int rproc_virtio_create_virtqueue(struct virtio_device *vdev, unsigned int flags,
					 unsigned int idx, const char *name, vq_callback callback)
{
	struct virtio_vring_info *vring_info;
	struct vring_alloc_info *vring_alloc;
	int ret;
	(void)flags;

	/* Get the vring information */
	vring_info = &vdev->vrings_info[idx];
	vring_alloc = &vring_info->info;

	/* Fail if the virtqueue has already been created */
	if (vring_info->vq) {
		return ERROR_VQUEUE_INVLD_PARAM;
	}

	/* Alloc the virtqueue and init it */
	vring_info->vq = virtqueue_allocate(vring_alloc->num_descs);
	if (!vring_info->vq) {
		return ERROR_NO_MEM;
	}

	if (VIRTIO_ROLE_IS_DRIVER(vdev)) {
		size_t offset = metal_io_virt_to_offset(vring_info->io, vring_alloc->vaddr);
		size_t size = vring_size(vring_alloc->num_descs, vring_alloc->align);

		metal_io_block_set(vring_info->io, offset, 0, size);
	}

	ret = virtqueue_create(vdev, idx, name, vring_alloc, callback, vdev->func->notify,
			       vring_info->vq);
	if (ret) {
		return ret;
	}

	return 0;
}

static void rproc_virtio_delete_virtqueues(struct virtio_device *vdev)
{
	struct virtio_vring_info *vring_info;
	unsigned int i;

	if (!vdev->vrings_info) {
		return;
	}

	for (i = 0; i < vdev->vrings_num; i++) {
		vring_info = &vdev->vrings_info[i];
		if (vring_info->vq) {
			virtqueue_free(vring_info->vq);
		}
	}
}

static int rproc_virtio_create_virtqueues(struct virtio_device *vdev, unsigned int flags,
					  unsigned int nvqs, const char *names[],
					  vq_callback callbacks[], void *callback_args[])
{
	unsigned int i;
	int ret;
	(void)callback_args;

	/* Check virtqueue numbers and the vrings_info */
	if (nvqs > vdev->vrings_num || !vdev || !vdev->vrings_info) {
		return ERROR_VQUEUE_INVLD_PARAM;
	}

	/* set the notification id for vrings */
	for (i = 0; i < nvqs; i++) {
		ret = rproc_virtio_create_virtqueue(vdev, flags, i, names[i], callbacks[i]);
		if (ret) {
			goto err;
		}
	}
	return 0;

err:
	rproc_virtio_delete_virtqueues(vdev);
	return ret;
}

static uint32_t rproc_virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

static const struct virtio_dispatch dispatch = {
	.create_virtqueues = rproc_virtio_create_virtqueues,
	.delete_virtqueues = rproc_virtio_delete_virtqueues,
	.get_status = rproc_virtio_get_status,
	.get_features = rproc_virtio_get_features,
	.notify = rproc_virtio_virtqueue_notify,
};

static void platform_ipm_callback(const struct device *dev, void *context, uint32_t id,
				  volatile void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(context);
	ARG_UNUSED(id);
	ARG_UNUSED(data);

	k_work_submit(&ipm_work);
}

static int platform_init(void)
{
	int status, rsc_size;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	status = metal_init(&metal_params);
	if (status) {
		return -1;
	}

	/* declare shared memory region */
	metal_io_init(&shm_io_data, (void *)shm_physmap, &shm_physmap, SHM_SIZE, -1, 0,
		      addr_translation_get_ops(shm_physmap));

	/* declare resource table region */
	rsc_table_get((void **)&rsc_table, &rsc_size);
	rsc_tab_physmap = (uintptr_t)rsc_table;

	metal_io_init(&rsc_io_data, rsc_table, &rsc_tab_physmap, rsc_size, -1, 0, NULL);

	/* setup IPM */
	if (!device_is_ready(ipm_handle)) {
		return -ENODEV;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_handle, 1);
	if (status) {
		return -1;
	}

	return 0;
}

static void rproc_virtio_create_vdev_local(struct virtio_device *vdev,
					   struct virtio_vring_info *vrings_info, unsigned int role,
					   unsigned int notifyid, void *rsc,
					   struct metal_io_region *rsc_io)
{
	struct fw_rsc_vdev *vdev_rsc = rsc;
	unsigned int num_vrings = vdev_rsc->num_of_vrings;

	/* Initialize the virtio device */
	vdev->vrings_info = vrings_info;
	vdev->notifyid = notifyid;
	vdev->id.device = vdev_rsc->id;
	vdev->role = role;
	vdev->reset_cb = NULL;
	vdev->vrings_num = num_vrings;
	vdev->func = &dispatch;
}

static void rproc_virtio_wait_remote_ready_local(struct virtio_device *vdev)
{
	uint8_t status;

	/*
	 * No status available for remote. As virtio driver has not to wait
	 * remote action, we can return. Behavior should be updated
	 * in future if a remote status is added.
	 */
	if (VIRTIO_ROLE_IS_DRIVER(vdev)) {
		return;
	}

	while (1) {
		status = rproc_virtio_get_status(vdev);
		if (status & VIRTIO_CONFIG_STATUS_DRIVER_OK) {
			return;
		}
		k_sleep(K_SECONDS(1));
	}
}

int rpmsg_backend_init(struct metal_io_region **io, struct virtio_device *vdev)
{
	int ret;
	struct fw_rsc_vdev_vring *vring_rsc;

	/* Initialize platform */
	ret = platform_init();
	if (ret) {
		return ret;
	}

	rproc_virtio_create_vdev_local(vdev, rvrings, VIRTIO_DEV_DEVICE, VDEV_ID,
				       rsc_table_to_vdev(rsc_table), &rsc_io_data);

	/* wait master rpmsg init completion */
	rproc_virtio_wait_remote_ready_local(vdev);

	vring_rsc = rsc_table_get_vring0(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid, (void *)vring_rsc->da,
				      &rsc_io_data, vring_rsc->num, vring_rsc->align);
	if (ret) {
		goto failed;
	}

	vring_rsc = rsc_table_get_vring1(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid, (void *)vring_rsc->da,
				      &rsc_io_data, vring_rsc->num, vring_rsc->align);
	if (ret) {
		goto failed;
	}

	*io = &shm_io_data;

	return 0;

failed:
	rproc_virtio_remove_vdev(vdev);
	return -1;
}
