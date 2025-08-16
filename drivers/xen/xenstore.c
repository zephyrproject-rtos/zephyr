/*
 * Copyright (c) 2023 EPAM Systems
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <xen/public/io/xs_wire.h>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/hvm.h>
#include <zephyr/xen/sched.h>
#include <zephyr/xen/events.h>
#include <zephyr/xen/xenstore.h>
#include <zephyr/sys/device_mmio.h>

#include <zephyr/logging/log.h>

/*
 * Importing and modifying code from the following source.
 * https://github.com/xen-troops/zephyr-xenlib/blob/v3.1.0/xenstore-srv/src/xenstore_srv.c
 *
 * The queues used are swapped because the read and write directions for xenstore are different.
 */

static bool check_indexes(XENSTORE_RING_IDX cons, XENSTORE_RING_IDX prod)
{
	return ((prod - cons) > XENSTORE_RING_SIZE);
}

static size_t get_input_offset(XENSTORE_RING_IDX cons, XENSTORE_RING_IDX prod, size_t *len)
{
	size_t delta = prod - cons;
	*len = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(cons);

	if (delta < *len) {
		*len = delta;
	}

	return MASK_XENSTORE_IDX(cons);
}

static size_t get_output_offset(XENSTORE_RING_IDX cons, XENSTORE_RING_IDX prod, size_t *len)
{
	size_t free_space = XENSTORE_RING_SIZE - (prod - cons);

	*len = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(prod);
	if (free_space < *len) {
		*len = free_space;
	}

	return MASK_XENSTORE_IDX(prod);
}

static int ring_write(struct xenstore *xs, const void *data, size_t len)
{
	size_t avail;
	void *dest;
	struct xenstore_domain_interface *intf = xs->domint;
	XENSTORE_RING_IDX cons, prod;

	cons = intf->req_cons;
	prod = intf->req_prod;
	z_barrier_dmem_fence_full();

	if (check_indexes(cons, prod)) {
		return -EINVAL;
	}

	dest = intf->req + get_output_offset(cons, prod, &avail);
	if (avail < len) {
		len = avail;
	}

	memcpy(dest, data, len);
	z_barrier_dmem_fence_full();
	intf->req_prod += len;

	notify_evtchn(xs->local_evtchn);

	return len;
}

static int ring_read(struct xenstore *xs, void *data, size_t len)
{
	size_t avail;
	const void *src;
	struct xenstore_domain_interface *intf = xs->domint;
	XENSTORE_RING_IDX cons, prod;

	cons = intf->rsp_cons;
	prod = intf->rsp_prod;
	z_barrier_dmem_fence_full();

	if (check_indexes(cons, prod)) {
		return -EIO;
	}

	src = intf->rsp + get_input_offset(cons, prod, &avail);
	if (avail < len) {
		len = avail;
	}

	if (data) {
		memcpy(data, src, len);
	}

	z_barrier_dmem_fence_full();
	intf->rsp_cons += len;

	if (data) {
		notify_evtchn(xs->local_evtchn);
	}

	return len;
}

/*
 * End importing.
 */

LOG_MODULE_REGISTER(xen_xenstore);

static atomic_t next_req_id;
static struct k_sem xs_lock;

static uint8_t work_buf[XENSTORE_RING_SIZE * 2];
static uint8_t read_buf[XENSTORE_RING_SIZE];
static size_t work_pos;
static size_t read_pos;
static int expected_req_id;
static int processed_req_id;

static xs_notify_cb notify_cb;
static void *notify_param;

struct k_spinlock lock;

static inline size_t ring_avail_for_read(struct xenstore *xs)
{
	struct xenstore_domain_interface *intf = xs->domint;
	XENSTORE_RING_IDX cons, prod;

	cons = intf->rsp_cons;
	prod = intf->rsp_prod;

	return prod - cons;
}

static void store_cb(void *ptr)
{
	struct xenstore *xs = ptr;
	size_t offset = 0;

	while (ring_avail_for_read(xs)) {
		int ret = ring_read(xs, work_buf + work_pos, ring_avail_for_read(xs));

		if (ret < 0) {
			break;
		}
		if (work_pos + ret > sizeof(work_buf)) {
			LOG_ERR("work_buf overflowed: %ld", work_pos + ret);
			work_pos = 0;
			break;
		}

		work_pos += ret;
	}

	while (work_pos - offset >= sizeof(struct xsd_sockmsg)) {
		struct xsd_sockmsg *hdr = (void *)(work_buf + offset);
		size_t msglen = hdr->len + sizeof(*hdr);

		if (work_pos - offset < msglen) {
			break;
		}

		if ((hdr->req_id == expected_req_id) && (hdr->req_id > processed_req_id)) {
			k_spinlock_key_t key = k_spin_lock(&lock);

			memcpy(read_buf, hdr, msglen);
			read_pos = msglen;
			k_spin_unlock(&lock, key);

			processed_req_id = expected_req_id;
			k_sem_give(&xs_lock);
		} else {
			k_spinlock_key_t key = k_spin_lock(&lock);

			if (notify_cb) {
				notify_cb((char *)hdr, msglen, notify_param);
			}

			k_spin_unlock(&lock, key);
		}
		offset += msglen;
	}

	if (offset) {
		memmove(work_buf, work_buf + offset, work_pos - offset);
		work_pos -= offset;
	}
}

