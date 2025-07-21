/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/vhost/vringh.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <string.h>

LOG_MODULE_REGISTER(vhost_vringh);

int vringh_init(struct vringh *vrh, uint64_t features, uint16_t num, bool weak_barriers,
		struct virtq_desc *desc, struct virtq_avail *avail, struct virtq_used *used)
{
	if (!vrh || !desc || !avail || !used) {
		return -EINVAL;
	}

	memset(vrh, 0, sizeof(*vrh));

	vrh->event_indices = false; /* not supported */
	vrh->weak_barriers = weak_barriers;
	vrh->last_avail_idx = 0;
	vrh->last_used_idx = 0;
	vrh->completed = 0;

	vrh->vring.num = num;
	vrh->vring.desc = desc;
	vrh->vring.avail = avail;
	vrh->vring.used = used;

	return 0;
}

static void vringh_kick_callback(const struct device *dev, uint16_t queue_id, void *ptr)
{
	struct vringh *vrh = ptr;

	if (vrh->kick) {
		vrh->kick(vrh);
	}
}

int vringh_init_device(struct vringh *vrh, const struct device *dev, uint16_t queue_id,
		       void (*kick_callback)(struct vringh *vrh))
{
	uint64_t drv_feats;
	void *parts[3];
	size_t q_num;
	int ret;

	if (!vrh || !dev) {
		return -EINVAL;
	}

	ret = vhost_get_virtq(dev, queue_id, parts, &q_num);
	if (ret < 0) {
		LOG_ERR("vhost_get_virtq failed: %d", ret);
		return ret;
	}

	ret = vhost_get_driver_features(dev, &drv_feats);
	if (ret < 0) {
		LOG_ERR("vhost_get_driver_features failed: %d", ret);
		return ret;
	}

	ret = vringh_init(vrh, drv_feats, q_num, false, parts[0], parts[1], parts[2]);
	if (ret < 0) {
		LOG_ERR("vringh_init failed: %d", ret);
		return ret;
	}

	vrh->dev = dev;
	vrh->queue_id = queue_id;
	vrh->kick = kick_callback;

	ret = vhost_register_virtq_notify_cb(dev, queue_id, vringh_kick_callback, (void *)vrh);
	if (ret < 0) {
		LOG_ERR("vhost_set_notify_callback failed: %d", ret);
		return ret;
	}

	return 0;
}

int vringh_getdesc(struct vringh *vrh, struct vringh_iov *riov, struct vringh_iov *wiov,
		   uint16_t *head_out)
{
	if (!vrh || !riov || !wiov || !head_out) {
		return -EINVAL;
	}

	barrier_dmem_fence_full();

	k_spinlock_key_t key = k_spin_lock(&vrh->lock);
	struct vring *vr = &vrh->vring;
	const uint16_t avail_idx = sys_le16_to_cpu(vrh->vring.avail->idx);
	const uint16_t slot = vrh->last_avail_idx % vr->num;
	const uint16_t head = sys_le16_to_cpu(vr->avail->ring[slot]);
	struct vhost_buf desc_ranges[vr->num];
	size_t filled_read = 0;
	size_t filled_write = 0;
	uint16_t idx = head;
	size_t count = 0;
	uint16_t flags;
	int ret;

	if (vrh->last_avail_idx == avail_idx) {
		k_spin_unlock(&vrh->lock, key);
		return 0;
	}

	if (head >= vrh->vring.num) {
		k_spin_unlock(&vrh->lock, key);
		LOG_ERR("Invalid descriptor head: %u >= %u", head, vrh->vring.num);
		return -EINVAL;
	}

	if (!vrh->event_indices) {
		flags = sys_le16_to_cpu(vr->used->flags);
		flags |= VIRTQ_USED_F_NO_NOTIFY;
		vr->used->flags = sys_cpu_to_le16(flags);
	}

	barrier_dmem_fence_full();

	k_spin_unlock(&vrh->lock, key);

	vringh_iov_reset(riov);
	vringh_iov_reset(wiov);

	do {
		const struct virtq_desc *d = &vr->desc[idx];
		const uint64_t gpa = sys_le64_to_cpu(d->addr);
		const uint32_t len = sys_le32_to_cpu(d->len);
		const uint16_t next = sys_le16_to_cpu(d->next);

		flags = sys_le16_to_cpu(d->flags);

		if (count >= vr->num) {
			LOG_ERR("Descriptor chain too long: %zu", count);
			ret = -ENOMEM;
			goto failed;
		}

		/* Validate next descriptor index */
		if ((flags & VIRTQ_DESC_F_NEXT) && next >= vr->num) {
			LOG_ERR("Invalid next descriptor: %u >= %u", next, vr->num);
			ret = -EINVAL;
			goto failed;
		}

		if (len == 0) {
			LOG_WRN("Zero-length descriptor at index %u", idx);
			idx = next;
			continue;
		}

		/* Store descriptor information for Phase 2 */
		desc_ranges[count].gpa = gpa;
		desc_ranges[count].len = len;
		desc_ranges[count].is_write = !!(flags & VIRTQ_DESC_F_WRITE);

		count++;
		idx = next;
	} while (flags & VIRTQ_DESC_F_NEXT);

	ret = vhost_prepare_iovec(vrh->dev, vrh->queue_id, head, desc_ranges, count, riov->iov,
				  riov->max_num, wiov->iov, wiov->max_num, &filled_read,
				  &filled_write);
	if (ret < 0) {
		LOG_ERR("vhost_prepare_iovec failed: %d", ret);
		goto failed;
	}

	riov->used = filled_read;
	wiov->used = filled_write;

	/* Success - update state and return */
	*head_out = head;

	key = k_spin_lock(&vrh->lock);
	vrh->last_avail_idx++;
	k_spin_unlock(&vrh->lock, key);

	return 1;

failed:
	if (count > 0) {
		int rc = vhost_release_iovec(vrh->dev, vrh->queue_id, head);

		if (rc < 0) {
			LOG_ERR("vhost_release_iovec failed: %d", rc);
			vhost_set_device_status(vrh->dev, DEVICE_STATUS_FAILED);
		}
	}

	vringh_iov_reset(riov);
	vringh_iov_reset(wiov);

	return ret;
}

