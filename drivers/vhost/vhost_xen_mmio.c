/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/types.h>
#include <inttypes.h>
#include <stdint.h>

#include <xenstore_cli.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/events.h>
#include <zephyr/xen/hvm.h>
#include <zephyr/xen/gnttab.h>
#include <zephyr/xen/memory.h>
#include <zephyr/xen/dmop.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/drivers/vhost.h>
#include <zephyr/drivers/vhost/vringh.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/xen/public/xen.h>
#include <zephyr/xen/public/hvm/ioreq.h>
#include <zephyr/xen/public/hvm/dm_op.h>
#include <zephyr/xen/public/hvm/hvm_op.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(xen_vhost_mmio);

#define DT_DRV_COMPAT xen_vhost_mmio

#define PAGE_SHIFT         (__builtin_ctz(CONFIG_MMU_PAGE_SIZE))
#define XEN_GRANT_ADDR_OFF (1ULL << 63)

#define VIRTIO_MMIO_MAGIC             0x74726976
#define VIRTIO_MMIO_SUPPORTED_VERSION 2

/* The maximum retry period is 12.8 seconds */
#define RETRY_DELAY_BASE_MS   50
#define RETRY_BACKOFF_EXP_MAX 8

#define HEX_64BIT_DIGITS 16

#define LOG_LVL_Q(lvl, str, ...)                                                                   \
	UTIL_CAT(LOG_, lvl)("%s[%u]: " str, __func__, queue_id, ##__VA_ARGS__)
#define LOG_ERR_Q(str, ...) LOG_LVL_Q(ERR, str, ##__VA_ARGS__)
#define LOG_WRN_Q(str, ...) LOG_LVL_Q(WRN, str, ##__VA_ARGS__)
#define LOG_INF_Q(str, ...) LOG_LVL_Q(INF, str, ##__VA_ARGS__)
#define LOG_DBG_Q(str, ...) LOG_LVL_Q(DBG, str, ##__VA_ARGS__)

#define META_PAGES_INDEX(cfg) (cfg->queue_size_max)

enum virtq_parts {
	VIRTQ_DESC = 0,
	VIRTQ_AVAIL,
	VIRTQ_USED,
	NUM_OF_VIRTQ_PARTS,
};

struct vhost_xen_mmio_config {
	k_thread_stack_t *workq_stack;
	size_t workq_stack_size;
	int workq_priority;

	uint16_t num_queues;
	uint16_t queue_size_max;
	uint8_t device_id;
	uint32_t vendor_id;
	uintptr_t base;
	size_t reg_size;

	uint64_t device_features;
};

struct mapped_pages {
	uint64_t gpa;
	uint8_t *buf;
	size_t len;
	struct gnttab_unmap_grant_ref *ops;
	size_t map_count;
	size_t pages;
};

struct mapped_pages_chunk {
	size_t count;
	struct mapped_pages *map;
	bool releasing;
};

struct virtq_callback {
	void (*cb)(const struct device *dev, uint16_t queue_id, void *user_data);
	void *data;
};

struct virtq_context {
	/**
	 * Store pages mapped to descriptors.
	 * Initializer allocate (queue_size_max + 1) statically,
	 * The last one is used to hold the desc, avail, and used
	 * of virtq itself.
	 */
	struct mapped_pages_chunk *pages_chunks;
	struct virtq_callback queue_notify_cb;
	atomic_t queue_size;
	atomic_t queue_ready_notified;
	uint64_t virtq_parts_gpa[NUM_OF_VIRTQ_PARTS];
	struct k_spinlock lock;
};

struct vhost_xen_mmio_data {
	struct k_work_delayable init_work;
	struct k_work_delayable isr_work;
	struct k_work_delayable ready_work;
	struct k_work_q workq;
	const struct device *dev;
	atomic_t initialized;
	atomic_t retry;

	struct xs_watcher watcher;
	evtchn_port_t xs_port;
	evtchn_port_t ioserv_port;
	struct shared_iopage *shared_iopage;
	uint32_t vcpus;

	struct {
		ioservid_t servid;
		domid_t domid;
		uint32_t deviceid;
		uint32_t irq;
		uintptr_t base;
	} fe;

	struct {
		uint64_t driver_features;
		uint8_t device_features_sel;
		uint8_t driver_features_sel;
		atomic_t irq_status;
		atomic_t status;
		atomic_t queue_sel;
	} be;

	atomic_t notify_queue_id; /**< Temporary variable to pass to workq */
	struct virtq_callback queue_ready_cb;
	struct virtq_context *vq_ctx;
};

struct query_param {
	const char *key;
	const char *expected;
};

/**
 * Get the nth string from a null-separated string buffer
 */
static const char *nth_str(const char *buf, size_t len, size_t n)
{
	int cnt = 0;

	if (n == 0) {
		return buf;
	}

	for (size_t i = 0; i < len; i++) {
		if (buf[i] == '\0') {
			cnt++;
		}

		if (cnt == n && (i != (len - 1))) {
			return &buf[i + 1];
		}
	}

	return NULL;
}

/**
 * Query VIRTIO frontend's domid/deviceid from XenStore
 */
static int query_virtio_backend(const struct query_param *params, size_t param_num, domid_t *domid,
				int *deviceid)
{
	char buf[65] = {0};
	const size_t len = ARRAY_SIZE(buf) - 1;
	const char *ptr_i, *ptr_j;
	int i, j;

	const ssize_t len_i = xs_directory("backend/virtio", buf, len, 0);

	if (len_i < 0) {
		return -EIO;
	}
	if (len_i == 6 && strncmp(buf, "ENOENT", len) == 0) {
		return -ENOENT;
	}

	for (i = 0, ptr_i = buf; ptr_i; ptr_i = nth_str(buf, len_i, i++)) {
		char *endptr;

		*domid = strtol(ptr_i, &endptr, 10);
		if (*endptr != '\0') {
			continue;
		}

		snprintf(buf, len, "backend/virtio/%d", *domid);

		const ssize_t len_j = xs_directory(buf, buf, ARRAY_SIZE(buf), 0);

		if (len_j < 0 || strncmp(buf, "ENOENT", ARRAY_SIZE(buf)) == 0) {
			continue;
		}

		for (j = 0, ptr_j = buf; ptr_j; ptr_j = nth_str(buf, len_j, j++)) {
			*deviceid = strtol(ptr_j, &endptr, 10);
			if (*endptr != '\0') {
				continue; /* Skip invalid device ID */
			}

			bool match = true;

			for (size_t k = 0; k < param_num; k++) {
				snprintf(buf, len, "backend/virtio/%d/%d/%s", *domid, *deviceid,
					 params[k].key);
				const ssize_t len_k = xs_read(buf, buf, ARRAY_SIZE(buf), 0);

				if ((len_k < 0) || (strncmp(buf, "ENOENT", ARRAY_SIZE(buf)) == 0) ||
				    (strncmp(params[k].expected, buf, ARRAY_SIZE(buf)) != 0)) {
					match = false;
					break;
				}
			}

			if (match) {
				return 0;
			}
		}
	}

	return -ENOENT;
}

static uintptr_t query_irq(domid_t domid, int deviceid)
{
	char buf[65] = {0};
	size_t len = ARRAY_SIZE(buf) - 1;
	char *endptr;

	snprintf(buf, len, "backend/virtio/%d/%d/irq", domid, deviceid);

	len = xs_read(buf, buf, ARRAY_SIZE(buf) - 1, 0);
	if ((len < 0) || (strncmp(buf, "ENOENT", ARRAY_SIZE(buf)) == 0)) {
		return (uintptr_t)-1;
	}

	uintptr_t irq_val = strtol(buf, &endptr, 10);

	if (*endptr != '\0') {
		return (uintptr_t)-1;
	}

	return irq_val;
}

static int unmap_pages(struct mapped_pages *pages)
{
	int ret = 0;

	if (!pages || !pages->ops) {
		return 0;
	}

	LOG_DBG("%s: pages=%p unmap=%p count=%zu pages=%zu", __func__, pages, pages->ops,
		pages->map_count, pages->pages);

	for (size_t i = 0; i < pages->map_count; i++) {
		LOG_DBG("pages: i=%zu status=%d", i, pages->ops[i].status);
		if (pages->ops[i].status == GNTST_okay) {
			int rc = gnttab_unmap_refs(&pages->ops[i], 1);

			if (rc < 0) {
				LOG_ERR("gnttab_unmap_refs failed: %d", rc);
				ret = rc;
			}
			pages->ops[i].status = GNTST_general_error;
		}
	}

	pages->map_count = 0;

	return ret;
}

static int free_pages_array(struct mapped_pages *pages, size_t len)
{
	int ret = 0;

	if (!pages) {
		return 0;
	}

	for (size_t i = 0; i < len; i++) {
		int rc = unmap_pages(&pages[i]);

		if (rc < 0) {
			LOG_ERR("%s: [%zu] unmap failed: %d", __func__, i, rc);
			ret = rc;
		}

		if (pages[i].ops) {
			k_free(pages[i].ops);
		}

		if (pages[i].buf) {
			rc = gnttab_put_pages(pages[i].buf, pages[i].pages);
			if (rc < 0) {
				LOG_ERR("%s: [%zu] gnttab_put_pages failed: %d", __func__, i, rc);
				ret = rc;
			}
		}
	}

	return ret;
}

static inline k_spinlock_key_t wait_for_chunk_ready(struct virtq_context *ctx,
					 struct mapped_pages_chunk *chunk,
					 k_spinlock_key_t key)
{
	while (chunk->releasing) {
		k_spin_unlock(&ctx->lock, key);
		key = k_spin_lock(&ctx->lock);
	}

	return key;
}

static void reset_queue(const struct device *dev, uint16_t queue_id)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];

	k_spinlock_key_t key = k_spin_lock(&vq_ctx->lock);

	for (size_t i = 0; i <= config->queue_size_max; i++) {
		struct mapped_pages_chunk *chunk = &vq_ctx->pages_chunks[i];

		key = wait_for_chunk_ready(vq_ctx, chunk, key);

		if (chunk->map && chunk->count > 0) {
			const size_t count = chunk->count;

			chunk->releasing = true;
			chunk->map = NULL;
			chunk->count = 0;
			k_spin_unlock(&vq_ctx->lock, key);

			free_pages_array(chunk->map, count);
			k_free(chunk->map);

			key = k_spin_lock(&vq_ctx->lock);
			chunk->releasing = false;
		} else {
			chunk->count = 0;
		}
	}

	vq_ctx->queue_notify_cb.cb = NULL;
	vq_ctx->queue_notify_cb.data = NULL;

	k_spin_unlock(&vq_ctx->lock, key);

	atomic_set(&vq_ctx->queue_size, 0);
	atomic_set(&vq_ctx->queue_ready_notified, 0);
}

