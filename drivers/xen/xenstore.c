/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <xen/public/io/xs_wire.h>

#include <zephyr/xen/generic.h>
#include <zephyr/xen/hvm.h>
#include <zephyr/xen/sched.h>
#include <zephyr/xen/events.h>
#include <zephyr/sys/device_mmio.h>
#include <sys/types.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(xen_virtio_xs_lite, 0);

#define RING_MASK(idx) ((idx) & (XENSTORE_RING_SIZE - 1U))
#define XS_POLL_TIMEOUT_USEC 1000000

static atomic_t next_req_id;

static struct xenstore_domain_interface *get_xenstore_interface(void)
{
	uint64_t paddr = 0;
	mm_reg_t vaddr = 0;
	int err;

	err = hvm_get_parameter(HVM_PARAM_STORE_PFN, DOMID_SELF, &paddr);
	if (err) {
		LOG_ERR("hvm_get_param(STORE_PFN) failed: err=%d", err);
		return NULL;
	}

	device_map(&vaddr, XEN_PFN_PHYS(paddr), XEN_PAGE_SIZE, K_MEM_CACHE_WB | K_MEM_PERM_RW);

	return (struct xenstore_domain_interface *)vaddr;
}

static inline size_t ring_avail(XENSTORE_RING_IDX prod, XENSTORE_RING_IDX cons)
{
	return prod - cons;
}

static inline size_t ring_space(XENSTORE_RING_IDX prod, XENSTORE_RING_IDX cons)
{
	return XENSTORE_RING_SIZE - (prod - cons);
}

static void ring_memcpy_out(char *dst, XENSTORE_RING_IDX prod, const void *src, size_t len)
{
	size_t src_idx = 0;

	while (len > 0) {
		size_t off = RING_MASK(prod);
		size_t chunk = XENSTORE_RING_SIZE - off;
		if (chunk > len) {
			chunk = len;
		}

		memcpy(dst + off, (const uint8_t *)src + src_idx, chunk);

		prod += chunk;
		src_idx += chunk;
		len -= chunk;
	}

	barrier_dmem_fence_full();
}

static void ring_memcpy_in(const char *src, XENSTORE_RING_IDX cons, void *dst, size_t len)
{
	barrier_dmem_fence_full();

	size_t dst_idx = 0;

	while (len > 0) {
		size_t off = RING_MASK(cons);
		size_t chunk = XENSTORE_RING_SIZE - off;
		if (chunk > len) {
			chunk = len;
		}

		memcpy((uint8_t *)dst + dst_idx, src + off, chunk);

		cons += chunk;
		dst_idx += chunk;
		len -= chunk;
	}
}

static ssize_t xs_ring_write(char *ring, volatile XENSTORE_RING_IDX *prod, XENSTORE_RING_IDX cons,
			     const void *src, size_t len)
{
	const size_t space = ring_space(*prod, cons);

	if (len > space) {
		LOG_INF("len=%ld > space=%ld", len, space);
		return -EAGAIN;
	}

	ring_memcpy_out(ring, *prod, src, len);
	*prod += (XENSTORE_RING_IDX)len;

	return len;
}

static ssize_t xs_ring_read(const char *ring, XENSTORE_RING_IDX prod,
			    volatile XENSTORE_RING_IDX *cons, void *dst, size_t len)
{
	const size_t avail = ring_avail(prod, *cons);

	if (len > avail) { /* アンダーラン回避 */
		LOG_INF("len=%ld > avail=%ld", len, avail);
		return -EAGAIN;
	}

	if (ring != NULL && dst != NULL) {
		ring_memcpy_in(ring, *cons, dst, len);
	}

	*cons += (XENSTORE_RING_IDX)len;

	return len;
}

static inline ssize_t xs_ring_skip(XENSTORE_RING_IDX prod, volatile XENSTORE_RING_IDX *cons,
				   size_t len)
{
	return xs_ring_read(NULL, prod, cons, NULL, len);
}

static ssize_t xs_r(int type, const char *path, char *buf, size_t len)
{
	struct xenstore_domain_interface *xs;
	evtchn_port_t port;
	uint64_t value = 0;
	ssize_t copy_len = 0;
	uint32_t req_id;
	size_t plen;
	int err;

	plen = strlen(path) + 1;
	if (plen > XENSTORE_PAYLOAD_MAX) {
		LOG_ERR("strlen(path) + 1: %ld > XENSTORE_PAYLOAD_MAX", plen);
		return -ENAMETOOLONG;
	}

	xs = get_xenstore_interface();
	if (!xs) {
		return -EIO;
	}

	err = hvm_get_parameter(HVM_PARAM_STORE_EVTCHN, DOMID_SELF, &value);
	if (err) {
		LOG_ERR("hvm_get_parameter(STORE_EVTCHN) failed: %d", err);
		return -ENODEV;
	}

	port = value;
	req_id = atomic_inc(&next_req_id);

	struct xsd_sockmsg hdr = {
		.type = type,
		.req_id = req_id,
		.tx_id = 0,
		.len = plen,
	};

	err = xs_ring_write(xs->req, &xs->req_prod, xs->req_cons, &hdr, sizeof(struct xsd_sockmsg));
	if (err < 0) {
		LOG_ERR("xs_ring_write(hdr) failed: %d", err);
		return -EAGAIN;
	}

	err = xs_ring_write(xs->req, &xs->req_prod, xs->req_cons, path, plen);
	if (err < 0) {
		LOG_ERR("xs_ring_write(path) failed: %d", err);
		return -EAGAIN;
	}

	/* notify and wait response */
	notify_evtchn(port);
	err = sched_poll(&port, 1, XS_POLL_TIMEOUT_USEC);
	if (err < 0) {
		LOG_ERR("sched_poll timeout/error: %d", err);
		return -EIO;
	}

	while (ring_avail(xs->rsp_prod, xs->rsp_cons) < sizeof(struct xsd_sockmsg)) {
		arch_spin_relax();
		k_yield();
	}

	memset(&hdr, 0, sizeof(struct xsd_sockmsg));

	err = xs_ring_read(xs->rsp, xs->rsp_prod, &xs->rsp_cons, &hdr, sizeof(struct xsd_sockmsg));
	if ((err < 0) || (hdr.req_id != req_id) || (hdr.type == XS_ERROR)) {
		LOG_ERR("err=%d, hdr.req_id=%d, req_id=%d, hdr.type=%d", err, hdr.req_id, req_id,
			hdr.type);
		return -EIO;
	}

	if (hdr.len > len) {
		LOG_ERR("no buffer hdr.len=%d > len=%ld)", hdr.len, len);
		err = -ENOBUFS;
		goto end;
	}

	while (ring_avail(xs->rsp_prod, xs->rsp_cons) < hdr.len) {
		arch_spin_relax();
		k_yield();
	}

	copy_len = (len - 1 < hdr.len) ? len - 1 : hdr.len;
	err = xs_ring_read(xs->rsp, xs->rsp_prod, &xs->rsp_cons, buf, copy_len);
	if (err < 0) {
		LOG_ERR("xs_read_write(buf) failed: %d", err);
		return -EIO;
	}

	buf[copy_len] = '\0';

end:
	if (hdr.len > copy_len) {
		xs_ring_skip(xs->rsp_prod, &xs->rsp_cons, hdr.len - copy_len);
	}

	if (err) {
		return err;
	}

	return copy_len;
}

ssize_t xs_read(const char *path, char *buf, size_t len)
{
	return xs_r(XS_READ, path, buf, len);
}

ssize_t xs_directory(const char *path, char *buf, size_t len)
{
	return xs_r(XS_DIRECTORY, path, buf, len);
}
