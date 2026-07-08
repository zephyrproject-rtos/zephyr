/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel/internal/mm.h>

#define DT_DRV_COMPAT        virtio_blk
#define VIRTIO_BLK_QUEUE_IDX 0

/*
 * The single request virtqueue holds one descriptor for the request header, up
 * to VIRTIO_BLK_MAX_DATA_SEGS scatter-gather data descriptors, and one for the
 * status byte.  VIRTIO_BLK_MAX_DATA_SEGS also bounds the size of the on-stack
 * scatter-gather segment array.
 */
#if defined(CONFIG_MMU)
#define VIRTIO_BLK_MAX_DATA_SEGS CONFIG_DISK_VIRTIO_BLK_MAX_SEGMENTS
#else
#define VIRTIO_BLK_MAX_DATA_SEGS 1
#endif
#define VIRTIO_BLK_REQUEST_QUEUE_SIZE Z_POW2_CEIL(VIRTIO_BLK_MAX_DATA_SEGS + 2)

BUILD_ASSERT(VIRTIO_BLK_REQUEST_QUEUE_SIZE <= 256,
	     "VIRTIO block request virtqueue must not exceed 256 entries");

/* Feature bits from VirtIO 1.3, section 5.2.3 */
#define VIRTIO_BLK_F_SIZE_MAX     1
#define VIRTIO_BLK_F_SEG_MAX      2
#define VIRTIO_BLK_F_GEOMETRY     4
#define VIRTIO_BLK_F_RO           5
#define VIRTIO_BLK_F_BLK_SIZE     6
#define VIRTIO_BLK_F_FLUSH        9
#define VIRTIO_BLK_F_TOPOLOGY     10
#define VIRTIO_BLK_F_CONFIG_WCE   11
#define VIRTIO_BLK_F_MQ           12
#define VIRTIO_BLK_F_DISCARD      13
#define VIRTIO_BLK_F_WRITE_ZEROES 14
#define VIRTIO_BLK_F_LIFETIME     15
#define VIRTIO_BLK_F_SECURE_ERASE 16
#define VIRTIO_BLK_F_ZONED        17

#define VIRTIO_BLK_T_IN    0
#define VIRTIO_BLK_T_OUT   1
#define VIRTIO_BLK_T_FLUSH 4

#define VIRTIO_BLK_S_OK     0
#define VIRTIO_BLK_S_IOERR  1
#define VIRTIO_BLK_S_UNSUPP 2

/* The VirtIO block protocol always addresses the device in 512-byte sectors. */
#define VIRTIO_BLK_SECTOR_SIZE 512U

BUILD_ASSERT((CONFIG_DISK_VIRTIO_BLK_SECTOR_SIZE % 512) == 0,
	     "DISK_VIRTIO_BLK_SECTOR_SIZE must be a multiple of 512");
#if defined(CONFIG_MMU)
BUILD_ASSERT(CONFIG_DISK_VIRTIO_BLK_SECTOR_SIZE <= CONFIG_MMU_PAGE_SIZE,
	     "DISK_VIRTIO_BLK_SECTOR_SIZE must not exceed the MMU page size");
#endif

LOG_MODULE_REGISTER(disk_virtio_blk, CONFIG_DISK_VIRTIO_BLK_LOG_LEVEL);

/*
 * Header prefix of struct virtio_blk_req (VirtIO 1.3, section 5.2.6).
 * The data payload and the status byte are placed in separate virtqueue
 * descriptors, so only the first three fields are represented here.
 */
struct virtio_blk_req_hdr {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
};

/*
 * Partial device-specific configuration layout from VirtIO 1.3, section 5.2.
 * Fields beyond blk_size are omitted because the driver does not implement
 * the corresponding features.
 */
struct virtio_blk_config {
	uint64_t capacity;
	uint32_t size_max;
	uint32_t seg_max;
	struct virtio_blk_geometry {
		uint16_t cylinders;
		uint8_t heads;
		uint8_t sectors;
	} geometry;
	uint32_t blk_size;
} __packed;