static void setup_unmap_info(struct mapped_pages *pages, const struct vhost_buf *bufs,
			     size_t bufs_len, const struct gnttab_map_grant_ref *map_ops)
{
	size_t map_idx = 0;

	for (size_t i = 0; i < bufs_len; i++) {
		const size_t num_pages = (bufs[i].len + XEN_PAGE_SIZE - 1) / XEN_PAGE_SIZE;
		struct mapped_pages *page_info = &pages[i];

		for (size_t j = 0; j < num_pages; j++) {
			const struct gnttab_map_grant_ref *map = &map_ops[map_idx];
			struct gnttab_unmap_grant_ref *unmap = &page_info->ops[j];

			unmap->host_addr = map->host_addr;
			unmap->dev_bus_addr = map->dev_bus_addr;
			unmap->handle = map->handle;
			unmap->status = map->status;
			map_idx++;
		}

		page_info->map_count = num_pages;
		LOG_DBG("%s: range[%zu] map_count=%zu num_pages=%zu "
			"pages=%zu",
			__func__, i, page_info->map_count, num_pages, page_info->pages);
	}
}

static int setup_iovec_mappings(struct mapped_pages *pages, domid_t domid,
				const struct vhost_buf *bufs, size_t bufs_len)
{
	size_t total_map_ops = 0;
	size_t map_idx = 0;
	int ret = 0;

	for (size_t i = 0; i < bufs_len; i++) {
		total_map_ops += (bufs[i].len + XEN_PAGE_SIZE - 1) / XEN_PAGE_SIZE;
	}

	struct gnttab_map_grant_ref *map_ops =
		k_malloc(sizeof(struct gnttab_map_grant_ref) * total_map_ops);

	if (!map_ops) {
		LOG_ERR("k_malloc failed for %zu map operations", total_map_ops);
		return -ENOMEM;
	}

	for (size_t i = 0; i < bufs_len; i++) {
		const size_t num_pages = (bufs[i].len + XEN_PAGE_SIZE - 1) / XEN_PAGE_SIZE;
		struct mapped_pages *page_info = &pages[i];

		for (size_t j = 0; j < num_pages; j++) {
			const uint64_t page_gpa = bufs[i].gpa + (j * XEN_PAGE_SIZE);

			if (!(page_gpa & XEN_GRANT_ADDR_OFF)) {
				LOG_ERR("addr missing grant marker: 0x%" PRIx64, page_gpa);
				ret = -EINVAL;
				goto free_ops;
			}

			map_ops[map_idx].host_addr =
				(uintptr_t)page_info->buf + (j * XEN_PAGE_SIZE);
			map_ops[map_idx].flags = GNTMAP_host_map;
			map_ops[map_idx].ref = (page_gpa & ~XEN_GRANT_ADDR_OFF) >> XEN_PAGE_SHIFT;
			map_ops[map_idx].dom = domid;

			map_idx++;
		}
	}

	ret = gnttab_map_refs(map_ops, total_map_ops);
	if (ret < 0) {
		LOG_ERR("gnttab_map_refs failed: %d", ret);
		goto free_ops;
	}

	/* Check mapping results */
	map_idx = 0;
	for (size_t i = 0; i < bufs_len; i++) {
		const size_t num_pages = (bufs[i].len + XEN_PAGE_SIZE - 1) / XEN_PAGE_SIZE;

		for (size_t j = 0; j < num_pages; j++) {
			const struct gnttab_map_grant_ref *op = &map_ops[map_idx];

			if (op->status != GNTST_okay) {
				LOG_ERR("Mapping failed for range %zu page %zu: status=%d", i, j,
					op->status);
				ret = -EIO;
				goto unmap;
			}
			map_idx++;
		}
	}

	ret = 0;

unmap:
	setup_unmap_info(pages, bufs, bufs_len, map_ops);

	if (ret < 0) {
		unmap_pages(pages);
	}

free_ops:
	if (map_ops) {
		k_free(map_ops);
	}

	return ret;
}