static int xs_cmd_req(struct xenstore *xs, int type, const char *const *params, size_t param_num,
		      char *buf, size_t len, uint32_t *preq_id)
{
	size_t plen = 0;
	int err;

	for (int i = 0; i < param_num; i++) {
		plen += strlen(params[i]) + 1;
	}

	if (plen > XENSTORE_PAYLOAD_MAX) {
		LOG_ERR("strlen(path) + 1: %zu > XENSTORE_PAYLOAD_MAX", plen);
		return -ENAMETOOLONG;
	}

	*preq_id = atomic_inc(&next_req_id);
	if (*preq_id == 0) {
		*preq_id = atomic_inc(&next_req_id);
	}

	struct xsd_sockmsg hdr = {
		.type = type,
		.req_id = *preq_id,
		.tx_id = 0,
		.len = plen,
	};

	err = ring_write(xs, &hdr, sizeof(struct xsd_sockmsg));
	if (err < 0) {
		LOG_ERR("ring_write(hdr) failed: %d", err);
		return -EAGAIN;
	} else if (err < sizeof(struct xsd_sockmsg)) {
		LOG_ERR("ring_write(hdr) shorter response: %d", err);
		return -EIO;
	}

	for (int i = 0; i < param_num; i++) {
		err = ring_write(xs, params[i], strlen(params[i]) + 1);
		if (err < 0) {
			LOG_ERR("ring_write(path) failed: %d", err);
			return -EAGAIN;
		} else if (err < strlen(params[i]) + 1) {
			LOG_ERR("ring_write(path) shorter response: %d", err);
			return -EIO;
		}
	}

	return 0;
}

static ssize_t xs_cmd(struct xenstore *xs, int type, const char *const *params, size_t params_num,
		      char *buf, size_t len, k_timeout_t timeout)
{
	int err;
	struct xsd_sockmsg *hdr;

	err = xs_cmd_req(xs, type, params, params_num, buf, len, &expected_req_id);
	if (err < 0) {
		LOG_ERR("xs_rw_common error: %d", err);
		return -EIO;
	}

	err = k_sem_take(&xs_lock, timeout);
	if (err < 0) {
		LOG_ERR("k_sem_take error: %d", err);
		return -EINVAL;
	}

	hdr = (void *)read_buf;

	if (hdr->len > len) {
		LOG_ERR("no buffer hdr.len=%u > len=%zu)", hdr->len, len);
		err = -ENOBUFS;
		goto end;
	}

	ssize_t copy_len = (len - 1 < hdr->len) ? len - 1 : hdr->len;
	k_spinlock_key_t key = k_spin_lock(&lock);

	memcpy(buf, read_buf + sizeof(struct xsd_sockmsg), copy_len);
	read_pos = 0;

	k_spin_unlock(&lock, key);

	if (copy_len < len) {
		buf[copy_len] = '\0';
	}

end:
	if (err) {
		return err;
	}

	return copy_len;
}

int xs_init_xenstore(struct xenstore *xs)
{
	uint64_t paddr = 0;
	uint64_t value = 0;
	mm_reg_t vaddr = 0;
	int err;

	k_sem_init(&xs_lock, 0, 1);

	err = hvm_get_parameter(HVM_PARAM_STORE_EVTCHN, DOMID_SELF, &value);
	if (err) {
		LOG_ERR("hvm_get_parameter(STORE_EVTCHN) failed: %d", err);
		return -ENODEV;
	}
	xs->local_evtchn = value;

	err = hvm_get_parameter(HVM_PARAM_STORE_PFN, DOMID_SELF, &paddr);
	if (err) {
		LOG_ERR("hvm_get_param(STORE_PFN) failed: err=%d", err);
		return -EIO;
	}
	device_map(&vaddr, XEN_PFN_PHYS(paddr), XEN_PAGE_SIZE, K_MEM_CACHE_WB | K_MEM_PERM_RW);

	xs->domint = (struct xenstore_domain_interface *)vaddr;

	while (ring_avail_for_read(xs)) {
		ring_read(xs, NULL, ring_avail_for_read(xs));
	}

	bind_event_channel(xs->local_evtchn, store_cb, xs);
	unmask_event_channel(xs->local_evtchn);

	return 0;
}

void xs_set_notify_callback(xs_notify_cb cb, void *param)
{
	notify_cb = cb;
	notify_param = param;
}

ssize_t xs_read(struct xenstore *xs, const char *path, char *buf, size_t len)
{
	const char *const params[] = {path};

	if (!xs || !path || !buf || len == 0) {
		return -EINVAL;
	}

	return xs_cmd(xs, XS_READ, params, ARRAY_SIZE(params), buf, len, K_FOREVER);
}

ssize_t xs_directory(struct xenstore *xs, const char *path, char *buf, size_t len)
{
	const char *const params[] = {path};

	if (!xs || !path || !buf || len == 0) {
		return -EINVAL;
	}

	return xs_cmd(xs, XS_DIRECTORY, params, ARRAY_SIZE(params), buf, len, K_FOREVER);
}

ssize_t xs_watch(struct xenstore *xs, const char *path, const char *token, char *buf, size_t len)
{
	const char *const params[] = {path, token};

	if (!xs || !path || !token || !buf || len == 0) {
		return -EINVAL;
	}

	return xs_cmd(xs, XS_WATCH, params, ARRAY_SIZE(params), buf, len, K_FOREVER);
}
