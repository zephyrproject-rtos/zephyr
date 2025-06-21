/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/types.h>

#include <xen/public/xen.h>
#include <xen/public/hvm/ioreq.h>
#include <xen/public/hvm/dm_op.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/events.h>
#include <zephyr/xen/hvm.h>
#include <zephyr/xen/gnttab.h>
#include <zephyr/xen/memory.h>
#include <zephyr/xen/xenstore.h>
#include <zephyr/xen/dmop.h>
#include <zephyr/virtio/backend.h>
#include <zephyr/virtio/virtio.h>
#include <zephyr/virtio/virtqueue.h>

#include "virtio_common.h"
#include "virtio_backend.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(xen_virtio_mmio_backend, 0);

#define DT_DRV_COMPAT xen_virtio_mmio_backend

#define HVM_IOREQSRV_BUFIOREQ_OFF 0
#define PAGE_SHIFT                (__builtin_ctz(CONFIG_MMU_PAGE_SIZE))
#define XEN_GRANT_ADDR_OFF        (1ULL << 63)

#define VIRTIO_MMIO_MAGIC             0x74726976
#define VIRTIO_MMIO_SUPPORTED_VERSION 2

#define VIRTIO_BE_MAX_SG 16

struct virtio_mmio_backend_cfg {
	uint8_t max_queues;
	uint8_t device_id;
	uint32_t vendor_id;
	size_t reg_size;
};

struct virtio_mmio_backend_data {
	/* Xen objects */
	uintptr_t base;

	ioservid_t srv_id;
	domid_t fe_domid;

	evtchn_port_t port;
	struct shared_iopage *shared_iopage;

	uint32_t vcpus;
	uint32_t fe_irq;

	struct virtq *virtqueues;
	size_t virtqueue_count;

	struct k_spinlock lock;

	uint8_t device_features_sel;
	uint8_t driver_features_sel;
	uint8_t irq_status;
	uint64_t queue_desc;
	uint64_t queue_avail;
	uint64_t queue_used;
	uint32_t queue_sel;
	uint32_t queue_ready;
	uint32_t status;

	uint8_t *desc_page;
	uint8_t *avail_page;
	uint8_t *used_page;
};

struct virtq_buf_array {
	struct virtq_buf buf[VIRTIO_BE_MAX_SG];
	struct gnttab_unmap_grant_ref unmap[VIRTIO_BE_MAX_SG];
	size_t pos;
	size_t len;
};

struct virtio_be_req {
	struct virtq_buf_array in;
	struct virtq_buf_array out;
	uint16_t head_idx;
	size_t count;
};

static const struct virtio_mmio_backend_bus_api virtio_mmio_backend_api = {
	/* .register_device = virtio_mmio_backend_register_device, */
};

static void bind_interdomain_cb(void *priv)
{
	printk("bind_interdomain_cb %p\n", priv);
}

static inline void iov_push(struct virtq_buf_array *iov, void *base, uint32_t len,
			    struct gnttab_unmap_grant_ref *unmap)
{
	iov->buf[iov->pos].addr = base;
	iov->buf[iov->pos].len = len;
	iov->unmap[iov->pos] = *unmap;
	iov->pos++;
}

static void *page_grant(domid_t fe_domid, uint64_t addr, struct gnttab_unmap_grant_ref *unmap)
{
	grant_ref_t gref = (addr & ~XEN_GRANT_ADDR_OFF) >> PAGE_SHIFT;
	void *host_page = gnttab_get_page();
	struct gnttab_map_grant_ref op = {
		.host_addr = (uintptr_t)host_page,
		.flags = GNTMAP_host_map, /* 必要なら | GNTMAP_readonly */
		.ref = gref,
		.dom = fe_domid,
	};
	int ret;

	if (!(addr & XEN_GRANT_ADDR_OFF)) {
		printk("no grant addr\n");
		return NULL;
	}

	if (!host_page) {
		printk("host_page can't alocate\n");
		return NULL;
	}

	ret = gnttab_map_refs(&op, 1);
	if (ret < 0 || op.status != 0) {
		printk("virtio-mmio: map gref=%u failed: %d\n", gref, ret);
		return NULL;
	}

	unmap->host_addr = op.host_addr;
	unmap->dev_bus_addr = op.dev_bus_addr;
	unmap->handle = op.handle;

	return (void *)(op.host_addr + (addr & (XEN_PAGE_SIZE - 1)));
}

