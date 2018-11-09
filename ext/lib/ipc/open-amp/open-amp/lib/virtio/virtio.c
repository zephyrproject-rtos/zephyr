/*-
 * Copyright (c) 2011, Bryan Venteicher <bryanv@FreeBSD.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <openamp/virtio.h>

static const char *virtio_feature_name(unsigned long feature,
				       const struct virtio_feature_desc *);

//TODO : This structure may change depending on the types of devices we support.
static const struct virtio_ident {
	unsigned short devid;
	const char *name;
} virtio_ident_table[] = {
	{
	VIRTIO_ID_NETWORK, "Network"}, {
	VIRTIO_ID_BLOCK, "Block"}, {
	VIRTIO_ID_CONSOLE, "Console"}, {
	VIRTIO_ID_ENTROPY, "Entropy"}, {
	VIRTIO_ID_BALLOON, "Balloon"}, {
	VIRTIO_ID_IOMEMORY, "IOMemory"}, {
	VIRTIO_ID_SCSI, "SCSI"}, {
	VIRTIO_ID_9P, "9P Transport"}, {
	0, NULL}
};

/* Device independent features. */
static const struct virtio_feature_desc virtio_common_feature_desc[] = {
	{VIRTIO_F_NOTIFY_ON_EMPTY, "NotifyOnEmpty"},
	{VIRTIO_RING_F_INDIRECT_DESC, "RingIndirect"},
	{VIRTIO_RING_F_EVENT_IDX, "EventIdx"},
	{VIRTIO_F_BAD_FEATURE, "BadFeature"},

	{0, NULL}
};

const char *virtio_dev_name(unsigned short devid)
{
	const struct virtio_ident *ident;

	for (ident = virtio_ident_table; ident->name != NULL; ident++) {
		if (ident->devid == devid)
			return (ident->name);
	}

	return (NULL);
}

static const char *virtio_feature_name(unsigned long val,
				       const struct virtio_feature_desc *desc)
{
	int i, j;
	const struct virtio_feature_desc *descs[2] = { desc,
		virtio_common_feature_desc
	};

	for (i = 0; i < 2; i++) {
		if (!descs[i])
			continue;

		for (j = 0; descs[i][j].vfd_val != 0; j++) {
			if (val == descs[i][j].vfd_val)
				return (descs[i][j].vfd_str);
		}
	}

	return (NULL);
}

void virtio_describe(struct virtio_device *dev, const char *msg,
		     uint32_t features, struct virtio_feature_desc *desc)
{
	(void)dev;
	(void)msg;
	(void)features;

	// TODO: Not used currently - keeping it for future use
	virtio_feature_name(0, desc);
}

int virtio_create_virtqueues(struct virtio_device *vdev, unsigned int flags,
			     unsigned int nvqs, const char *names[],
			     vq_callback *callbacks[])
{
	struct virtio_vring_info *vring_info;
	struct vring_alloc_info *vring_alloc;
	unsigned int num_vrings, i;
	int ret;
	(void)flags;

	num_vrings = vdev->vrings_num;
	if (nvqs > num_vrings)
		return -ERROR_VQUEUE_INVLD_PARAM;
	/* Initialize virtqueue for each vring */
	for (i = 0; i < nvqs; i++) {
		vring_info = &vdev->vrings_info[i];

		vring_alloc = &vring_info->info;
#ifndef VIRTIO_SLAVE_ONLY
		if (vdev->role == VIRTIO_DEV_MASTER) {
			size_t offset;
			struct metal_io_region *io = vring_info->io;

			offset = metal_io_virt_to_offset(io,
							 vring_alloc->vaddr);
			metal_io_block_set(io, offset, 0,
					   vring_size(vring_alloc->num_descs,
						      vring_alloc->align));
		}
#endif
		ret = virtqueue_create(vdev, i, names[i], vring_alloc,
				       callbacks[i], vdev->func->notify,
				       vring_info->vq);
		if (ret)
			return ret;
	}
	return 0;
}

