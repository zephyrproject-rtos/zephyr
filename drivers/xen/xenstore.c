/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/hypercall.h>

#include <zephyr/xen/generic.h>
#include <zephyr/xen/hvm.h>
#include <zephyr/xen/events.h>
#include <zephyr/xen/sched.h>
#include <xen/public/io/xs_wire.h>

#include <zephyr/sys/device_mmio.h>
#include <zephyr/logging/log.h>

#include <sys/types.h>

#define XS_HDR_SZ            sizeof(struct xsd_sockmsg)
#define XS_POLL_TIMEOUT_USEC 1000000

LOG_MODULE_REGISTER(xen_virtio_xs_lite, 0);

static atomic_t next_req_id;

static struct xenstore_domain_interface *get_xenstore_interface(void)
{
	uint64_t paddr = 0;
	mm_reg_t vaddr = 0;
	int err;

	err = hvm_get_parameter(HVM_PARAM_STORE_PFN, DOMID_SELF, &paddr);
	if (err) {
		LOG_ERR("hvm_get_param(STORE_PFN) failed: %d", err);
		return NULL;
	}

	/* printk("paddr %llx\n", paddr); */

	device_map(&vaddr, XEN_PFN_PHYS(paddr), XEN_PAGE_SIZE, K_MEM_CACHE_WB | K_MEM_PERM_RW);

	/* printk("vaddr %lx\n", vaddr); */

	return (struct xenstore_domain_interface *)vaddr;
}

/* 便利マクロ ------------------------------------------------------------ */
#define RING_MASK(idx) ((idx) & (XENSTORE_RING_SIZE - 1U))

/*
 * Xenのリングバッファインデックスはwrapせず単調増加。
 * アクセス時にはRING_MASKで循環させる設計。
 */
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
	size_t space = ring_space(*prod, cons);
	if (len > space) { /* オーバーラン回避 */
		return -EAGAIN;
	}

	ring_memcpy_out(ring, *prod, src, len);
	*prod += (XENSTORE_RING_IDX)len;

	return (ssize_t)len;
}

static ssize_t xs_ring_read(const char *ring, XENSTORE_RING_IDX prod,
			    volatile XENSTORE_RING_IDX *cons, void *dst, size_t len)
{
	size_t avail = ring_avail(prod, *cons);
	if (len > avail) { /* アンダーラン回避 */
		printk("len=%ld > available=%ld\n", len, avail);
		return -EAGAIN;
	}

	ring_memcpy_in(ring, *cons, dst, len);
	*cons += (XENSTORE_RING_IDX)len;

	return (ssize_t)len;
}

static ssize_t xs_ring_skip(volatile XENSTORE_RING_IDX *cons, XENSTORE_RING_IDX prod, size_t len)
{
	size_t avail = ring_avail(prod, *cons);
	if (len > avail) {
		return -EAGAIN; /* アンダーラン回避 */
	}

	*cons += (XENSTORE_RING_IDX)len;

	return len;
}