static int init_pages_chunks(const struct device *dev, uint16_t queue_id, uint16_t head,
			     const struct vhost_buf *bufs, size_t bufs_len,
			     struct vhost_iovec *r_iovecs, size_t r_iovecs_len,
			     struct vhost_iovec *w_iovecs, size_t w_iovecs_len, size_t *r_count_out,
			     size_t *w_count_out, size_t total_pages)
{
	struct vhost_xen_mmio_data *data = dev->data;
	struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];
	struct mapped_pages_chunk *chunk = &vq_ctx->pages_chunks[head];
	size_t r_iovecs_count = 0, w_iovecs_count = 0;
	int ret;

	LOG_DBG_Q("%zu bufs, %zu total pages", bufs_len, total_pages);

	/* Reallocate page chunk structure */
	if (!chunk->map || chunk->count < bufs_len) {
		if (chunk->map) {
			free_pages_array(chunk->map, chunk->count);
			k_free(chunk->map);
		}
		chunk->map = k_malloc(sizeof(struct mapped_pages) * bufs_len);
		if (!chunk->map) {
			return -ENOMEM;
		}
		memset(chunk->map, 0, sizeof(struct mapped_pages) * bufs_len);
		chunk->count = bufs_len;
		LOG_DBG_Q("Allocated chunk at %p, count=%zu", chunk->map, chunk->count);
	}

	for (size_t i = 0; i < bufs_len; i++) {
		const size_t num_pages = (bufs[i].len + XEN_PAGE_SIZE - 1) / XEN_PAGE_SIZE;
		struct mapped_pages *page_info = &chunk->map[i];

		/* Allocate or reuse buffer for this range */
		if (!page_info->buf || page_info->pages < num_pages) {
			free_pages_array(page_info, 1);
			memset(page_info, 0, sizeof(struct mapped_pages));

			barrier_dmem_fence_full();

			page_info->len = num_pages * XEN_PAGE_SIZE;
			page_info->gpa = bufs[i].gpa;
			page_info->buf = gnttab_get_pages(num_pages);
			page_info->ops =
				k_malloc(sizeof(struct gnttab_unmap_grant_ref) * num_pages);
			page_info->pages = num_pages;
			page_info->map_count = 0;

			LOG_DBG_Q("range[%zu] allocated pages=%zu num_pages=%zu buf=%p unmap=%p", i,
				  page_info->pages, num_pages, page_info->buf, page_info->ops);

			if (!page_info->buf || !page_info->ops) {
				LOG_ERR_Q("Failed to allocate for range[%zu]: buf=%p unmap=%p", i,
					  page_info->buf, page_info->ops);
				return -ENOMEM;
			}

			for (size_t j = 0; j < num_pages; j++) {
				page_info->ops[j].status = GNTST_general_error;
			}
		}
	}

	ret = setup_iovec_mappings(chunk->map, data->fe.domid, bufs, bufs_len);
	if (ret < 0) {
		return ret;
	}

	k_spinlock_key_t key = k_spin_lock(&vq_ctx->lock);

	key = wait_for_chunk_ready(vq_ctx, chunk, key);

	for (size_t i = 0; i < bufs_len; i++) {
		const bool is_write = bufs[i].is_write;
		const size_t iovecs_len = is_write ? w_iovecs_len : r_iovecs_len;
		const size_t current_count = is_write ? w_iovecs_count : r_iovecs_count;
		struct vhost_iovec *iovec =
			is_write ? &w_iovecs[w_iovecs_count] : &r_iovecs[r_iovecs_count];

		if (current_count >= iovecs_len) {
			LOG_ERR_Q("no more %s iovecs: %zu >= %zu", is_write ? "write" : "read",
				  current_count, iovecs_len);
			k_spin_unlock(&vq_ctx->lock, key);
			ret = -E2BIG;
			break;
		}

		const size_t page_offset = bufs[i].gpa & (XEN_PAGE_SIZE - 1);
		const void *base_buf = chunk->map[i].buf;
		const void *va = (void *)(((uintptr_t)base_buf) + page_offset);

		iovec->iov_base = (void *)va;
		iovec->iov_len = bufs[i].len;

		if (is_write) {
			w_iovecs_count++;
		} else {
			r_iovecs_count++;
		}
	}

	if (ret != -E2BIG) {
		k_spin_unlock(&vq_ctx->lock, key);
	}

	if (ret < 0) {
		LOG_ERR_Q("chunk[%u] initialization failed: %d", head, ret);
	} else {
		*r_count_out = r_iovecs_count;
		*w_count_out = w_iovecs_count;
		LOG_DBG_Q("chunk[%u] init succeed bufs=%zu", head, bufs_len);
	}

	return ret;
}