struct virtio_blk_drv_config {
	const struct device *vdev;
	const char *disk_name;
};

struct virtio_blk_data {
	const struct device *vdev;
	struct disk_info info;
	struct k_sem sem;
	struct k_mutex lock;
	uint64_t capacity;
	uint32_t sector_size;
	uint32_t sectors_per_block;
	uint16_t max_data_segs;
	bool read_only;
	bool flush;
	uint8_t status;
	struct virtio_blk_req_hdr req_hdr;
};

/*
 * A chunk is a physically-contiguous piece of a caller buffer within a single
 * MMU page.  One sector may span two pages, so it can produce up to two chunks;
 * physically-adjacent chunks are coalesced into a scatter-gather segment.
 */
struct virtio_blk_chunk {
	uintptr_t phys;
	size_t len;
};

extern int arch_page_phys_get(void *virt, uintptr_t *phys);
static int virtio_blk_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff);
static const struct disk_operations virtio_blk_ops;

static uint16_t virtio_blk_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *opaque)
{
	struct virtio_blk_data *data = opaque;

	if (q_index == VIRTIO_BLK_QUEUE_IDX) {
		uint16_t request_queue_size = Z_POW2_CEIL(data->max_data_segs + 2U);

		return MIN(request_queue_size, q_size_max);
	}

	return 0;
}

static void virtio_blk_complete(void *priv, uint32_t used_len)
{
	struct virtio_blk_data *data = priv;

	ARG_UNUSED(used_len);

	k_sem_give(&data->sem);
}

/*
 * The start_sector/num_sector arguments are in logical-block units (the sector
 * size reported to disk_access); the device capacity is in 512-byte VirtIO
 * sectors, so scale by sectors_per_block before comparing.
 */
static inline bool virtio_blk_sector_range_valid(struct virtio_blk_data *data, uint32_t start,
						 uint32_t count)
{
	uint64_t end = ((uint64_t)start + count) * data->sectors_per_block;

	return end <= data->capacity;
}

static int virtio_blk_negotiate_feature(const struct device *vdev, int bit, bool *negotiated)
{
	int ret;

	*negotiated = false;
	if (!virtio_read_device_feature_bit(vdev, bit)) {
		return 0;
	}

	ret = virtio_write_driver_feature_bit(vdev, bit, true);
	if (ret != 0) {
		LOG_ERR("failed to enable feature bit %d: %d", bit, ret);
		return ret;
	}

	*negotiated = true;

	return 0;
}

/*
 * Submit a single descriptor chain: request header, zero or more caller data
 * segments, and the status byte.  Serialized by data->lock because req_hdr,
 * status and the completion semaphore are shared per device (single in-flight
 * request); disk_access does not serialize concurrent read/write callers.
 *
 * sector is expressed in 512-byte VirtIO sectors, as required by the protocol.
 */
static int virtio_blk_submit(struct virtio_blk_data *data, uint32_t type, uint64_t sector,
			     struct virtq_buf *data_segs, uint16_t n_segs, bool is_write)
{
	struct virtq *vq = virtio_get_virtqueue(data->vdev, VIRTIO_BLK_QUEUE_IDX);
	struct virtq_buf bufs[VIRTIO_BLK_MAX_DATA_SEGS + 2];
	uint16_t dev_readable_count;
	uint16_t bufs_count = 0;
	uint8_t status;
	int ret;