ssize_t xs_r(int type, const char *path, char *buf, size_t len)
{
	struct xenstore_domain_interface *xs;
	evtchn_port_t port;
	uint64_t value = 0;
	size_t plen;
	ssize_t copy_len = 0;
	int err;

	/* サイズ上限チェック */
	plen = strlen(path) + 1;
	if (plen > XENSTORE_PAYLOAD_MAX) {
		printk("hdr.len failed\n");
		return -ENAMETOOLONG;
	}

	xs = get_xenstore_interface();
	if (!xs) {
		return -EIO;
	}

	/*
	printk("xs = %p\n", xs);
	printk("xs->req_prod = %d\n", xs->req_prod);
	printk("xs->rsp_prod = %d\n", xs->rsp_prod);
	printk("xs->server_features = %x\n", xs->server_features);
	printk("xs->connection = %x\n", xs->connection);
	printk("xs->error = %x\n", xs->error);
	*/

	/*----------- 1. イベントチャネル確保 ------------------*/

	err = hvm_get_parameter(HVM_PARAM_STORE_EVTCHN, DOMID_SELF, &value);
	if (err) {
		printk("get_evtchn_port failed\n");
		return -ENODEV;
	}

	port = value;
	LOG_DBG("evtchn_port = %d\n", port);

	/*----------- 2. ヘッダ作成 & リクエスト送信 -----------*/
	uint32_t req_id = atomic_inc(&next_req_id);
	struct xsd_sockmsg hdr = {
		.type = type,
		.req_id = req_id,
		.tx_id = 0,
		.len = plen,
	};

	/* printk("xs_ring_write\n"); */

	/* header + path をリングへ */
	err = xs_ring_write(xs->req, &xs->req_prod, xs->req_cons, &hdr, XS_HDR_SZ);
	if (err < 0) {
		LOG_ERR("xs_ring_write(hdr) failed: %d", err);
		return -EAGAIN; /* 異常時は上層でリトライ */
	}

	/* printk("xs_ring_write\n"); */

	err = xs_ring_write(xs->req, &xs->req_prod, xs->req_cons, path, hdr.len);
	if (err < 0) {
		LOG_ERR("xs_ring_write(path) failed: %d", err);
		return -EAGAIN; /* 異常時は上層でリトライ */
	}

	/* printk("notify\n"); */

	/* Notify xenstored */
	notify_evtchn(port);

	/* printk("poll\n"); */

	/*----------- 3. 応答待ち -----------------------------*/
	err = sched_poll(&port, 1, XS_POLL_TIMEOUT_USEC);
	if (err < 0) {
		LOG_ERR("sched_poll timeout/error: %d", err);
		return -EIO;
	}

	/*----------- 4. ヘッダ取り出し ------------------------*/
	/* ヘッダのデータがそろうまで待つ */
	while (ring_avail(xs->rsp_prod, xs->rsp_cons) < XS_HDR_SZ) {
		arch_spin_relax();
		k_yield();
	}

	/* printk("read\n"); */

	struct xsd_sockmsg rhdr;
	err = xs_ring_read(xs->rsp, xs->rsp_prod, &xs->rsp_cons, &rhdr, XS_HDR_SZ);
	if (err < 0) {
		LOG_ERR("xs_ring_read(hdr) failed: %d", err);
		return -EIO;
	}

	/*----------- 5. ヘッダ検証 ---------------------------*/
	if (rhdr.req_id != req_id) {
		LOG_ERR("rhdr.req_id != req_id)");
		return -EIO; /* シンプルに abort。キューイング対応なら要キュー */
	}

	if (rhdr.type == XS_ERROR) {
		LOG_ERR("rhdr.type == XS_ERROR");
		/* エラー文字列を読み飛ばして consumer を進める */
		/* xs_ring_skip(&xs->rsp_cons, xs->rsp_prod, rhdr.len); */
		err = -EIO;
		goto end;
	}

	if ((size_t)rhdr.len > len) {
		/* バッファ不足 */
		/* xs_ring_skip(&xs->rsp_cons, xs->rsp_prod, rhdr.len); */
		err = -ENOBUFS;
		goto end;
	}

	/* printk("avail\n"); */

	/*----------- 6. ペイロード受信 -----------------------*/
	/* 必要分そろうまで待つ */
	while (ring_avail(xs->rsp_prod, xs->rsp_cons) < rhdr.len) {
		arch_spin_relax();
		k_yield();
	}

	/* printk("read\n"); */
	copy_len = (len - 1 < rhdr.len) ? len - 1 : rhdr.len;
	if (xs_ring_read(xs->rsp, xs->rsp_prod, &xs->rsp_cons, buf, copy_len) < 0) {
		printk("read2 failed\n");
		return -EIO;
	}

	buf[copy_len] = '\0';

	/* printk("skip\n"); */
	/* 残りのパディング (読み捨て) */
end:
	if (rhdr.len > copy_len) {
		xs_ring_skip(&xs->rsp_cons, xs->rsp_prod, rhdr.len - copy_len);
	}

	if (err) {
		return err;
	}

	return (ssize_t)copy_len;
}

ssize_t xs_read(const char *path, char *buf, size_t len)
{
	return xs_r(XS_READ, path, buf, len);
}

ssize_t xs_directory(const char *path, char *buf, size_t len)
{
	return xs_r(XS_DIRECTORY, path, buf, len);
}

