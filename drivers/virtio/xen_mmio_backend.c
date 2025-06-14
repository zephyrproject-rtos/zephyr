/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/events.h>
#include <zephyr/xen/hvm.h>
#include <zephyr/xen/gnttab.h>
#include <zephyr/xen/memory.h>
#include <zephyr/xen/dmop.h>
#include <zephyr/xen/public/xen.h>
#include <zephyr/xen/public/hvm/ioreq.h>
#include <zephyr/xen/public/hvm/dm_op.h>
#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/arch/arm64/cache.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/virtio/backend.h>
#include <zephyr/virtio/virtio.h>
#include <zephyr/virtio/virtqueue.h>

#include <zephyr/logging/log.h>

#include <sys/types.h>

LOG_MODULE_REGISTER(xen_virtio_mmio_backend, 0);

#define DT_DRV_COMPAT xen_virtio_mmio_backend

#define HVM_IOREQSRV_BUFIOREQ_OFF 0
#define PAGE_SHIFT         (__builtin_ctz(CONFIG_MMU_PAGE_SIZE))
#define XEN_GRANT_ADDR_OFF (1ULL << 63)

#define VIRTIO_MMIO_MAGIC             0x74726976
#define VIRTIO_MMIO_SUPPORTED_VERSION 2

#define VIRTIO_BE_MAX_SG 16

ssize_t xs_read(const char *path, char *buf, size_t len);
ssize_t xs_write(const char *path, const char *buf);
ssize_t xs_directory(const char *path, char *buf, size_t len);

struct virtio_mmio_backend_cfg {
	int max_queues;
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

static void bind_interdomain_cb(void *priv)
{
	printk("bind_interdomain_cb %p\n", priv);
}

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

void *acquire_resource(domid_t fe_domid, ioservid_t id, uint64_t frame, uint32_t nr_frames,
		       uintptr_t base)
{
	int rc;
	const uint32_t len = nr_frames << XEN_PAGE_SHIFT;

	xen_pfn_t *gfns;

	gfns = k_malloc(sizeof(xen_pfn_t) * nr_frames);

	gfns[0] = 0;

	printk("gfns %p\n", gfns);
	rc = xendom_acquire_resource(fe_domid, XENMEM_resource_ioreq_server, id,
				     XENMEM_resource_ioreq_server_frame_ioreq(frame), &nr_frames,
				     (xen_pfn_t *)gfns);

	if (rc < 0) {
		return NULL;
	}

	arch_dcache_invd_all();
	barrier_dmem_fence_full();

	for (unsigned i = 0; i < nr_frames; i++) {
		printk("gfns[%d] %llx\n", i, gfns[i]);
	}

	/* ⑥ map the returned MFN into the VA space */
	paddr_t ring_hpa = (paddr_t)gfns[0] << XEN_PAGE_SHIFT;
	k_free(gfns);

	mm_reg_t ring_va;
	device_map(&ring_va, ring_hpa, len, K_MEM_CACHE_NONE);

	printk("%p\n", (void *)ring_va);
	/* add_to_physmap_range は不要 (PFN=IPA=MFN) */
	return (void *)ring_va;
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
	if (!(addr & XEN_GRANT_ADDR_OFF)) {
		printk("no grant addr\n");
		return NULL;
	}

	void *host_page = gnttab_get_page();
	if (!host_page) {
		printk("host_page can't alocate\n");
		return NULL;
	}

	struct gnttab_map_grant_ref op = {
		.host_addr = (uintptr_t)host_page,
		.flags = GNTMAP_host_map, /* 必要なら | GNTMAP_readonly */
		.ref = gref,
		.dom = fe_domid,
	};

	int ret = gnttab_map_refs(&op, 1);
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
		struct virtq_desc *d = &vq->desc[idx];
		const uint64_t gpa = sys_le64_to_cpu(d->addr);
		const uint32_t len = sys_le32_to_cpu(d->len);
		const size_t off = gpa & (XEN_PAGE_SIZE - 1);
		void *va;
		flags = sys_le16_to_cpu(d->flags);
		struct gnttab_unmap_grant_ref unmap;

		va = page_grant(data->fe_domid, gpa, &unmap);

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

static struct virtq *virtio_be_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	struct virtio_mmio_backend_data *data = dev->data;

	return &data->virtqueues[queue_idx];
}

static int virtio_be_isr(const struct device *dev, uint16_t virtqueue_id,
		  int (*cb)(struct virtio_be_req *, void *), void *ctx)
{
	struct virtio_mmio_backend_data *data = dev->data;

	barrier_dmem_fence_full();
	printk("%s\n", __func__);
	struct virtq *vq = virtio_be_get_virtqueue(dev, virtqueue_id);
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
			sys_write8(cnt++, &dst[i]); /* 0,1,2,… とインクリメント */
		}
	}
	barrier_dmem_fence_full();
	/* 書き込んだ総バイト数は req->in.len に既に入っているため
	 * コア側の total_len 計算はそのまま流用できます。
	 */

	return 0; /* 成功 */
}