int vringh_complete(struct vringh *vrh, uint16_t head, uint32_t total_len)
{
	struct vring *vr = &vrh->vring;
	int rc = 0;

	if (!vrh) {
		return -EINVAL;
	}

	rc = vhost_release_iovec(vrh->dev, vrh->queue_id, head);
	if (rc < 0) {
		LOG_ERR("vhost_release_iovec failed: %d", rc);
		vhost_set_device_status(vrh->dev, DEVICE_STATUS_FAILED);
		return rc;
	}

	k_spinlock_key_t key = k_spin_lock(&vrh->lock);

	const uint16_t used_idx = sys_le16_to_cpu(vr->used->idx);
	struct virtq_used_elem *ue = &vr->used->ring[used_idx % vr->num];

	LOG_DBG("used_idx %u ue={%u, %u}", used_idx, head, total_len);

	ue->id = sys_cpu_to_le32((uint32_t)head);
	ue->len = sys_cpu_to_le32(total_len);

	barrier_dmem_fence_full();
	vr->used->idx = sys_cpu_to_le16(used_idx + 1);
	vrh->last_used_idx++;

	uint16_t flags = sys_le16_to_cpu(vr->used->flags);

	flags &= ~VIRTQ_USED_F_NO_NOTIFY;
	vr->used->flags = sys_cpu_to_le16(flags);
	barrier_dmem_fence_full();

	k_spin_unlock(&vrh->lock, key);

	return rc;
}

int vringh_abandon(struct vringh *vrh, uint32_t num)
{
	struct vring *vr = &vrh->vring;
	int rc = 0;

	if (num == 0) {
		return 0;
	}

	if (!vrh) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&vrh->lock);

	if (num <= vrh->last_avail_idx) {
		vrh->last_avail_idx -= num;
		LOG_DBG("Abandoned %u descs, new last_avail_idx: %u", num, vrh->last_avail_idx);
	} else {
		LOG_ERR("Cannot abandon %u descs, avail=%u", num, vrh->last_avail_idx);
		rc = -ERANGE;
	}

	uint16_t flags = sys_le16_to_cpu(vr->used->flags);

	flags &= ~VIRTQ_USED_F_NO_NOTIFY;
	vr->used->flags = sys_cpu_to_le16(flags);

	barrier_dmem_fence_full();

	k_spin_unlock(&vrh->lock, key);

	return rc;
}

void vringh_iov_reset(struct vringh_iov *iov)
{
	if (!iov || !iov->iov) {
		return;
	}

	if (iov->consumed > 0 && iov->i < iov->used) {
		iov->iov[iov->i].iov_len += iov->consumed;
		iov->iov[iov->i].iov_base = (char *)iov->iov[iov->i].iov_base - iov->consumed;
	}

	iov->consumed = 0;
	iov->i = 0;
	iov->used = 0;
}

int vringh_need_notify(struct vringh *vrh)
{
	if (!vrh) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&vrh->lock);
	const uint16_t flags = sys_le16_to_cpu(vrh->vring.avail->flags);

	k_spin_unlock(&vrh->lock, key);

	return !(flags & VIRTQ_AVAIL_F_NO_INTERRUPT);
}

void vringh_notify(struct vringh *vrh)
{
	k_spinlock_key_t key = k_spin_lock(&vrh->lock);
	const uint16_t flags = sys_le16_to_cpu(vrh->vring.avail->flags);

	k_spin_unlock(&vrh->lock, key);

	if (flags & VIRTQ_AVAIL_F_NO_INTERRUPT) {
		return;
	}

	vhost_notify_virtq(vrh->dev, vrh->queue_id);
}