/**
 * @brief Set up a VirtIO queue for operation
 *
 * Maps the required grant pages for VirtIO ring structures (descriptor,
 * available, and used rings) based on the current queue size. Uses
 * metachain (pages[queue_size_max]) to store the meta pages.
 *
 * @param dev VHost device instance
 * @param queue_id ID of the queue to set up
 * @return 0 on success, negative error code on failure
 */
static int setup_queue(const struct device *dev, uint16_t queue_id)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];
	const size_t queue_size = atomic_get(&data->vq_ctx[queue_id].queue_size);
	const size_t num_pages[] = {DIV_ROUND_UP(16 * queue_size, XEN_PAGE_SIZE),
				    DIV_ROUND_UP(2 * queue_size + 6, XEN_PAGE_SIZE),
				    DIV_ROUND_UP(8 * queue_size + 6, XEN_PAGE_SIZE)};
	int ret = 0;

	struct vhost_buf meta_bufs[NUM_OF_VIRTQ_PARTS];
	struct vhost_iovec dummy_iovecs[NUM_OF_VIRTQ_PARTS];
	size_t dummy_read_count, dummy_write_count;
	size_t total_pages = 0;

	for (size_t i = 0; i < NUM_OF_VIRTQ_PARTS; i++) {
		meta_bufs[i].gpa = vq_ctx->virtq_parts_gpa[i];
		meta_bufs[i].len = num_pages[i] * XEN_PAGE_SIZE;
		meta_bufs[i].is_write = true;
		total_pages += num_pages[i];

		LOG_DBG_Q("Meta range[%zu]: gpa=0x%" PRIx64 " len=%zu pages=%zu", i,
			  meta_bufs[i].gpa, meta_bufs[i].len, num_pages[i]);
	}

	ret = init_pages_chunks(dev, queue_id, META_PAGES_INDEX(config), meta_bufs,
				NUM_OF_VIRTQ_PARTS, dummy_iovecs,
				0, /* No read iovecs needed for meta pages */
				dummy_iovecs, NUM_OF_VIRTQ_PARTS, /* Write iovecs for meta pages */
				&dummy_read_count, &dummy_write_count, total_pages);

	if (ret < 0) {
		LOG_ERR_Q("init_pages_chunks failed: %d", ret);
		return ret;
	}

	k_spinlock_key_t key = k_spin_lock(&vq_ctx->lock);

	for (size_t i = 0; i < config->queue_size_max; i++) {
		struct mapped_pages_chunk *chunk = &vq_ctx->pages_chunks[i];

		key = wait_for_chunk_ready(vq_ctx, chunk, key);

		chunk->map = NULL;
		chunk->count = 0;
	}

	k_spin_unlock(&vq_ctx->lock, key);

	return 0;
}

static void reset_device(const struct device *dev)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;

	data->be.driver_features = 0;
	data->be.device_features_sel = 0;
	data->be.driver_features_sel = 0;
	atomic_set(&data->be.irq_status, 0);
	atomic_set(&data->be.status, 0);
	atomic_set(&data->be.queue_sel, 0);

	for (size_t i = 0; i < config->num_queues; i++) {
		reset_queue(dev, i);
	}
}

static void ioreq_server_read_req(const struct device *dev, struct ioreq *r)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	const size_t addr_offset = r->addr - data->fe.base;

	LOG_DBG("count %u: size: %u vp_eport: %u state: %d df: %d type: %d", r->count, r->size,
		r->vp_eport, r->state, r->df, r->type);

	switch (addr_offset) {
	case VIRTIO_MMIO_MAGIC_VALUE: {
		r->data = VIRTIO_MMIO_MAGIC;
	} break;
	case VIRTIO_MMIO_VERSION: {
		r->data = VIRTIO_MMIO_SUPPORTED_VERSION;
	} break;
	case VIRTIO_MMIO_DEVICE_ID: {
		r->data = config->device_id;
	} break;
	case VIRTIO_MMIO_VENDOR_ID: {
		r->data = config->vendor_id;
	} break;
	case VIRTIO_MMIO_DEVICE_FEATURES: {
		if (data->be.device_features_sel == 0) {
			r->data = (config->device_features & UINT32_MAX);
		} else if (data->be.device_features_sel == 1) {
			r->data = (config->device_features >> 32);
		} else {
			r->data = 0;
		}
	} break;
	case VIRTIO_MMIO_DRIVER_FEATURES: {
		if (data->be.driver_features_sel == 0) {
			r->data = (data->be.driver_features & UINT32_MAX);
		} else if (data->be.driver_features_sel == 1) {
			r->data = (data->be.driver_features >> 32);
		} else {
			r->data = 0;
		}
	} break;
	case VIRTIO_MMIO_QUEUE_SIZE_MAX: {
		r->data = config->queue_size_max;
	} break;
	case VIRTIO_MMIO_STATUS: {
		r->data = atomic_get(&data->be.status);
	} break;
	case VIRTIO_MMIO_INTERRUPT_STATUS: {
		r->data = atomic_clear(&data->be.irq_status);
	} break;
	case VIRTIO_MMIO_QUEUE_READY: {
		r->data = vhost_queue_ready(dev, atomic_get(&data->be.queue_sel));
	} break;
	default: {
		r->data = -1;
	} break;
	}

	LOG_DBG("r/%zx %" PRIx64, addr_offset, r->data);
}