static void evtchn_cb(void *ptr)
{
	const struct device *dev = ptr;
	struct virtio_mmio_backend_data *data = dev->data;

	printk("evtchn_cb ptr=%p\n", ptr);
	printk("evtchn_cb %p\n", data->shared_iopage);

	struct ioreq *r = &data->shared_iopage->vcpu_ioreq[0];

	if (r->state == STATE_IOREQ_READY) {
		printk("MMIO req: addr=0x%llx size=%u write=%u data=0x%llx\n", r->addr, r->size,
		       r->dir, r->data);

		if (r->dir == IOREQ_READ) {
			printk("data %llx\n", r->data);
			printk("count %u\n", r->count);
			printk("size %u\n", r->size);
			printk("vp_eport %u\n", r->vp_eport);
			printk("state %d\n", r->state);
			printk("df %d\n", r->df);
			printk("type %d\n", r->type);

			switch (r->addr - data->base) {
			case VIRTIO_MMIO_MAGIC_VALUE:
				r->data = VIRTIO_MMIO_MAGIC;
				printk("r/VIRTIO_MMIO_MAGIC %llx\n", r->data);
				break;
			case VIRTIO_MMIO_VERSION:
				r->data = 2;
				printk("r/VIRTIO_MMIO_VERSION %llx\n", r->data);
				break;
			case VIRTIO_MMIO_DEVICE_ID:
				r->data = 4;
				printk("r/VIRTIO_MMIO_DEVICE_ID %llx\n", r->data);
				break;
			case VIRTIO_MMIO_VENDOR_ID:
				r->data = 0xFFFFFFFF; /* 0x7a707972; */
				printk("r/VIRTIO_MMIO_VENDOR_ID %llx\n", r->data);
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
				printk("r/VIRTIO_MMIO_DEVICE_FEATURES %d:%llx\n",
				       data->device_features_sel, r->data);
				break;
			case VIRTIO_MMIO_QUEUE_SIZE_MAX:
				r->data = 2;
				printk("r/VIRTIO_MMIO_QUEUE_SIZE_MAX %llx\n", r->data);
				break;
			case VIRTIO_MMIO_STATUS:
				r->data = data->status;
				printk("r/VIRTIO_MMIO_STATUS %llx\n", r->data);
				break;
			case VIRTIO_MMIO_INTERRUPT_STATUS:
				r->data = data->irq_status;
				data->irq_status = 0;
				printk("r/VIRTIO_MMIO_INTERRUPT_STATUS %llx\n", r->data);
				break;
			case VIRTIO_MMIO_QUEUE_READY:
				r->data = (data->virtqueues[data->queue_sel].desc != NULL);
				printk("r/VIRTIO_MMIO_QUEUE_READY %llx\n", r->data);
				break;
			/* NOT SUPPORTED */
			case VIRTIO_MMIO_SHM_LEN_LOW:
				r->data = -1;
				printk("r/VIRTIO_MMIO_SHM_LEN_LOW %llx\n", r->data);
				break;
			case VIRTIO_MMIO_SHM_LEN_HIGH:
				r->data = -1;
				printk("r/VIRTIO_MMIO_SHM_LEN_HIGH %llx\n", r->data);
				break;
			case VIRTIO_MMIO_CONFIG_GENERATION:
				r->data = -1;
				printk("r/VIRTIO_MMIO_CONFIG_GENERATION %llx\n", r->data);
				break;
			/* WRITE _ONLY */
			case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
			case VIRTIO_MMIO_DRIVER_FEATURES:
			case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
			case VIRTIO_MMIO_QUEUE_SEL:
			case VIRTIO_MMIO_QUEUE_SIZE:
			case VIRTIO_MMIO_QUEUE_NOTIFY:
			case VIRTIO_MMIO_INTERRUPT_ACK:
			case VIRTIO_MMIO_QUEUE_DESC_LOW:
			case VIRTIO_MMIO_QUEUE_DESC_HIGH:
			case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
			case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
			case VIRTIO_MMIO_QUEUE_USED_LOW:
			case VIRTIO_MMIO_QUEUE_USED_HIGH:
			case VIRTIO_MMIO_SHM_SEL:
			default:
				printk("offset %llx\n", r->addr);
				break;
			}

		} else {
			printk("dir %d\n", r->dir);
			switch (r->addr - data->base) {
			case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
				printk("w/VIRTIO_MMIO_DEVICE_FEATURES_SEL %llx\n", r->data);
				data->device_features_sel = (uint8_t)r->data;
				break;
			case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
				printk("w/VIRTIO_MMIO_DRIVER_FEATURES_SEL%llx\n", r->data);
				data->driver_features_sel = (uint8_t)r->data;
				break;
			case VIRTIO_MMIO_DRIVER_FEATURES:
				printk("w/VIRTIO_MMIO_DRIVER_FEATURES_SEL%llx\n", r->data);
				break;
			case VIRTIO_MMIO_INTERRUPT_ACK:
				printk("w/VIRTIO_MMIO_INTERRUPT_ACK %llx\n", r->data);
				data->irq_status &= (uint32_t)~r->data;
				break;
			case VIRTIO_MMIO_STATUS:
				printk("w/VIRTIO_MMIO_STATUS %llx\n", r->data);
				data->status |= r->data;
				break;
			case VIRTIO_MMIO_QUEUE_DESC_LOW:
				printk("w/VIRTIO_MMIO_QUEUE_DESC_LOW %llx\n", r->data);
				data->queue_desc =
					(r->data | (data->queue_desc & 0xFFFFFFFF00000000));
				break;
			case VIRTIO_MMIO_QUEUE_DESC_HIGH:
				printk("w/VIRTIO_MMIO_QUEUE_DESC_HIGH %llx\n", r->data);
				data->queue_desc =
					(r->data << 32 | (data->queue_desc & UINT32_MAX));
				break;
			case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
				printk("w/VIRTIO_MMIO_QUEUE_AVAIL_LOW %llx\n", r->data);
				data->queue_avail =
					(r->data | (data->queue_avail & 0xFFFFFFFF00000000));
				break;
			case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
				printk("w/VIRTIO_MMIO_QUEUE_AVAIL_HIGH %llx\n", r->data);
				data->queue_avail =
					(r->data << 32 | (data->queue_avail & UINT32_MAX));
				break;
			case VIRTIO_MMIO_QUEUE_USED_LOW:
				printk("w/VIRTIO_MMIO_QUEUE_USED_LOW %llx\n", r->data);
				data->queue_used =
					(r->data | (data->queue_used & 0xFFFFFFFF00000000));
				break;
			case VIRTIO_MMIO_QUEUE_USED_HIGH:
				printk("w/VIRTIO_MMIO_QUEUE_USED_HIGH %llx\n", r->data);
				data->queue_used =
					(r->data << 32 | (data->queue_used & UINT32_MAX));
				break;
			case VIRTIO_MMIO_QUEUE_NOTIFY: {
				printk("w/VIRTIO_MMIO_QUEUE_NOTIFY %llx\n", r->data);
				virtio_be_isr(dev, r->data, virtio_test_fill_cb, NULL);
				data->irq_status |= VIRTIO_QUEUE_INTERRUPT;

				dmop_set_irq_level(data->fe_domid, data->fe_irq, 1);
				dmop_set_irq_level(data->fe_domid, data->fe_irq, 0);
			} break;
			case VIRTIO_MMIO_QUEUE_SIZE:
				printk("w/VIRTIO_MMIO_QUEUE_SIZE %llx\n", r->data);
				r->data = 1;
				break;
			case VIRTIO_MMIO_QUEUE_READY: {
				const uint64_t addrs[3] = {data->queue_desc, data->queue_avail,
							   data->queue_used};
				uint64_t host_addr[3];
				uint64_t dev_bus_addr[3];
				grant_handle_t handle[3];
				unsigned long offset;
				struct virtq *q;
				int i, ret;
				bool map_failed = false;

				printk("w/VIRTIO_MMIO_QUEUE_READY %llx\n", r->data);

				q = &data->virtqueues[data->queue_sel];

				/* ３ページまとめてマップ */
				for (i = 0; i < ARRAY_SIZE(addrs); i++) {
					if (!(addrs[i] & XEN_GRANT_ADDR_OFF)) {
						printk("BE: addr[%d] missing grant marker: "
						       "0x%llx\n",
						       i, addrs[i]);
						break;
					}
					grant_ref_t gref =
						(addrs[i] & ~XEN_GRANT_ADDR_OFF) >> PAGE_SHIFT;

					void *host_page = gnttab_get_page();
					if (!host_page) {
						printk("host_page can't alocate\n");
						break;
					}

					struct gnttab_map_grant_ref op = {
						.host_addr = (uintptr_t)host_page,
						.flags = GNTMAP_host_map, /* 必要なら |
									     GNTMAP_readonly */
						.ref = gref,
						.dom = data->fe_domid,
					};

					ret = gnttab_map_refs(&op, 1);
					if (ret < 0 || op.status != 0) {
						printk("virtio-mmio: map gref[%d]=%u failed: %d\n",
						       i, gref, ret);
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
						printk("%s: addr=%p\n", __func__, q->desc);
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
					/* マッピングに失敗したら既に map したものをアンマップ */
					while (--i >= 0) {
						struct gnttab_unmap_grant_ref unmap = {
							.host_addr = host_addr[i],
							.dev_bus_addr = dev_bus_addr[i],
							.handle = handle[i],
						};
						gnttab_unmap_refs(&unmap, 1);
					}
					break;
				}
				q->last_avail_idx = 0;

				printk("virtio-mmio: queue %u mapped → desc@%p avail@%p used@%p\n",
				       data->queue_sel, q->desc, q->avail, q->used);

				uint8_t *descptr = (void *)data->desc_page;

				for (int i = 0; i < sizeof(struct virtq_desc); i++) {
					if (i % 8 == 0) {
						printk("\n");
					}
					printk("%02x ", descptr[i]);
				}

			} break;
			case VIRTIO_MMIO_QUEUE_SEL:
				printk("w/VIRTIO_MMIO_QUEUE_SEL %llx\n", r->data);
				data->queue_sel = r->data;
				break;
			case VIRTIO_MMIO_SHM_SEL:
				printk("w/VIRTIO_MMIO_SHM_SEL %llx\n", r->data);
				break;
			/* READ ONLY */
			case VIRTIO_MMIO_MAGIC_VALUE:
			case VIRTIO_MMIO_VERSION:
			case VIRTIO_MMIO_DEVICE_ID:
			case VIRTIO_MMIO_VENDOR_ID:
			case VIRTIO_MMIO_DEVICE_FEATURES:
			case VIRTIO_MMIO_QUEUE_SIZE_MAX:
			case VIRTIO_MMIO_INTERRUPT_STATUS:
			case VIRTIO_MMIO_SHM_LEN_LOW:
			case VIRTIO_MMIO_SHM_LEN_HIGH:
			case VIRTIO_MMIO_CONFIG_GENERATION:
			default:
				printk("offset %llx\n", r->addr);
				break;
			}
		}

		r->state = STATE_IORESP_READY;

		printk("notify_evtchn %d\n", data->port);
		barrier_dmem_fence_full();
		notify_evtchn(data->port);
	}
}

/**
 * Initialize XenStore event channel: obtain the port, call backend IRQ init,
 * and unmask the event channel.
 */
static int init_evtchn(const struct device *dev, domid_t fe_domid, uint16_t *ports, size_t vcpus)
{
	struct virtio_mmio_backend_data *data = dev->data;
	int ret;
	uint32_t port;

	for (int i = 0; i < vcpus; i++) {
		printk("interdomain dom=%d port=%d\n", fe_domid,
		       data->shared_iopage->vcpu_ioreq[i].vp_eport);

		ret = bind_interdomain_event_channel(fe_domid,
						     data->shared_iopage->vcpu_ioreq[i].vp_eport,
						     bind_interdomain_cb, (void *)dev);
		if (ret < 0) {
			printk("EVTCHNOP_bind_interdomain failed: %d", ret);
			return -EIO;
		}

		printk("vp_eport %d local_port %d\n", data->shared_iopage->vcpu_ioreq[i].vp_eport,
		       ret);

		port = ret;
		ports[i] = port;

		bind_event_channel(port, evtchn_cb, (void *)dev);

		data->port = port;

		/* 5) evtchn unmask */
		unmask_event_channel(port);
	}

	return 0;
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
	xen_events_init();
	struct virtio_mmio_backend_data *data = dev->data;
	int fe_domid;
	int fe_irq;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	fe_domid = query_fe_domid();
	data->fe_domid = fe_domid;
	fe_irq = query_fe_irq();
	data->fe_irq = fe_irq;

	data->base = query_base_addr(fe_domid, 0);

	printk("base %lx\n", data->base);
	printk("fe_domid %d\n", fe_domid);
	printk("fe_irq %d\n", fe_irq);

	ret = dmop_nr_vcpus(fe_domid);
	if (ret < 0) {
		printk("dmop_nr_vcpus err=%d\n", ret);
		ret = -EIO;
		goto end;
	}
	data->vcpus = ret;
	printk("vcpus %d\n", data->vcpus);

	ret = dmop_create_ioreq_server(fe_domid, HVM_IOREQSRV_BUFIOREQ_OFF, &data->srv_id);
	if (ret < 0) {
		printk("dmop_create_ioreq_server err=%d\n", ret);
		ret = -EIO;
		goto end;
	}

	ret = dmop_map_io_range_to_ioreq_server(fe_domid, data->srv_id, 1, data->base,
						data->base + CONFIG_XEN_VIRTIO_MMIO_DEV_SIZE - 1);
	if (ret < 0) {
		printk("dmop_map_io_range_to_ioreq_server err=%d\n", ret);
		goto end;
	}

	uint8_t *ptr = acquire_resource(fe_domid, data->srv_id, 0, 1, data->base);
	if (!ptr) {
		printk("err4 %d\n", ret);
		ret = -EIO;
		goto end;
	}

	ret = dmop_set_ioreq_server_state(fe_domid, data->srv_id, 1);
	if (ret) {
		printk("dmop_set_ioreq_server_state err=%d\n", ret);
		ret = -EIO;
		goto end;
	}

	data->shared_iopage = (void *)ptr;

	char bufx[64];

	int devid = 0;

	sprintf(bufx, "backend/virtio/%d/%d/state", fe_domid, devid);
	ret = xs_write(bufx, "2");
	if (ret) {
		printk("xs_write %d\n", ret);
	}

	barrier_dmem_fence_full();

	while (!data->shared_iopage->vcpu_ioreq[0].vp_eport) {
		k_msleep(1000);
	}

	printk("init_evtchn\n");

	uint16_t port = 0;
	ret = init_evtchn(dev, fe_domid, &port, data->vcpus);
	if (ret) {
		ret = -EIO;
		goto end;
	}

end:
	k_spin_unlock(&data->lock, key);

	printk("%s end %d\n", __func__, ret);

	LOG_INF("%s: backend ready (evtchn=%u)", dev->name, cfg->evtchn_port);
	return ret;
}

#define XEN_VIRTIO_MMIO_BACKEND_INST(idx)                                                          \
	static const struct virtio_mmio_backend_cfg virtio_mmio_backend_cfg_##idx = {              \
		.max_queues = 8,                                                                   \
	};                                                                                         \
	static struct virtq virtio_mmio_backend_virtqueues_##idx[8];                               \
	static struct virtio_mmio_backend_data virtio_mmio_backend_data_##idx = {                  \
		.virtqueues = virtio_mmio_backend_virtqueues_##idx,                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, virtio_mmio_backend_init, NULL,                                 \
			      &virtio_mmio_backend_data_##idx, &virtio_mmio_backend_cfg_##idx,     \
			      POST_KERNEL, 100, &virtio_mmio_backend_api);

DT_INST_FOREACH_STATUS_OKAY(XEN_VIRTIO_MMIO_BACKEND_INST)