static int build_req(struct virtio_mmio_backend_data *data, struct virtq *vq, uint16_t head,
		     struct virtio_be_req *req)
{
	uint16_t idx = head;
	uint16_t flags;

	memset(req, 0, sizeof(struct virtio_be_req));

	do {
		struct gnttab_unmap_grant_ref unmap;
		struct virtq_desc *d = &vq->desc[idx];
		const uint64_t gpa = sys_le64_to_cpu(d->addr);
		const uint32_t len = sys_le32_to_cpu(d->len);
		const size_t off = gpa & (XEN_PAGE_SIZE - 1);
		void *va;

		va = page_grant(data->fe_domid, gpa, &unmap);
		flags = sys_le16_to_cpu(d->flags);

		printk("%s: d=%p, gpa=%llu len=%u off=%ld va=%p\n", __func__, d, gpa, len, off, va);

		iov_push((flags & VIRTQ_DESC_F_WRITE) ? &req->in : &req->out, va, len, &unmap);

		idx = sys_le16_to_cpu(d->next);
		printk("idx = %d\n", idx);
		req->count++;
	} while (flags & VIRTQ_DESC_F_NEXT);

	req->head_idx = head;

	return 0;
}

static int destroy_req(struct virtio_be_req *req)
{
	for (int i = (req->in.pos - 1); i >= 0; i--) {
		int ret = gnttab_unmap_refs(&req->in.unmap[i], 1);
	}
	for (int i = (req->out.pos - 1); i >= 0; i--) {
		int ret = gnttab_unmap_refs(&req->out.unmap[i], 1);
	}
}

static int virtio_be_isr(const struct device *dev, uint16_t virtqueue_id,
			 int (*cb)(struct virtio_be_req *, void *), void *ctx)
{
	struct virtio_mmio_backend_data *data = dev->data;

	barrier_dmem_fence_full();
	printk("%s\n", __func__);
	struct virtq *vq = &data->virtqueues[virtqueue_id];
	printk("%s: vq=%p\n", __func__, vq);
	printk("%s: vq=%p avail=%p\n", __func__, vq, vq->avail);
	printk("%s: vq=%p avail->idx %d\n", __func__, vq, vq->avail->idx);
	uint16_t avail_idx = sys_le16_to_cpu(vq->avail->idx);

	printk("%s: vq=%p, last=%u a_idx=%u\n", __func__, vq, vq->last_avail_idx, avail_idx);

	while (vq->last_avail_idx != avail_idx) {
		uint16_t slot = vq->last_avail_idx % vq->num;
		uint16_t head = sys_le16_to_cpu(vq->avail->ring[slot]);

		printk("slot=%u, head=%u\n", slot, head);

		struct virtio_be_req req = {0};
		build_req(data, vq, head, &req);

		/* サブドライバ呼び出し */
		int rc = cb(&req, ctx);
		if (rc) {
			return rc;
		}

		destroy_req(&req);

		/* Used へ返却（len はサブドライバが in/write 総和を書いても良い）*/
		uint32_t total_len = 0;
		for (uint8_t i = 0; i < req.in.pos; i++) {
			total_len += req.in.buf[i].len;
		}

		printk("total_len = %d\n", total_len);

		uint16_t used_idx = sys_le16_to_cpu(vq->used->idx);
		struct virtq_used_elem *ue = &vq->used->ring[used_idx % vq->num];
		ue->id = sys_cpu_to_le16(head);
		ue->len = sys_cpu_to_le32(total_len);

		printk("used_idx %d ue={%d, %d}\n", used_idx, ue->id, ue->len);

		barrier_dmem_fence_full();
		vq->used->idx = sys_cpu_to_le16(used_idx + 1);
		vq->last_avail_idx++;
	}

	return 0;
}