static void ioreq_server_write_req(const struct device *dev, struct ioreq *r)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	const size_t addr_offset = r->addr - data->fe.base;

	LOG_DBG("w/%zx %" PRIx64, addr_offset, r->data);

	switch (addr_offset) {
	case VIRTIO_MMIO_DEVICE_FEATURES_SEL: {
		if (r->data == 0 || r->data == 1) {
			data->be.device_features_sel = (uint8_t)r->data;
		}
	} break;
	case VIRTIO_MMIO_DRIVER_FEATURES_SEL: {
		if (r->data == 0 || r->data == 1) {
			data->be.driver_features_sel = (uint8_t)r->data;
		}
	} break;
	case VIRTIO_MMIO_DRIVER_FEATURES: {
		if (data->be.driver_features_sel == 0) {
			uint64_t *drvfeats = &data->be.driver_features;

			*drvfeats = (r->data | (*drvfeats & 0xFFFFFFFF00000000));
		} else {
			uint64_t *drvfeats = &data->be.driver_features;

			*drvfeats = ((r->data << 32) | (*drvfeats & UINT32_MAX));
		}
	} break;
	case VIRTIO_MMIO_INTERRUPT_ACK: {
		if (r->data) {
			atomic_and(&data->be.irq_status, ~r->data);
		}
	} break;
	case VIRTIO_MMIO_STATUS: {
		if (r->data & BIT(DEVICE_STATUS_FEATURES_OK)) {
			const bool ok = !(data->be.driver_features & ~config->device_features);

			if (ok) {
				atomic_or(&data->be.status, BIT(DEVICE_STATUS_FEATURES_OK));
			} else {
				LOG_ERR("%" PRIx64 " d driver_feats=%" PRIx64
					" device_feats=%" PRIx64,
					r->data, data->be.driver_features, config->device_features);
				atomic_or(&data->be.status, BIT(DEVICE_STATUS_FAILED));
				atomic_and(&data->be.status, ~BIT(DEVICE_STATUS_FEATURES_OK));
			}
		} else if (r->data == 0) {
			reset_device(dev);
		} else {
			atomic_or(&data->be.status, r->data);
		}
	} break;
	case VIRTIO_MMIO_QUEUE_DESC_LOW:
	case VIRTIO_MMIO_QUEUE_DESC_HIGH:
	case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
	case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
	case VIRTIO_MMIO_QUEUE_USED_LOW:
	case VIRTIO_MMIO_QUEUE_USED_HIGH: {
		const uint16_t queue_id = atomic_get(&data->be.queue_sel);

		if (queue_id < config->num_queues) {
			const size_t part = (addr_offset - VIRTIO_MMIO_QUEUE_DESC_LOW) / 0x10;
			const bool hi = !!((addr_offset - VIRTIO_MMIO_QUEUE_DESC_LOW) % 0x10);
			uint64_t *p_gpa = &data->vq_ctx[queue_id].virtq_parts_gpa[part];

			*p_gpa = hi ? ((r->data << 32) | (*p_gpa & UINT32_MAX))
				    : (r->data | (*p_gpa & 0xFFFFFFFF00000000));
		}
	} break;
	case VIRTIO_MMIO_QUEUE_NOTIFY: {
		if (r->data < config->num_queues) {
			atomic_set(&data->notify_queue_id, r->data);
			k_work_schedule_for_queue(&data->workq, &data->isr_work, K_NO_WAIT);
		}
	} break;
	case VIRTIO_MMIO_QUEUE_SIZE: {
		const uint16_t queue_sel = atomic_get(&data->be.queue_sel);

		if (queue_sel < config->num_queues) {
			const bool is_pow2 = (POPCOUNT((int)r->data) == 1);

			if (r->data > 0 && r->data <= config->queue_size_max && is_pow2) {
				atomic_set(&data->vq_ctx[queue_sel].queue_size, r->data);
			} else {
				LOG_ERR("queue_size should be 2^n and <%u: size=%" PRIu64,
					config->queue_size_max, r->data);
				atomic_or(&data->be.status, BIT(DEVICE_STATUS_FAILED));
			}
		}
	} break;
	case VIRTIO_MMIO_QUEUE_READY: {
		const uint16_t queue_sel = atomic_get(&data->be.queue_sel);
		const uint16_t queue_id = queue_sel;

		if (r->data == 0) {
			reset_queue(dev, queue_id);
		} else if (r->data && (queue_sel < config->num_queues)) {
			int err = setup_queue(dev, queue_id);

			if (err < 0) {
				atomic_or(&data->be.status, BIT(DEVICE_STATUS_FAILED));
				LOG_ERR("queue%u setup failed: %d", queue_sel, err);
			} else if (data->queue_ready_cb.cb) {
				data->queue_ready_cb.cb(dev, queue_id, data->queue_ready_cb.data);
			}
		}
	} break;
	case VIRTIO_MMIO_QUEUE_SEL: {
		atomic_set(&data->be.queue_sel, r->data);
	} break;
	default:
		break;
	}
}

static void ioreq_server_cb(void *ptr)
{
	const struct device *dev = ptr;
	struct vhost_xen_mmio_data *data = dev->data;
	struct ioreq *r = &data->shared_iopage->vcpu_ioreq[0];

	if (r->state == STATE_IOREQ_READY) {
		if (r->dir == IOREQ_WRITE) {
			ioreq_server_write_req(dev, r);
		} else {
			ioreq_server_read_req(dev, r);
		}

		r->state = STATE_IORESP_READY;

		barrier_dmem_fence_full();
		notify_evtchn(data->ioserv_port);
	}
}

static void bind_interdomain_nop(void *priv)
{
}

static void xs_notify_handler(const char *path, const char *token, void *param)
{
	const struct device *dev = param;
	struct vhost_xen_mmio_data *data = dev->data;

	if (!atomic_get(&data->initialized) && !k_work_delayable_is_pending(&data->init_work)) {
		k_work_schedule_for_queue(&data->workq, &data->init_work, K_NO_WAIT);
	}
}

static void isr_workhandler(struct k_work *work)
{
	const struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct vhost_xen_mmio_data *data =
		CONTAINER_OF(delayable, struct vhost_xen_mmio_data, isr_work);
	const struct device *dev = data->dev;
	const struct vhost_xen_mmio_config *config = dev->config;

	const uint16_t queue_id = atomic_get(&data->notify_queue_id);
	const struct virtq_context *vq_ctx =
		(queue_id < config->num_queues) ? &data->vq_ctx[queue_id] : NULL;

	if (vq_ctx && vq_ctx->queue_notify_cb.cb) {
		vq_ctx->queue_notify_cb.cb(dev, queue_id, vq_ctx->queue_notify_cb.data);
	}
}

static void ready_workhandler(struct k_work *work)
{
	const struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct vhost_xen_mmio_data *data =
		CONTAINER_OF(delayable, struct vhost_xen_mmio_data, ready_work);
	const struct device *dev = data->dev;
	const struct vhost_xen_mmio_config *config = dev->config;

	for (size_t i = 0; i < config->num_queues; i++) {
		bool queue_ready_notified = atomic_get(&data->vq_ctx[i].queue_ready_notified);

		if (vhost_queue_ready(dev, i) && data->queue_ready_cb.cb && !queue_ready_notified) {
			data->queue_ready_cb.cb(dev, i, data->queue_ready_cb.data);
			atomic_set(&data->vq_ctx[i].queue_ready_notified, 1);
		}
	}
}