	if (vq == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->req_hdr.type = sys_cpu_to_le32(type);
	data->req_hdr.reserved = 0;
	data->req_hdr.sector = sys_cpu_to_le64(sector);
	data->status = 0xff;

	bufs[bufs_count].addr = &data->req_hdr;
	bufs[bufs_count].len = sizeof(data->req_hdr);
	bufs_count++;

	for (uint16_t i = 0; i < n_segs; i++) {
		bufs[bufs_count++] = data_segs[i];
	}

	bufs[bufs_count].addr = &data->status;
	bufs[bufs_count].len = sizeof(data->status);
	bufs_count++;

	/*
	 * The header, and for a write all data segments, are device-readable;
	 * the status byte (and, for a read, the data) is device-writable.
	 */
	dev_readable_count = is_write ? (uint16_t)(1U + n_segs) : 1U;

	ret = virtq_add_buffer_chain(vq, bufs, bufs_count, dev_readable_count, virtio_blk_complete,
				     data, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("failed to submit request: %d", ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	virtio_notify_virtqueue(data->vdev, VIRTIO_BLK_QUEUE_IDX);
	k_sem_take(&data->sem, K_FOREVER);

	if (data->status != VIRTIO_BLK_S_OK) {
		status = data->status;
		k_mutex_unlock(&data->lock);
		LOG_ERR("request failed, status=%u", status);

		return (status == VIRTIO_BLK_S_UNSUPP) ? -ENOTSUP : -EIO;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

#if defined(CONFIG_MMU)
/*
 * The virtqueue layer translates buffer addresses with k_mem_phys_addr(), which
 * is only valid for the kernel's linear RAM mapping.  A buffer outside that
 * mapping (e.g. the thread-stack work area FatFS passes to f_mkfs()) is not
 * necessarily physically contiguous, so it cannot be handed over as a single
 * descriptor.
 *
 * Walk the sectors starting at buf, translate each page with
 * arch_page_phys_get() to its real physical address, and coalesce physically
 * adjacent chunks into scatter-gather segments.  Each segment is expressed as its
 * linear-mapping alias k_mem_virt_addr(phys); the virtqueue layer's
 * k_mem_phys_addr() then reproduces the correct physical address.  This is
 * zero-copy and needs no shared bounce buffer.
 *
 * At most max_segs segments are produced.  When the budget is exhausted the
 * function stops on a sector boundary, so the caller can split a too-fragmented
 * transfer across several device requests.  Returns the number of sectors
 * covered (>= 1) and stores the segment count in *n_out, or a negative errno.
 */

/*
 * Translate one page to its physical address, using a one-page cache to avoid
 * repeated lookups when a single sector spans two pages.
 */
static int virtio_blk_page_phys(uintptr_t page, uintptr_t *cached_page, uintptr_t *cached_phys,
				bool *have_cache, uintptr_t *phys)
{
	if (*have_cache && page == *cached_page) {
		*phys = *cached_phys;
		return 0;
	}

	if (arch_page_phys_get(UINT_TO_POINTER(page), phys) != 0) {
		return -EIO;
	}

	*cached_page = page;
	*cached_phys = *phys;
	*have_cache = true;
	return 0;
}

/*
 * Split one sector, starting at sector offset sector_idx within buf, into at most two
 * physically-contiguous chunks.  A sector may cross a page boundary, producing two
 * chunks; chunks that happen to be physically adjacent are merged into one.
 * Returns the number of chunks produced, or a negative errno.
 */
static int virtio_blk_sector_to_chunks(const uint8_t *buf, uint32_t sector_size,
				       uint32_t sector_idx, struct virtio_blk_chunk *chunks,
				       uintptr_t *cached_page, uintptr_t *cached_phys,
				       bool *have_cache)
{
	int nchunks = 0;
	size_t sec_off = 0;

	while (sec_off < sector_size) {
		/* Virtual address of the next byte to be covered in this sector. */
		uintptr_t va = POINTER_TO_UINT(buf) + (size_t)sector_idx * sector_size + sec_off;
		/* Page containing that byte, and the offset within the page. */
		uintptr_t page = ROUND_DOWN(va, CONFIG_MMU_PAGE_SIZE);
		size_t page_off = va - page;
		/* Length remaining in the sector or to the end of the page, whichever is smaller.
		 */
		size_t contig_len =
			MIN((size_t)sector_size - sec_off, (size_t)CONFIG_MMU_PAGE_SIZE - page_off);
		uintptr_t phys;
		int ret;

		ret = virtio_blk_page_phys(page, cached_page, cached_phys, have_cache, &phys);
		if (ret != 0) {
			return ret;
		}
		phys += page_off;

		/* Merge with the previous chunk if the two are physically adjacent. */
		if (nchunks > 0 && chunks[nchunks - 1].phys + chunks[nchunks - 1].len == phys) {
			chunks[nchunks - 1].len += contig_len;
		} else {
			chunks[nchunks].phys = phys;
			chunks[nchunks].len = contig_len;
			nchunks++;
		}
		sec_off += contig_len;
	}

	return nchunks;
}

/*
 * Append or coalesce one physical chunk into the output segment array.
 *
 * If the new chunk is physically adjacent to the previous segment, extend the
 * previous segment instead of consuming a new descriptor.  Otherwise start a
 * new segment at segs[*n_segs].  The segment address is expressed via its
 * linear-mapping alias so the virtqueue layer can translate it back.
 */
static void virtio_blk_commit_chunk(struct virtq_buf *segs, uint16_t *n_segs,
				    uintptr_t *prev_end_phys, uintptr_t chunk_phys,
				    size_t chunk_len)
{
	/* Coalesce with the previous segment when the new chunk is contiguous. */
	if (*n_segs > 0 && chunk_phys == *prev_end_phys) {
		segs[*n_segs - 1].len += (uint32_t)chunk_len;
	} else {
		segs[*n_segs].addr = k_mem_virt_addr(chunk_phys);
		segs[*n_segs].len = (uint32_t)chunk_len;
		(*n_segs)++;
	}

	/* Record the end of the (possibly extended) last segment. */
	*prev_end_phys = chunk_phys + chunk_len;
}

static int virtio_blk_build_segs(const uint8_t *buf, uint32_t sector_size, uint32_t num_sector,
				 struct virtq_buf *segs, uint16_t max_segs, uint16_t *n_out)
{
	struct virtio_blk_chunk chunks[2];
	uintptr_t prev_end_phys = 0;
	uintptr_t cached_page = 0;
	uintptr_t cached_phys = 0;
	bool have_cache = false;
	uint16_t n_segs = 0;
	int nchunks;
	int new_segs;

	/*
	 * Walk the caller buffer sector by sector, translate each page to its
	 * physical address, and coalesce physically adjacent chunks into segments.
	 * Stop on a sector boundary when the descriptor budget is exhausted so the
	 * caller can submit what we have built and continue with a new request.
	 */
	for (uint32_t sector_idx = 0; sector_idx < num_sector; sector_idx++) {
		nchunks = virtio_blk_sector_to_chunks(buf, sector_size, sector_idx, chunks,
						      &cached_page, &cached_phys, &have_cache);
		if (nchunks < 0) {
			return nchunks;
		}

		/*
		 * new_segs is the number of additional segment slots this sector would
		 * consume after coalescing with the previous segment (if any).
		 */
		new_segs = nchunks;
		if (n_segs > 0 && chunks[0].phys == prev_end_phys) {
			new_segs -= 1;
		}

		/* Not enough descriptors left for this sector: stop here. */
		if ((uint32_t)n_segs + (uint32_t)new_segs > max_segs) {
			if (sector_idx == 0) {
				/* A single sector needs more segments than the budget. */
				return -ENOBUFS;
			}
			*n_out = n_segs;
			return (int)sector_idx;
		}

		for (int c = 0; c < nchunks; c++) {
			virtio_blk_commit_chunk(segs, &n_segs, &prev_end_phys, chunks[c].phys,
						chunks[c].len);
		}
	}

	*n_out = n_segs;
	return (int)num_sector;
}

static int virtio_blk_transfer_mmu(struct virtio_blk_data *data, uint32_t type,
				   uint32_t start_sector, const uint8_t *buf, uint32_t num_sector,
				   bool is_write)
{
	struct virtq *vq = virtio_get_virtqueue(data->vdev, VIRTIO_BLK_QUEUE_IDX);
	struct virtq_buf segs[VIRTIO_BLK_MAX_DATA_SEGS];
	uint32_t sector_size = data->sector_size;
	uint16_t max_data_descs;
	uint32_t done = 0;

	if (vq == NULL) {
		return -ENODEV;
	}

	/* Reserve one descriptor for the header and one for the status byte. */
	max_data_descs = (uint16_t)MIN((uint32_t)data->max_data_segs,
				       (vq->num > 2U) ? (uint32_t)(vq->num - 2U) : 1U);

	while (done < num_sector) {
		uint16_t n_segs = 0;
		int n_sectors;
		int ret;

		n_sectors = virtio_blk_build_segs(buf + (size_t)done * sector_size, sector_size,
						  num_sector - done, segs, max_data_descs, &n_segs);
		if (n_sectors < 0) {
			return n_sectors;
		}

		ret = virtio_blk_submit(data, type,
					(uint64_t)(start_sector + done) * data->sectors_per_block,
					segs, n_segs, is_write);
		if (ret != 0) {
			return ret;
		}

		done += (uint32_t)n_sectors;
	}

	return 0;
}
#endif /* CONFIG_MMU */

/*
 * Run a read or write transfer.  The caller's buffer is passed to the device
 * directly (zero copy) as one or more scatter-gather segments; a buffer outside
 * the linear RAM mapping is translated page-by-page rather than copied.
 *
 * start_sector/num_sector are in logical-block units; the device is addressed
 * in 512-byte VirtIO sectors, so scale by sectors_per_block on submission.
 */
static int virtio_blk_transfer(struct virtio_blk_data *data, uint32_t type, uint32_t start_sector,
			       const uint8_t *buf, uint32_t num_sector, bool is_write)
{
	if (num_sector == 0U) {
		return 0;
	}

#if defined(CONFIG_MMU)
	return virtio_blk_transfer_mmu(data, type, start_sector, buf, num_sector, is_write);
#else
	struct virtq_buf seg;
	uint32_t sector_size = data->sector_size;

	/* Without an MMU every buffer is identity-mapped and contiguous. */
	seg.addr = (void *)buf;
	seg.len = (uint32_t)((size_t)num_sector * sector_size);

	return virtio_blk_submit(data, type, (uint64_t)start_sector * data->sectors_per_block, &seg,
				 1, is_write);
#endif
}

static int virtio_blk_dev_init(const struct device *dev)
{
	const struct virtio_blk_drv_config *cfg = dev->config;
	struct virtio_blk_data *data = dev->data;
	const uint8_t *dev_cfg_bytes;
	struct virtq *vq;
	bool have_blk_size = false;
	bool have_seg_max = false;
	uint32_t sector_size = CONFIG_DISK_VIRTIO_BLK_SECTOR_SIZE;
	uint32_t seg_max = 0;
	int ret;

	if (!device_is_ready(cfg->vdev)) {
		LOG_ERR("parent VirtIO device not ready");
		return -ENODEV;
	}

	data->vdev = cfg->vdev;
	data->info.name = cfg->disk_name;
	data->info.ops = &virtio_blk_ops;
	data->info.dev = dev;
	k_sem_init(&data->sem, 0, 1);
	k_mutex_init(&data->lock);

	/*
	 * Negotiate feature bits before committing them; the device-specific
	 * configuration space must only be read once FEATURES_OK is set (see
	 * VirtIO 1.3, section 3.1.1).
	 */
	ret = virtio_blk_negotiate_feature(cfg->vdev, VIRTIO_BLK_F_RO, &data->read_only);
	if (ret != 0) {
		return ret;
	}

	ret = virtio_blk_negotiate_feature(cfg->vdev, VIRTIO_BLK_F_SEG_MAX, &have_seg_max);
	if (ret != 0) {
		return ret;
	}

	ret = virtio_blk_negotiate_feature(cfg->vdev, VIRTIO_BLK_F_FLUSH, &data->flush);
	if (ret != 0) {
		return ret;
	}

	ret = virtio_blk_negotiate_feature(cfg->vdev, VIRTIO_BLK_F_BLK_SIZE, &have_blk_size);
	if (ret != 0) {
		return ret;
	}

	ret = virtio_commit_feature_bits(cfg->vdev);
	if (ret != 0) {
		return ret;
	}

	dev_cfg_bytes = virtio_get_device_specific_config(cfg->vdev);
	if (dev_cfg_bytes == NULL) {
		LOG_ERR("missing device-specific configuration");
		return -ENODEV;
	}

	data->capacity = sys_get_le64(dev_cfg_bytes + offsetof(struct virtio_blk_config, capacity));

	/*
	 * seg_max is only valid when VIRTIO_BLK_F_SEG_MAX was negotiated.  It
	 * caps the number of data descriptors in one request; the request header
	 * and status byte use separate descriptors.
	 */
	if (have_seg_max) {
		seg_max = sys_get_le32(dev_cfg_bytes + offsetof(struct virtio_blk_config, seg_max));
		if (seg_max == 0U) {
			LOG_ERR("unsupported seg_max %u", seg_max);
			return -ENOTSUP;
		}
		data->max_data_segs = (uint16_t)MIN((uint32_t)VIRTIO_BLK_MAX_DATA_SEGS, seg_max);
	} else {
		data->max_data_segs = VIRTIO_BLK_MAX_DATA_SEGS;
	}

	/*
	 * The VirtIO block protocol always addresses the device in 512-byte
	 * sectors and reports capacity in those units (VirtIO 1.3, section 5.2).
	 * The logical block size exposed to disk_access is the device-advertised
	 * blk_size when VIRTIO_BLK_F_BLK_SIZE was negotiated, otherwise the
	 * configured fallback.  Transfers are converted to 512-byte VirtIO
	 * sectors via sectors_per_block.
	 */
	if (have_blk_size) {
		sector_size =
			sys_get_le32(dev_cfg_bytes + offsetof(struct virtio_blk_config, blk_size));
	}

	if (sector_size < VIRTIO_BLK_SECTOR_SIZE || (sector_size % VIRTIO_BLK_SECTOR_SIZE) != 0U) {
		LOG_ERR("unsupported block size %u (must be a multiple of 512)", sector_size);
		return -ENOTSUP;
	}

#if defined(CONFIG_MMU)
	if (sector_size > CONFIG_MMU_PAGE_SIZE) {
		LOG_ERR("block size %u exceeds MMU page size %u", sector_size,
			(uint32_t)CONFIG_MMU_PAGE_SIZE);
		return -ENOTSUP;
	}
#endif

	data->sector_size = sector_size;
	data->sectors_per_block = sector_size / VIRTIO_BLK_SECTOR_SIZE;
	ret = virtio_init_virtqueues(cfg->vdev, 1, virtio_blk_enum_queues_cb, data);
	if (ret != 0) {
		return ret;
	}

	/*
	 * A normal request needs at least one header, one data and one status
	 * descriptor.  Reject a virtqueue that cannot hold the minimum chain.
	 */
	vq = virtio_get_virtqueue(cfg->vdev, VIRTIO_BLK_QUEUE_IDX);
	if (vq == NULL || vq->num < 3U) {
		LOG_ERR("request virtqueue missing or too small");
		return -ENOTSUP;
	}

	virtio_finalize_init(cfg->vdev);

	LOG_INF("capacity %llu bytes, block size %u bytes", data->capacity * VIRTIO_BLK_SECTOR_SIZE,
		data->sector_size);

	return disk_access_register(&data->info);
}

static int virtio_blk_disk_init(struct disk_info *disk)
{
	return virtio_blk_disk_ioctl(disk, DISK_IOCTL_CTRL_INIT, NULL);
}

static int virtio_blk_disk_status(struct disk_info *disk)
{
	struct virtio_blk_data *data = CONTAINER_OF(disk, struct virtio_blk_data, info);

	return data->read_only ? DISK_STATUS_WR_PROTECT : DISK_STATUS_OK;
}

static int virtio_blk_disk_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
				uint32_t num_sector)
{
	struct virtio_blk_data *data = CONTAINER_OF(disk, struct virtio_blk_data, info);

	if (!virtio_blk_sector_range_valid(data, start_sector, num_sector)) {
		return -EINVAL;
	}

	return virtio_blk_transfer(data, VIRTIO_BLK_T_IN, start_sector, data_buf, num_sector,
				   false);
}

static int virtio_blk_disk_write(struct disk_info *disk, const uint8_t *data_buf,
				 uint32_t start_sector, uint32_t num_sector)
{
	struct virtio_blk_data *data = CONTAINER_OF(disk, struct virtio_blk_data, info);

	if (data->read_only) {
		return -EROFS;
	}

	if (!virtio_blk_sector_range_valid(data, start_sector, num_sector)) {
		return -EINVAL;
	}

	return virtio_blk_transfer(data, VIRTIO_BLK_T_OUT, start_sector, data_buf, num_sector,
				   true);
}

static int virtio_blk_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct virtio_blk_data *data = CONTAINER_OF(disk, struct virtio_blk_data, info);

	if (buff == NULL &&
	    (cmd == DISK_IOCTL_GET_SECTOR_COUNT || cmd == DISK_IOCTL_GET_SECTOR_SIZE ||
	     cmd == DISK_IOCTL_GET_ERASE_BLOCK_SZ)) {
		return -EINVAL;
	}

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT: {
		uint64_t count = data->capacity / data->sectors_per_block;

		if (count > UINT32_MAX) {
			LOG_WRN("capacity exceeds 32-bit range");
		}
		*(uint32_t *)buff = (uint32_t)MIN(count, (uint64_t)UINT32_MAX);
		break;
	}
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = data->sector_size;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buff = 1U;
		break;
	case DISK_IOCTL_CTRL_INIT:
	case DISK_IOCTL_CTRL_DEINIT:
		break;
	case DISK_IOCTL_CTRL_SYNC:
		/*
		 * Only issue a flush when the device advertised a writeback cache via
		 * VIRTIO_BLK_F_FLUSH.  Without it, there is nothing to flush; per
		 * VirtIO 1.3 §5.2.5.1 we may assume a writethrough cache and report
		 * success without sending a FLUSH request.
		 */
		if (!data->flush) {
			return 0;
		}
		return virtio_blk_submit(data, VIRTIO_BLK_T_FLUSH, 0, NULL, 0, false);
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct disk_operations virtio_blk_ops = {
	.init = virtio_blk_disk_init,
	.status = virtio_blk_disk_status,
	.read = virtio_blk_disk_read,
	.write = virtio_blk_disk_write,
	.ioctl = virtio_blk_disk_ioctl,
};

#define VIRTIO_BLK_DEFINE(inst)                                                                    \
	static struct virtio_blk_data virtio_blk_data_##inst;                                      \
	static const struct virtio_blk_drv_config virtio_blk_drv_config_##inst = {                 \
		.vdev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                       \
		.disk_name = DT_INST_PROP(inst, disk_name),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, virtio_blk_dev_init, NULL, &virtio_blk_data_##inst,            \
			      &virtio_blk_drv_config_##inst, POST_KERNEL,                          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &virtio_blk_ops);

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_BLK_DEFINE)