static int virtio_test_fill_cb(struct virtio_be_req *req, void *unused_ctx)
{
	static uint8_t cnt = 0; /* 呼ばれるたびに続きから書く */

	for (size_t s = 0; s < req->in.pos; s++) {
		uint8_t *dst = req->in.buf[s].addr;
		uint32_t len = req->in.buf[s].len;

		printk("%s: addr=%p\n", __func__, dst);

		for (uint32_t i = 0; i < len; i++) {
			printk("dst=%p i=%d : %d\n", dst, i, cnt);
			sys_write8(cnt++, (mem_addr_t)&dst[i]); /* 0,1,2,… とインクリメント */
		}
	}
	barrier_dmem_fence_full();
	/* 書き込んだ総バイト数は req->in.len に既に入っているため
	 * コア側の total_len 計算はそのまま流用できます。
	 */

	return 0; /* 成功 */
}

static void virtio_setup_queue(const struct device *dev)
{
	struct virtio_mmio_backend_data *data = dev->data;

	const uint64_t addrs[3] = {data->queue_desc, data->queue_avail, data->queue_used};
	uint64_t host_addr[3];
	uint64_t dev_bus_addr[3];
	grant_handle_t handle[3];
	unsigned long offset;
	struct virtq *q;
	int i, ret;
	bool map_failed = false;

	q = &data->virtqueues[data->queue_sel];

	/* ３ページまとめてマップ */
	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		grant_ref_t gref = (addrs[i] & ~XEN_GRANT_ADDR_OFF) >> PAGE_SHIFT;
		void *host_page = gnttab_get_page();
		struct gnttab_map_grant_ref op = {
			.host_addr = (uintptr_t)host_page,
			.flags = GNTMAP_host_map, /* 必要ならGNTMAP_readonly */
			.ref = gref,
			.dom = data->fe_domid,
		};

		if (!(addrs[i] & XEN_GRANT_ADDR_OFF)) {
			LOG_ERR("BE: addr[%d] missing grant marker: " "0x%llx\n", i, addrs[i]);
			break;
		}

		if (!host_page) {
			LOG_ERR("host_page can't alocate");
			break;
		}

		ret = gnttab_map_refs(&op, 1);
		if (ret < 0 || op.status != 0) {
			LOG_ERR("virtio-mmio: map gref[%d]=%u failed: %d\n", i, gref, ret);
			map_failed = true;
			break;
		}

		host_addr[i] = op.host_addr;
		dev_bus_addr[i] = op.dev_bus_addr;
		handle[i] = op.handle;

		offset = addrs[i] & (XEN_PAGE_SIZE - 1);
		switch (i) {
		case 0:
			data->desc_page = host_page;
			q->desc = (void *)(op.host_addr + offset);
			break;
		case 1:
			data->avail_page = host_page;
			q->avail = (void *)(op.host_addr + offset);
			break;
		case 2:
			data->used_page = host_page;
			q->used = (void *)(op.host_addr + offset);
			break;
		}
	}

	if (map_failed) {
		while (--i >= 0) {
			struct gnttab_unmap_grant_ref unmap = {
				.host_addr = host_addr[i],
				.dev_bus_addr = dev_bus_addr[i],
				.handle = handle[i],
			};
			gnttab_unmap_refs(&unmap, 1);
			switch(i) {
			case 0:
				data->desc_page = NULL;
				break;
			case 1:
				data->avail_page = NULL;
				break;
			case 2:
				data->used_page = NULL;
				break;
			}
		}
		return;
	}
	q->last_avail_idx = 0;

	LOG_DBG("queue[%u]: desc@%p avail@%p used@%p", data->queue_sel, q->desc, q->avail, q->used);
}