static void init_workhandler(struct k_work *work)
{
	struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct vhost_xen_mmio_data *data =
		CONTAINER_OF(delayable, struct vhost_xen_mmio_data, init_work);
	const struct device *dev = data->dev;
	const struct vhost_xen_mmio_config *config = dev->config;
	char baseaddr[HEX_64BIT_DIGITS + 3]; /* add rooms for "0x" and '\0' */
	uint32_t n_frms = 1;
	xen_pfn_t gfn = 0;
	mm_reg_t va;
	int ret;

	if (atomic_get(&data->initialized)) {
		return;
	}

	/*
	 * Using the settings obtained from xenstore can only be checked for
	 * matching the base value.
	 * This means that if multiple FEs try to connect to a BE using the same base address,
	 * they cannot be matched correctly.
	 */

	snprintf(baseaddr, sizeof(baseaddr), "0x%lx", config->base);

	struct query_param params[] = {{
		.key = "base",
		.expected = baseaddr,
	}};

	ret = query_virtio_backend(params, ARRAY_SIZE(params), &data->fe.domid, &data->fe.deviceid);
	if (ret < 0) {
		LOG_INF("%s: failed %d", __func__, ret);
		goto retry;
	}

	data->fe.base = config->base;
	data->fe.irq = query_irq(data->fe.domid, data->fe.deviceid);
	if (data->fe.irq == -1) {
		ret = -EINVAL;
		goto retry;
	}

	LOG_DBG("%u %u %lu", data->fe.domid, data->fe.irq, data->fe.base);

	ret = dmop_nr_vcpus(data->fe.domid);
	if (ret < 0) {
		LOG_ERR("dmop_nr_vcpus err=%d", ret);
		goto retry;
	}
	data->vcpus = ret;

	ret = dmop_create_ioreq_server(data->fe.domid, HVM_IOREQSRV_BUFIOREQ_OFF, &data->fe.servid);
	if (ret < 0) {
		LOG_ERR("dmop_create_ioreq_server err=%d", ret);
		goto retry;
	}

	ret = dmop_map_io_range_to_ioreq_server(data->fe.domid, data->fe.servid, 1, data->fe.base,
						data->fe.base + config->reg_size - 1);
	if (ret < 0) {
		LOG_ERR("dmop_map_io_range_to_ioreq_server err=%d", ret);
		goto retry;
	}

	ret = xendom_acquire_resource(data->fe.domid, XENMEM_resource_ioreq_server, data->fe.servid,
				      XENMEM_resource_ioreq_server_frame_ioreq(0), &n_frms, &gfn);
	if (ret < 0) {
		LOG_ERR("xendom_acquire_resource err=%d", ret);
		goto retry;
	}

	device_map(&va, (gfn << XEN_PAGE_SHIFT), (n_frms << XEN_PAGE_SHIFT), K_MEM_CACHE_NONE);
	data->shared_iopage = (void *)va;

	ret = dmop_set_ioreq_server_state(data->fe.domid, data->fe.servid, 1);
	if (ret) {
		LOG_ERR("dmop_set_ioreq_server_state err=%d", ret);
		goto retry;
	}

	LOG_DBG("bind_interdomain dom=%d remote_port=%d", data->fe.domid,
		data->shared_iopage->vcpu_ioreq[0].vp_eport);

	/* Assume that all interrupts are accepted by cpu0. */
	ret = bind_interdomain_event_channel(data->fe.domid,
					     data->shared_iopage->vcpu_ioreq[0].vp_eport,
					     bind_interdomain_nop, NULL);
	if (ret < 0) {
		LOG_ERR("EVTCHNOP_bind_interdomain[0] err=%d", ret);
		goto retry;
	}
	data->ioserv_port = ret;

	bind_event_channel(data->ioserv_port, ioreq_server_cb, (void *)dev);
	unmask_event_channel(data->ioserv_port);

	LOG_INF("%s: backend ready base=%zx fe.domid=%d irq=%d vcpus=%d shared_iopage=%p "
		"ioserv_port=%d",
		dev->name, data->fe.base, data->fe.domid, data->fe.irq, data->vcpus,
		data->shared_iopage, data->ioserv_port);

	atomic_set(&data->initialized, 1);

	ret = 0;

retry:
	if (ret < 0) {
		const uint32_t retry_count = MIN(RETRY_BACKOFF_EXP_MAX, atomic_inc(&data->retry));

		reset_device(dev);
		k_work_schedule_for_queue(&data->workq, &data->init_work,
					  K_MSEC(RETRY_DELAY_BASE_MS * (1 << retry_count)));
	}

	LOG_INF("exit work_inithandler: %d", ret);
}

static bool vhost_xen_mmio_virtq_is_ready(const struct device *dev, uint16_t queue_id)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	const struct vhost_xen_mmio_data *data = dev->data;
	const struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];
	const size_t queue_size = atomic_get(&vq_ctx->queue_size);

	if (queue_id >= config->num_queues) {
		LOG_ERR_Q("Invalid queue ID");
		return false;
	}

	if (queue_size == 0) {
		return false;
	}

	struct mapped_pages_chunk *meta = &vq_ctx->pages_chunks[META_PAGES_INDEX(config)];

	if (!meta->map || meta->count != NUM_OF_VIRTQ_PARTS) {
		return false;
	}

	for (size_t i = 0; i < meta->count; i++) {
		if (meta->map[i].buf == NULL) {
			return false;
		}

		for (size_t j = 0; j < meta->map[i].map_count; j++) {
			if (meta->map[i].ops[j].status != GNTST_okay) {
				return false;
			}
		}
	}

	return vq_ctx->virtq_parts_gpa[VIRTQ_DESC] != 0 &&
	       vq_ctx->virtq_parts_gpa[VIRTQ_AVAIL] != 0 &&
	       vq_ctx->virtq_parts_gpa[VIRTQ_USED] != 0;
}

