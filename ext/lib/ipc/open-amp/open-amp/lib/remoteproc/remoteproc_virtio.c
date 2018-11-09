/*
 * Remoteproc Virtio Framework Implementation
 *
 * Copyright(c) 2018 Xilinx Ltd.
 * Copyright(c) 2011 Texas Instruments, Inc.
 * Copyright(c) 2011 Google, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name Texas Instruments nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <openamp/remoteproc.h>
#include <openamp/remoteproc_virtio.h>
#include <openamp/virtqueue.h>
#include <metal/utilities.h>
#include <metal/alloc.h>

static void rproc_virtio_virtqueue_notify(struct virtqueue *vq)
{
	struct remoteproc_virtio *rpvdev;
	struct virtio_vring_info *vring_info;
	struct virtio_device *vdev;
	unsigned int vq_id = vq->vq_queue_index;

	vdev = vq->vq_dev;
	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	metal_assert(vq_id <= vdev->vrings_num);
	vring_info = &vdev->vrings_info[vq_id];
	rpvdev->notify(rpvdev->priv, vring_info->notifyid);
}

static unsigned char rproc_virtio_get_status(struct virtio_device *vdev)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;
	char status;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	status = metal_io_read8(io,
				metal_io_virt_to_offset(io, &vdev_rsc->status));
	return status;
}

#ifndef VIRTIO_SLAVE_ONLY
static void rproc_virtio_set_status(struct virtio_device *vdev,
				    unsigned char status)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	metal_io_write8(io,
			metal_io_virt_to_offset(io, &vdev_rsc->status),
			status);
	rpvdev->notify(rpvdev->priv, vdev->index);
}
#endif

static uint32_t rproc_virtio_get_features(struct virtio_device *vdev)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;
	uint32_t features;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	/* TODO: shall we get features based on the role ? */
	features = metal_io_read32(io,
			metal_io_virt_to_offset(io, &vdev_rsc->dfeatures));

	return features;
}

#ifndef VIRTIO_SLAVE_ONLY
static void rproc_virtio_set_features(struct virtio_device *vdev,
				      uint32_t features)
{
	struct remoteproc_virtio *rpvdev;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *io;

	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	vdev_rsc = rpvdev->vdev_rsc;
	io = rpvdev->vdev_rsc_io;
	/* TODO: shall we set features based on the role ? */
	metal_io_write32(io,
			 metal_io_virt_to_offset(io, &vdev_rsc->dfeatures),
			 features);
	rpvdev->notify(rpvdev->priv, vdev->index);
}
#endif

static uint32_t rproc_virtio_negotiate_features(struct virtio_device *vdev,
						uint32_t features)
{
	(void)vdev;
	(void)features;

	return 0;
}

static void rproc_virtio_read_config(struct virtio_device *vdev,
				     uint32_t offset, void *dst, int length)
{
	(void)vdev;
	(void)offset;
	(void)dst;
	(void)length;
}

#ifndef VIRTIO_SLAVE_ONLY
static void rproc_virtio_write_config(struct virtio_device *vdev,
				      uint32_t offset, void *src, int length)
{
	(void)vdev;
	(void)offset;
	(void)src;
	(void)length;
}

static void rproc_virtio_reset_device(struct virtio_device *vdev)
{
	if (vdev->role == VIRTIO_DEV_MASTER)
		rproc_virtio_set_status(vdev,
					VIRTIO_CONFIG_STATUS_NEEDS_RESET);
}
#endif

const struct virtio_dispatch remoteproc_virtio_dispatch_funcs = {
	.get_status =  rproc_virtio_get_status,
	.get_features = rproc_virtio_get_features,
	.read_config = rproc_virtio_read_config,
	.notify = rproc_virtio_virtqueue_notify,
	.negotiate_features = rproc_virtio_negotiate_features,
#ifndef VIRTIO_SLAVE_ONLY
	/*
	 * We suppose here that the vdev is in a shared memory so that can
	 * be access only by one core: the master. In this case salve core has
	 * only read access right.
	 */
	.set_status = rproc_virtio_set_status,
	.set_features = rproc_virtio_set_features,
	.write_config = rproc_virtio_write_config,
	.reset_device = rproc_virtio_reset_device,
#endif
};