static void evtchn_cb_w(const struct device *dev)
{
	struct virtio_mmio_backend_data *data = dev->data;
	struct ioreq *r = &data->shared_iopage->vcpu_ioreq[0];

	LOG_DBG("w/%s %llx\n", virtio_reg_name(r->addr - data->base), r->data);

	switch (r->addr - data->base) {
	case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
		data->device_features_sel = (uint8_t)r->data;
		break;
	case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
		data->driver_features_sel = (uint8_t)r->data;
		break;
	case VIRTIO_MMIO_DRIVER_FEATURES:
		break;
	case VIRTIO_MMIO_INTERRUPT_ACK:
		data->irq_status &= (uint32_t)~r->data;
		break;
	case VIRTIO_MMIO_STATUS:
		data->status |= r->data;
		break;
	case VIRTIO_MMIO_QUEUE_DESC_LOW:
		data->queue_desc = (r->data | (data->queue_desc & 0xFFFFFFFF00000000));
		break;
	case VIRTIO_MMIO_QUEUE_DESC_HIGH:
		data->queue_desc = (r->data << 32 | (data->queue_desc & UINT32_MAX));
		break;
	case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
		data->queue_avail = (r->data | (data->queue_avail & 0xFFFFFFFF00000000));
		break;
	case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
		data->queue_avail = (r->data << 32 | (data->queue_avail & UINT32_MAX));
		break;
	case VIRTIO_MMIO_QUEUE_USED_LOW:
		data->queue_used = (r->data | (data->queue_used & 0xFFFFFFFF00000000));
		break;
	case VIRTIO_MMIO_QUEUE_USED_HIGH:
		data->queue_used = (r->data << 32 | (data->queue_used & UINT32_MAX));
		break;
	case VIRTIO_MMIO_QUEUE_NOTIFY:
		virtio_be_isr(dev, r->data, virtio_test_fill_cb, NULL);
		data->irq_status |= VIRTIO_QUEUE_INTERRUPT;

		dmop_set_irq_level(data->fe_domid, data->fe_irq, 1);
		dmop_set_irq_level(data->fe_domid, data->fe_irq, 0);
		break;
	case VIRTIO_MMIO_QUEUE_SIZE:
		r->data = 1;
		break;
	case VIRTIO_MMIO_QUEUE_READY:
		if (r->data) {
			virtio_setup_queue(dev);
		}
		break;
	case VIRTIO_MMIO_QUEUE_SEL:
		data->queue_sel = r->data;
		break;
	default:
		break;
	}
}

static void evtchn_cb_r(const struct device *dev)
{
	const struct virtio_mmio_backend_cfg *config = dev->config;
	struct virtio_mmio_backend_data *data = dev->data;
	struct ioreq *r = &data->shared_iopage->vcpu_ioreq[0];

	printk("count %u\n", r->count);
	printk("size %u\n", r->size);
	printk("vp_eport %u\n", r->vp_eport);
	printk("state %d\n", r->state);
	printk("df %d\n", r->df);
	printk("type %d\n", r->type);

	switch (r->addr - data->base) {
	case VIRTIO_MMIO_MAGIC_VALUE:
		r->data = VIRTIO_MMIO_MAGIC;
		break;
	case VIRTIO_MMIO_VERSION:
		r->data = VIRTIO_MMIO_SUPPORTED_VERSION;
		break;
	case VIRTIO_MMIO_DEVICE_ID:
		r->data = config->device_id;
		break;
	case VIRTIO_MMIO_VENDOR_ID:
		r->data = config->vendor_id; /* 0x7a707972; */
		break;
	case VIRTIO_MMIO_DEVICE_FEATURES:
		switch (data->device_features_sel) {
		case 0:
			r->data = 0x3;
			break;
		case 1:
			r->data = 0x3;
			break;
		default:
			break;
		}
		break;
	case VIRTIO_MMIO_QUEUE_SIZE_MAX:
		r->data = config->max_queues;
		break;
	case VIRTIO_MMIO_STATUS:
		r->data = data->status;
		break;
	case VIRTIO_MMIO_INTERRUPT_STATUS:
		r->data = data->irq_status;
		data->irq_status = 0;
		break;
	case VIRTIO_MMIO_QUEUE_READY:
		r->data = (data->virtqueues[data->queue_sel].desc != NULL);
		break;
	default:
		r->data = -1;
		break;
	}

	LOG_DBG("r/%s %llx\n", virtio_reg_name(r->addr - data->base), r->data);
}