static int vhost_xen_mmio_get_virtq(const struct device *dev, uint16_t queue_id, void **parts,
				    size_t *queue_size)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	const struct vhost_xen_mmio_data *data = dev->data;
	const struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];

	if (queue_id >= config->num_queues) {
		LOG_ERR_Q("Invalid queue ID");
		return -EINVAL;
	}

	if (!vhost_xen_mmio_virtq_is_ready(dev, queue_id)) {
		LOG_ERR_Q("not ready");
		return -ENODEV;
	}

	struct mapped_pages_chunk *meta = &vq_ctx->pages_chunks[META_PAGES_INDEX(config)];

	if (!meta->map || meta->count != NUM_OF_VIRTQ_PARTS) {
		return -EINVAL;
	}

	for (size_t i = 0; i < meta->count; i++) {
		parts[i] = meta->map[i].buf + (vq_ctx->virtq_parts_gpa[i] & (XEN_PAGE_SIZE - 1));
	}

	if (!parts[VIRTQ_DESC] || !parts[VIRTQ_AVAIL] || !parts[VIRTQ_USED]) {
		LOG_ERR_Q("failed to get ring base addresses");
		return -EINVAL;
	}

	*queue_size = atomic_get(&vq_ctx->queue_size);

	LOG_DBG_Q("rings desc=%p, avail=%p, used=%p, size=%zu", parts[VIRTQ_DESC],
		  parts[VIRTQ_AVAIL], parts[VIRTQ_USED], *queue_size);

	return 0;
}

static int vhost_xen_mmio_get_driver_features(const struct device *dev, uint64_t *drv_feats)
{
	const struct vhost_xen_mmio_data *data = dev->data;

	*drv_feats = data->be.driver_features;

	return 0;
}

static int vhost_xen_mmio_notify_virtq(const struct device *dev, uint16_t queue_id)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;

	if (queue_id >= config->num_queues) {
		LOG_ERR_Q("Invalid queue ID");
		return -EINVAL;
	}

	atomic_or(&data->be.irq_status, VIRTIO_QUEUE_INTERRUPT);

	dmop_set_irq_level(data->fe.domid, data->fe.irq, 1);
	dmop_set_irq_level(data->fe.domid, data->fe.irq, 0);

	return 0;
}

static int vhost_xen_mmio_set_device_status(const struct device *dev, uint32_t status)
{
	struct vhost_xen_mmio_data *data = dev->data;

	if (!data) {
		return -EINVAL;
	}

	atomic_or(&data->be.status, status);
	atomic_or(&data->be.irq_status, VIRTIO_DEVICE_CONFIGURATION_INTERRUPT);

	dmop_set_irq_level(data->fe.domid, data->fe.irq, 1);
	dmop_set_irq_level(data->fe.domid, data->fe.irq, 0);

	return 0;
}

static int vhost_xen_mmio_release_iovec(const struct device *dev, uint16_t queue_id, uint16_t head)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];
	const size_t queue_size = atomic_get(&vq_ctx->queue_size);
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&vq_ctx->lock);

	if (queue_id >= config->num_queues) {
		LOG_ERR_Q("Invalid queue ID");
		k_spin_unlock(&vq_ctx->lock, key);
		return -EINVAL;
	}

	if (head >= queue_size) {
		LOG_ERR_Q("Invalid head: head=%u >= queue_size=%zu", head, queue_size);
		k_spin_unlock(&vq_ctx->lock, key);
		return -EINVAL;
	}

	struct mapped_pages_chunk *chunk = &vq_ctx->pages_chunks[head];

	if (!chunk->map || chunk->count == 0) {
		LOG_ERR_Q("Head not in use: head=%u", head);

		k_spin_unlock(&vq_ctx->lock, key);
		return -EINVAL;
	}

	key = wait_for_chunk_ready(vq_ctx, chunk, key);

	chunk->releasing = true;

	k_spin_unlock(&vq_ctx->lock, key);

	for (size_t i = 0; i < chunk->count; i++) {
		int rc = unmap_pages(&chunk->map[i]);

		if (rc < 0) {
			LOG_ERR_Q("gnttab_unmap_refs failed for page %zu: %d", i, rc);
			ret = rc;
		}
	}

	key = k_spin_lock(&vq_ctx->lock);

	for (size_t i = 0; i < chunk->count; i++) {
		chunk->map[i].map_count = 0;
	}

	chunk->releasing = false;
	k_spin_unlock(&vq_ctx->lock, key);

	return ret;
}

static int vhost_xen_mmio_prepare_iovec(const struct device *dev, uint16_t queue_id, uint16_t head,
					const struct vhost_buf *bufs, size_t bufs_count,
					struct vhost_iovec *r_iovecs, size_t r_iovecs_count,
					struct vhost_iovec *w_iovecs, size_t w_iovecs_count,
					size_t *read_count, size_t *write_count)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];
	const size_t queue_size = atomic_get(&vq_ctx->queue_size);
	size_t total_pages = 0;
	int ret = 0;

	if (queue_id >= config->num_queues) {
		LOG_ERR_Q("Invalid queue ID");
		ret = -EINVAL;
		goto end;
	}

	if (head >= queue_size) {
		LOG_ERR_Q("Invalid head: head=%u >= queue_size=%zu", head, queue_size);
		ret = -EINVAL;
		goto end;
	}

	for (size_t i = 0; i < bufs_count; i++) {
		const uint64_t start_page = bufs[i].gpa >> XEN_PAGE_SHIFT;
		const uint64_t end_page = (bufs[i].gpa + bufs[i].len - 1) >> XEN_PAGE_SHIFT;

		if (!(bufs[i].gpa & XEN_GRANT_ADDR_OFF)) {
			LOG_ERR_Q("addr missing grant marker: 0x%" PRIx64, bufs[i].gpa);
			ret = -EINVAL;
			goto end;
		}

		total_pages += end_page - start_page + 1;
	}

	if (total_pages == 0) {
		*read_count = 0;
		*write_count = 0;

		return 0;
	}

	LOG_DBG_Q("bufs_count=%zu total_pages=%zu max_read=%zu max_write=%zu", bufs_count,
		  total_pages, r_iovecs_count, w_iovecs_count);

	struct mapped_pages_chunk *chunk = &vq_ctx->pages_chunks[head];

	if (chunk->map && chunk->count > 0 && chunk->map[0].map_count > 0) {
		LOG_WRN("Found unreleased head: %d", head);

		ret = vhost_xen_mmio_release_iovec(dev, queue_id, head);
		if (ret < 0) {
			LOG_ERR_Q("vhost_xen_mmio_release_iovec: failed %d", ret);
			goto end;
		}
	}

	ret = init_pages_chunks(dev, queue_id, head, bufs, bufs_count, r_iovecs, r_iovecs_count,
				w_iovecs, w_iovecs_count, read_count, write_count, total_pages);