ssize_t xs_write(const char *path, const char *value)
{
	struct xenstore_domain_interface *xs;
	evtchn_port_t port;
	uint64_t evtchn_value = 0;
	size_t plen, vlen, total_len;
	int err;

	/* パラメータ検証 */
	if (!path || !value) {
		return -EINVAL;
	}

	/* サイズ上限チェック */
	plen = strlen(path) + 1;  /* null terminator included */
	vlen = strlen(value) + 1; /* null terminator included */
	total_len = plen + vlen;

	if (total_len > XENSTORE_PAYLOAD_MAX) {
		LOG_ERR("payload too large: %zu > %d", total_len, XENSTORE_PAYLOAD_MAX);
		return -ENAMETOOLONG;
	}

	xs = get_xenstore_interface();
	if (!xs) {
		return -EIO;
	}

	/*----------- 1. イベントチャネル確保 ------------------*/
	err = hvm_get_parameter(HVM_PARAM_STORE_EVTCHN, DOMID_SELF, &evtchn_value);
	if (err) {
		LOG_ERR("get_evtchn_port failed: %d", err);
		return -ENODEV;
	}

	port = evtchn_value;
	LOG_DBG("evtchn_port = %d", port);

	/*----------- 2. ヘッダ作成 & リクエスト送信 -----------*/
	uint32_t req_id = atomic_inc(&next_req_id);
	struct xsd_sockmsg hdr = {
		.type = XS_WRITE,
		.req_id = req_id,
		.tx_id = 0,
		.len = total_len - 1,
	};

	/* header をリングへ */
	err = xs_ring_write(xs->req, &xs->req_prod, xs->req_cons, &hdr, XS_HDR_SZ);
	if (err < 0) {
		LOG_ERR("xs_ring_write(hdr) failed: %d", err);
		return -EAGAIN;
	}

	/* path をリングへ */
	err = xs_ring_write(xs->req, &xs->req_prod, xs->req_cons, path, plen);
	if (err < 0) {
		LOG_ERR("xs_ring_write(path) failed: %d", err);
		return -EAGAIN;
	}

	/* value をリングへ */
	err = xs_ring_write(xs->req, &xs->req_prod, xs->req_cons, value, vlen - 1);
	if (err < 0) {
		LOG_ERR("xs_ring_write(value) failed: %d", err);
		return -EAGAIN;
	}

	/* Notify xenstored */
	notify_evtchn(port);

	/*----------- 3. 応答待ち -----------------------------*/
	err = sched_poll(&port, 1, XS_POLL_TIMEOUT_USEC);
	if (err < 0) {
		LOG_ERR("sched_poll timeout/error: %d", err);
		return -EIO;
	}

	/*----------- 4. ヘッダ取り出し ------------------------*/
	/* ヘッダのデータがそろうまで待つ */
	while (ring_avail(xs->rsp_prod, xs->rsp_cons) < XS_HDR_SZ) {
		arch_spin_relax();
		k_yield();
	}

	struct xsd_sockmsg rhdr;
	err = xs_ring_read(xs->rsp, xs->rsp_prod, &xs->rsp_cons, &rhdr, XS_HDR_SZ);
	if (err < 0) {
		LOG_ERR("xs_ring_read(hdr) failed: %d", err);
		return -EIO;
	}

	/*----------- 5. ヘッダ検証 ---------------------------*/
	if (rhdr.req_id != req_id) {
		LOG_ERR("rhdr.req_id != req_id: expected %u, got %u", req_id, rhdr.req_id);
		return -EIO;
	}

	if (rhdr.type == XS_ERROR) {
		LOG_ERR("XS_WRITE failed with XS_ERROR");
		/* エラー文字列を読み飛ばして consumer を進める */
		if (rhdr.len > 0) {
			xs_ring_skip(&xs->rsp_cons, xs->rsp_prod, rhdr.len);
		}
		return -EIO;
	}

	/*----------- 6. ペイロード処理 -----------------------*/
	/* XS_WRITE の応答にはペイロードが含まれる場合があるので読み飛ばす */
	if (rhdr.len > 0) {
		/* 必要分そろうまで待つ */
		while (ring_avail(xs->rsp_prod, xs->rsp_cons) < rhdr.len) {
			arch_spin_relax();
			k_yield();
		}

		/* ペイロードを読み飛ばす */
		xs_ring_skip(&xs->rsp_cons, xs->rsp_prod, rhdr.len);
	}

	/* 成功した場合は書き込んだバイト数を返す */
	return vlen - 1; /* null terminator を除いた実際の値の長さ */
}