static void evtchn_cb(void *ptr)
{
	const struct device *dev = ptr;
	struct virtio_mmio_backend_data *data = dev->data;
	struct ioreq *r = &data->shared_iopage->vcpu_ioreq[0];

	if (r->state == STATE_IOREQ_READY) {
		if (r->dir == IOREQ_WRITE) {
			evtchn_cb_w(dev);
		} else {
			evtchn_cb_r(dev);
		}

		r->state = STATE_IORESP_READY;

		barrier_dmem_fence_full();
		notify_evtchn(data->port);
	}
}

static const char *nth_str(const char *buf, size_t len, size_t n)
{
	int cnt = 0;

	if (n == 0) {
		return buf;
	}

	for (int i = 0; i < len; i++) {
		if (buf[i] == '\0') {
			cnt++;
		}

		if (cnt == n && (i != (len - 1))) {
			return &buf[i + 1];
		}
	}

	return NULL;
}

static int query_fe_domid()
{
	char buf[64] = {0};
	int fe_domid = -1;
	ssize_t len;

	printk("query_fe_domid\n");

	len = xs_read("domid", buf, 64);

	printk("domid %s\n", buf);

	while (true) {
		k_msleep(3000);

		len = xs_directory("backend/virtio", buf, 64);

		if (len < 0) {
			continue;
		}

		const char *ptr = NULL;
		for (int i = 0; i < len; i++) {
			ptr = nth_str(buf, len, i);
			if (ptr == NULL) {
				break;
			}
			int fe = atoi(ptr);

			sprintf(buf, "backend/virtio/%d", fe);

			printk("%s\n", buf);

			len = xs_directory(buf, buf, 64);

			printk("%s\n", buf);

			for (int j = 0; i < len; j++) {
				ptr = nth_str(buf, len, i);
				if (ptr == NULL) {
					break;
				}
				int devid = atoi(ptr);

				sprintf(buf, "backend/virtio/%d/%d/frontend-id", fe, devid);

				printk("%s\n", buf);

				xs_read(buf, buf, 64);

				printk("frontend-id = %s\n", buf);

				fe_domid = atoi(buf);

				goto end;
			}
		}

		if (fe_domid > 0) {
			break;
		}
	}

end:

	return fe_domid;
}

static int query_fe_irq()
{
	char buf[64] = {0};
	int irq = -1;
	ssize_t len;

	printk("query_irq\n");

	len = xs_read("domid", buf, 64);

	printk("domid %s\n", buf);

	while (true) {
		k_msleep(3000);

		len = xs_directory("backend/virtio", buf, 64);

		if (len < 0) {
			continue;
		}

		const char *ptr = NULL;
		for (int i = 0; i < len; i++) {
			ptr = nth_str(buf, len, i);
			if (ptr == NULL) {
				break;
			}
			int fe = atoi(ptr);

			sprintf(buf, "backend/virtio/%d", fe);

			printk("%s\n", buf);

			len = xs_directory(buf, buf, 64);

			printk("%s\n", buf);

			for (int j = 0; i < len; j++) {
				ptr = nth_str(buf, len, i);
				if (ptr == NULL) {
					break;
				}
				int devid = atoi(ptr);

				sprintf(buf, "backend/virtio/%d/%d/irq", fe, devid);

				printk("%s\n", buf);

				xs_read(buf, buf, 64);

				printk("irq = %s\n", buf);

				irq = atoi(buf);

				goto end;
			}
		}

		if (irq > 0) {
			break;
		}
	}

end:

	return irq;
}

static uintptr_t query_base_addr(domid_t fe_domid, int devid)
{
	char buf[64];
	sprintf(buf, "backend/virtio/%d/%d/base", fe_domid, devid);

	xs_read(buf, buf, 64);

	return strtoll(&buf[2], NULL, 16);
}