end:
	if (ret < 0) {
		*read_count = 0;
		*write_count = 0;
	}

	return ret;
}

static int vhost_xen_mmio_register_virtq_ready_cb(const struct device *dev,
						  void (*callback)(const struct device *dev,
								   uint16_t queue_id,
								   void *user_data),
						  void *user_data)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;

	data->queue_ready_cb.cb = callback;
	data->queue_ready_cb.data = user_data;

	for (size_t i = 0; i < config->num_queues; i++) {
		atomic_set(&data->vq_ctx[i].queue_ready_notified, 0);
	}

	k_work_schedule_for_queue(&data->workq, &data->ready_work, K_NO_WAIT);

	return 0;
}

static int vhost_xen_mmio_register_virtq_notify_cb(const struct device *dev, uint16_t queue_id,
						   void (*callback)(const struct device *dev,
								    uint16_t queue_id,
								    void *user_data),
						   void *user_data)
{
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	struct virtq_context *vq_ctx = &data->vq_ctx[queue_id];

	if (queue_id >= config->num_queues) {
		LOG_ERR_Q("Invalid queue ID");
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&vq_ctx->lock);

	vq_ctx->queue_notify_cb.cb = callback;
	vq_ctx->queue_notify_cb.data = user_data;

	k_spin_unlock(&vq_ctx->lock, key);

	return 0;
}

static const struct vhost_controller_api vhost_driver_xen_mmio_api = {
	.virtq_is_ready = vhost_xen_mmio_virtq_is_ready,
	.get_virtq = vhost_xen_mmio_get_virtq,
	.get_driver_features = vhost_xen_mmio_get_driver_features,
	.set_device_status = vhost_xen_mmio_set_device_status,
	.notify_virtq = vhost_xen_mmio_notify_virtq,
	.prepare_iovec = vhost_xen_mmio_prepare_iovec,
	.release_iovec = vhost_xen_mmio_release_iovec,
	.register_virtq_ready_cb = vhost_xen_mmio_register_virtq_ready_cb,
	.register_virtq_notify_cb = vhost_xen_mmio_register_virtq_notify_cb,
};

static int vhost_xen_mmio_init(const struct device *dev)
{
	const struct k_work_queue_config qcfg = {.name = "vhost-mmio-wq"};
	const struct vhost_xen_mmio_config *config = dev->config;
	struct vhost_xen_mmio_data *data = dev->data;
	char buf[65] = {0};
	int ret;

	data->dev = dev;

	atomic_set(&data->initialized, 0);
	atomic_set(&data->retry, 0);
	atomic_set(&data->be.status, 0);
	atomic_set(&data->be.queue_sel, 0);
	atomic_set(&data->be.irq_status, 0);
	atomic_set(&data->notify_queue_id, 0);
	xen_events_init();

	ret = xs_init();

	if (ret < 0) {
		LOG_INF("xs_init_xenstore failed: %d", ret);
		return ret;
	}

	k_work_init_delayable(&data->init_work, init_workhandler);
	k_work_init_delayable(&data->isr_work, isr_workhandler);
	k_work_init_delayable(&data->ready_work, ready_workhandler);
	k_work_queue_init(&data->workq);
	k_work_queue_start(&data->workq, config->workq_stack, config->workq_stack_size,
			   config->workq_priority, &qcfg);

	xs_watcher_init(&data->watcher, xs_notify_handler, (void *)dev);
	xs_watcher_register(&data->watcher);

	ret = xs_watch("backend/virtio", dev->name, buf, ARRAY_SIZE(buf) - 1, 0);

	if (ret < 0) {
		LOG_INF("xs_watch failed: %d", ret);
		return ret;
	}

	return 0;
}

#define VQCTX_INIT(n, idx)                                                                         \
	{                                                                                          \
		.pages_chunks = vhost_xen_mmio_pages_chunks_##idx[n],                              \
	}

#define Q_NUM(idx)    DT_INST_PROP_OR(idx, num_queues, 1)
#define Q_SZ_MAX(idx) DT_INST_PROP_OR(idx, queue_size_max, 1)

#define VHOST_XEN_MMIO_INST(idx)                                                                   \
	static K_THREAD_STACK_DEFINE(workq_stack_##idx, DT_INST_PROP_OR(idx, stack_size, 4096));   \
	static const struct vhost_xen_mmio_config vhost_xen_mmio_config_##idx = {                  \
		.queue_size_max = DT_INST_PROP_OR(idx, queue_size_max, 1),                         \
		.num_queues = DT_INST_PROP_OR(idx, num_queues, 1),                                 \
		.device_id = DT_INST_PROP(idx, device_id),                                         \
		.vendor_id = DT_INST_PROP_OR(idx, vendor_id, 0),                                   \
		.base = DT_INST_PROP(idx, base),                                                   \
		.reg_size = XEN_PAGE_SIZE,                                                         \
		.workq_stack = (k_thread_stack_t *)&workq_stack_##idx,                             \
		.workq_stack_size = K_THREAD_STACK_SIZEOF(workq_stack_##idx),                      \
		.workq_priority = DT_INST_PROP_OR(idx, priority, 0),                               \
		.device_features = BIT(VIRTIO_F_VERSION_1) | BIT(VIRTIO_F_ACCESS_PLATFORM),        \
	};                                                                                         \
	struct mapped_pages_chunk vhost_xen_mmio_pages_chunks_##idx[Q_NUM(idx)]                    \
								   [Q_SZ_MAX(idx) + 1];            \
	static struct virtq_context vhost_xen_mmio_vq_ctx_##idx[Q_NUM(idx)] = {                    \
		LISTIFY(Q_NUM(idx), VQCTX_INIT, (,), idx),                                         \
	};                                                                                         \
	static struct vhost_xen_mmio_data vhost_xen_mmio_data_##idx = {                            \
		.vq_ctx = vhost_xen_mmio_vq_ctx_##idx,                                             \
		.fe.base = -1,                                                                     \
		.ioserv_port = -1,                                                                 \
		.fe.servid = -1,                                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, vhost_xen_mmio_init, NULL, &vhost_xen_mmio_data_##idx,          \
			      &vhost_xen_mmio_config_##idx, POST_KERNEL, 100,                      \
			      &vhost_driver_xen_mmio_api);

DT_INST_FOREACH_STATUS_OKAY(VHOST_XEN_MMIO_INST)