struct virtio_device *
rproc_virtio_create_vdev(unsigned int role, unsigned int notifyid,
			 void *rsc, struct metal_io_region *rsc_io,
			 void *priv,
			 rpvdev_notify_func notify,
			 virtio_dev_reset_cb rst_cb)
{
	struct remoteproc_virtio *rpvdev;
	struct virtio_vring_info *vrings_info;
	struct fw_rsc_vdev *vdev_rsc = rsc;
	struct virtio_device *vdev;
	unsigned int num_vrings = vdev_rsc->num_of_vrings;
	unsigned int i;

	rpvdev = metal_allocate_memory(sizeof(*rpvdev));
	if (!rpvdev)
		return NULL;
	vrings_info = metal_allocate_memory(sizeof(*vrings_info) * num_vrings);
	if (!vrings_info)
		goto err0;
	memset(rpvdev, 0, sizeof(*rpvdev));
	memset(vrings_info, 0, sizeof(*vrings_info));
	vdev = &rpvdev->vdev;

	for (i = 0; i < num_vrings; i++) {
		struct virtqueue *vq;
		struct fw_rsc_vdev_vring *vring_rsc;
		unsigned int num_extra_desc = 0;

		vring_rsc = &vdev_rsc->vring[i];
		if (role == VIRTIO_DEV_MASTER) {
			num_extra_desc = vring_rsc->num;
		}
		vq = virtqueue_allocate(num_extra_desc);
		if (!vq)
			goto err1;
		vrings_info[i].vq = vq;
	}

	/* FIXME commended as seems not nedded, already stored in vdev */
	//rpvdev->notifyid = notifyid;
	rpvdev->notify = notify;
	rpvdev->priv = priv;
	vdev->vrings_info = vrings_info;
	/* Assuming the shared memory has been mapped and registered if
	 * necessary
	 */
	rpvdev->vdev_rsc = vdev_rsc;
	rpvdev->vdev_rsc_io = rsc_io;

	vdev->index = notifyid;
	vdev->role = role;
	vdev->reset_cb = rst_cb;
	vdev->vrings_num = num_vrings;
	vdev->func = &remoteproc_virtio_dispatch_funcs;
	/* TODO: Shall we set features here ? */

	return &rpvdev->vdev;

err1:
	for (i = 0; i < num_vrings; i++) {
		if (vrings_info[i].vq)
			metal_free_memory(vrings_info[i].vq);
	}
	metal_free_memory(vrings_info);
err0:
	metal_free_memory(rpvdev);
	return NULL;
}

void rproc_virtio_remove_vdev(struct virtio_device *vdev)
{
	struct remoteproc_virtio *rpvdev;
	unsigned int i;

	if (!vdev)
		return;
	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	for (i = 0; i < vdev->vrings_num; i++) {
		struct virtqueue *vq;

		vq = vdev->vrings_info[i].vq;
		if (vq)
			metal_free_memory(vq);
	}
	metal_free_memory(vdev->vrings_info);
	metal_free_memory(rpvdev);
}

int rproc_virtio_init_vring(struct virtio_device *vdev, unsigned int index,
			    unsigned int notifyid, void *va,
			    struct metal_io_region *io,
			    unsigned int num_descs, unsigned int align)
{
	struct virtio_vring_info *vring_info;
	unsigned int num_vrings;

	num_vrings = vdev->vrings_num;
	if (index >= num_vrings)
		return -RPROC_EINVAL;
	vring_info = &vdev->vrings_info[index];
	vring_info->io = io;
	vring_info->notifyid = notifyid;
	vring_info->info.vaddr = va;
	vring_info->info.num_descs = num_descs;
	vring_info->info.align = align;

	return 0;
}

int rproc_virtio_notified(struct virtio_device *vdev, uint32_t notifyid)
{
	unsigned int num_vrings, i;
	struct virtio_vring_info *vring_info;
	struct virtqueue *vq;

	if (!vdev)
		return -EINVAL;
	/* We do nothing for vdev notification in this implementation */
	if (vdev->index == notifyid)
		return 0;
	num_vrings = vdev->vrings_num;
	for (i = 0; i < num_vrings; i++) {
		vring_info = &vdev->vrings_info[i];
		if (vring_info->notifyid == notifyid ||
		    notifyid == RSC_NOTIFY_ID_ANY) {
			vq = vring_info->vq;
			virtqueue_notification(vq);
		}
	}
	return 0;
}

void rproc_virtio_wait_remote_ready(struct virtio_device *vdev)
{
	uint8_t status;

	/*
	 * No status available for slave. As Master has not to wait
	 * slave action, we can return. Behavior should be updated
	 * in future if a slave status is added.
	 */
	if (vdev->role == VIRTIO_DEV_MASTER)
		return;

	while (1) {
		status = rproc_virtio_get_status(vdev);
		if (status & VIRTIO_CONFIG_STATUS_DRIVER_OK)
			return;
	}
}