static int virtio_mmio_backend_init(const struct device *dev)
{
	const struct virtio_mmio_backend_cfg *config = dev->config;
	struct virtio_mmio_backend_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t n_frms = 1;
	xen_pfn_t gfn = 0;
	mm_reg_t va;
	int ret;

	xen_events_init();

	data->fe_domid = query_fe_domid();
	data->fe_irq = query_fe_irq();
	data->base = query_base_addr(data->fe_domid, 0);

	ret = dmop_nr_vcpus(data->fe_domid);
	if (ret < 0) {
		LOG_ERR("dmop_nr_vcpus err=%d\n", ret);
		ret = -EIO;
		goto end;
	}

	data->vcpus = ret;

	ret = dmop_create_ioreq_server(data->fe_domid, HVM_IOREQSRV_BUFIOREQ_OFF, &data->srv_id);
	if (ret < 0) {
		LOG_ERR("dmop_create_ioreq_server err=%d", ret);
		ret = -EIO;
		goto end;
	}

	ret = dmop_map_io_range_to_ioreq_server(data->fe_domid, data->srv_id, 1, data->base,
						data->base + config->reg_size - 1);
	if (ret < 0) {
		LOG_ERR("dmop_map_io_range_to_ioreq_server err=%d", ret);
		ret = -EIO;
		goto end;
	}

	ret = xendom_acquire_resource(data->fe_domid, XENMEM_resource_ioreq_server, data->srv_id,
				      XENMEM_resource_ioreq_server_frame_ioreq(0), &n_frms, &gfn);
	if (ret < 0) {
		LOG_ERR("xendom_acquire_resource err=%d", ret);
		ret = -EIO;
		goto end;
	}

	device_map(&va, (gfn << XEN_PAGE_SHIFT), (n_frms << XEN_PAGE_SHIFT), K_MEM_CACHE_NONE);
	data->shared_iopage = (void *)va;

	ret = dmop_set_ioreq_server_state(data->fe_domid, data->srv_id, 1);
	if (ret) {
		LOG_ERR("dmop_set_ioreq_server_state err=%d", ret);
		ret = -EIO;
		goto end;
	}

	uint16_t port = 0;

	for (int i = 0; i < data->vcpus; i++) {
		while (!data->shared_iopage->vcpu_ioreq[i].vp_eport) {
			k_msleep(1000);
		}

		LOG_DBG("bind_interdomain dom=%d remote_port=%d\n", data->fe_domid,
			data->shared_iopage->vcpu_ioreq[i].vp_eport);

		ret = bind_interdomain_event_channel(data->fe_domid,
						     data->shared_iopage->vcpu_ioreq[i].vp_eport,
						     bind_interdomain_cb, (void *)dev);
		if (ret < 0) {
			LOG_ERR("EVTCHNOP_bind_interdomain[%d] err=%d", i, ret);
			ret = -EIO;
			goto end;
		}

		LOG_DBG("bind_interdomain local_port %d\n", ret);

		port = ret;
		data->port = port;

		bind_event_channel(port, evtchn_cb, (void *)dev);
		unmask_event_channel(port);
	}

	if (ret) {
		ret = -EIO;
		goto end;
	}

end:
	k_spin_unlock(&data->lock, key);

	LOG_INF("%s: backend ready base=%lx fe_domid=%d fe_irq=%d vcpus=%d shared_iopage=%p",
		dev->name, data->base, data->fe_domid, data->fe_irq, data->vcpus, data->shared_iopage);
	return ret;
}

#define XEN_VIRTIO_MMIO_BACKEND_INST(idx)                                                          \
	static const struct virtio_mmio_backend_cfg virtio_mmio_backend_cfg_##idx = {              \
		.max_queues = 8,                                                                   \
		.device_id = 4,                                                                    \
		.vendor_id = 0xFFFFFFFF,                                                           \
		.reg_size = XEN_PAGE_SIZE,                                                         \
	};                                                                                         \
	static struct virtq virtio_mmio_backend_virtqueues_##idx[8];                               \
	static struct virtio_mmio_backend_data virtio_mmio_backend_data_##idx = {                  \
		.virtqueues = virtio_mmio_backend_virtqueues_##idx,                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, virtio_mmio_backend_init, NULL,                                 \
			      &virtio_mmio_backend_data_##idx, &virtio_mmio_backend_cfg_##idx,     \
			      POST_KERNEL, 100, &virtio_mmio_backend_api);

DT_INST_FOREACH_STATUS_OKAY(XEN_VIRTIO_MMIO_BACKEND_INST)
