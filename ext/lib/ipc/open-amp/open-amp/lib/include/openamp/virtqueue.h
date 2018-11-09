#ifndef VIRTQUEUE_H_
#define VIRTQUEUE_H_

/*-
 * Copyright (c) 2011, Bryan Venteicher <bryanv@FreeBSD.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * $FreeBSD$
 */

#include <stdbool.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

typedef uint8_t boolean;

#include <openamp/virtio_ring.h>
#include <metal/alloc.h>
#include <metal/io.h>

/*Error Codes*/
#define VQ_ERROR_BASE                                 -3000
#define ERROR_VRING_FULL                              (VQ_ERROR_BASE - 1)
#define ERROR_INVLD_DESC_IDX                          (VQ_ERROR_BASE - 2)
#define ERROR_EMPTY_RING                              (VQ_ERROR_BASE - 3)
#define ERROR_NO_MEM                                  (VQ_ERROR_BASE - 4)
#define ERROR_VRING_MAX_DESC                          (VQ_ERROR_BASE - 5)
#define ERROR_VRING_ALIGN                             (VQ_ERROR_BASE - 6)
#define ERROR_VRING_NO_BUFF                           (VQ_ERROR_BASE - 7)
#define ERROR_VQUEUE_INVLD_PARAM                      (VQ_ERROR_BASE - 8)

#define VQUEUE_SUCCESS                                0

/* The maximum virtqueue size is 2^15. Use that value as the end of
 * descriptor chain terminator since it will never be a valid index
 * in the descriptor table. This is used to verify we are correctly
 * handling vq_free_cnt.
 */
#define VQ_RING_DESC_CHAIN_END                         32768
#define VIRTQUEUE_FLAG_INDIRECT                        0x0001
#define VIRTQUEUE_FLAG_EVENT_IDX                       0x0002
#define VIRTQUEUE_MAX_NAME_SZ                          32

/* Support for indirect buffer descriptors. */
#define VIRTIO_RING_F_INDIRECT_DESC    (1 << 28)

/* Support to suppress interrupt until specific index is reached. */
#define VIRTIO_RING_F_EVENT_IDX        (1 << 29)

struct virtqueue_buf {
	void *buf;
	int len;
};

struct virtqueue {
	struct virtio_device *vq_dev;
	const char *vq_name;
	uint16_t vq_queue_index;
	uint16_t vq_nentries;
	uint32_t vq_flags;
	void (*callback)(struct virtqueue *vq);
	void (*notify)(struct virtqueue *vq);
	struct vring vq_ring;
	uint16_t vq_free_cnt;
	uint16_t vq_queued_cnt;
	void *shm_io; /* opaque pointer to data needed to allow v2p & p2v */

	/*
	 * Head of the free chain in the descriptor table. If
	 * there are no free descriptors, this will be set to
	 * VQ_RING_DESC_CHAIN_END.
	 */
	uint16_t vq_desc_head_idx;

	/*
	 * Last consumed descriptor in the used table,
	 * trails vq_ring.used->idx.
	 */
	uint16_t vq_used_cons_idx;

	/*
	 * Last consumed descriptor in the available table -
	 * used by the consumer side.
	 */
	uint16_t vq_available_idx;

#ifdef VQUEUE_DEBUG
	boolean vq_inuse;
#endif

	/*
	 * Used by the host side during callback. Cookie
	 * holds the address of buffer received from other side.
	 * Other fields in this structure are not used currently.
	 */

	struct vq_desc_extra {
		void *cookie;
		uint16_t ndescs;
	} vq_descx[0];
};

/* struct to hold vring specific information */
struct vring_alloc_info {
	void *vaddr;
	uint32_t align;
	uint16_t num_descs;
	uint16_t pad;
};

typedef void vq_callback(struct virtqueue *);
typedef void vq_notify(struct virtqueue *);

#ifdef VQUEUE_DEBUG
#include <metal/log.h>
#include <metal/assert.h>

#define VQASSERT(_vq, _exp, _msg) \
	do { \
		if (!(_exp)) { \
			metal_log(METAL_LOG_EMERGENCY, \
				  "%s: %s - _msg", __func__, (_vq)->vq_name); \
			metal_assert(_exp); \
		} \
	} while (0)

#define VQ_RING_ASSERT_VALID_IDX(_vq, _idx)            \
	VQASSERT((_vq), (_idx) < (_vq)->vq_nentries, "invalid ring index")

#define VQ_RING_ASSERT_CHAIN_TERM(_vq)                \
	VQASSERT((_vq), (_vq)->vq_desc_head_idx ==            \
	VQ_RING_DESC_CHAIN_END, \
	"full ring terminated incorrectly: invalid head")

#define VQ_PARAM_CHK(condition, status_var, status_err) \
	do {						\
		if (((status_var) == 0) && (condition)) { \
			status_var = status_err;        \
		}					\
	} while (0)

#define VQUEUE_BUSY(vq) \
	do {						     \
		if (!(vq)->vq_inuse)                 \
			(vq)->vq_inuse = true;               \
		else                                         \
			VQASSERT(vq, !(vq)->vq_inuse,\
				"VirtQueue already in use")  \
	} while (0)

#define VQUEUE_IDLE(vq)            ((vq)->vq_inuse = false)

#else

#define KASSERT(cond, str)
#define VQASSERT(_vq, _exp, _msg)
#define VQ_RING_ASSERT_VALID_IDX(_vq, _idx)
#define VQ_RING_ASSERT_CHAIN_TERM(_vq)
#define VQ_PARAM_CHK(condition, status_var, status_err)
#define VQUEUE_BUSY(vq)
#define VQUEUE_IDLE(vq)

#endif

int virtqueue_create(struct virtio_device *device, unsigned short id,
		     const char *name, struct vring_alloc_info *ring,
		     void (*callback)(struct virtqueue *vq),
		     void (*notify)(struct virtqueue *vq),
		     struct virtqueue *v_queue);

/*
 * virtqueue_set_shmem_io
 *
 * set virtqueue shared memory I/O region
 *
 * @vq - virt queue
 * @io - pointer to the shared memory I/O region
 */
static inline void virtqueue_set_shmem_io(struct virtqueue *vq,
					  struct metal_io_region *io)
{
	vq->shm_io = io;
}

int virtqueue_add_buffer(struct virtqueue *vq, struct virtqueue_buf *buf_list,
			 int readable, int writable, void *cookie);

void *virtqueue_get_buffer(struct virtqueue *vq, uint32_t *len, uint16_t *idx);

void *virtqueue_get_available_buffer(struct virtqueue *vq, uint16_t *avail_idx,
				     uint32_t *len);

int virtqueue_add_consumed_buffer(struct virtqueue *vq, uint16_t head_idx,
				  uint32_t len);

void virtqueue_disable_cb(struct virtqueue *vq);

int virtqueue_enable_cb(struct virtqueue *vq);

void virtqueue_kick(struct virtqueue *vq);

static inline struct virtqueue *virtqueue_allocate(unsigned int num_desc_extra)
{
	struct virtqueue *vqs;
	uint32_t vq_size = sizeof(struct virtqueue) +
		 num_desc_extra * sizeof(struct vq_desc_extra);

	vqs = (struct virtqueue *)metal_allocate_memory(vq_size);

	if (vqs) {
		memset(vqs, 0x00, vq_size);
	}

	return vqs;
}

void virtqueue_free(struct virtqueue *vq);

void virtqueue_dump(struct virtqueue *vq);

void virtqueue_notification(struct virtqueue *vq);

uint32_t virtqueue_get_desc_size(struct virtqueue *vq);

uint32_t virtqueue_get_buffer_length(struct virtqueue *vq, uint16_t idx);

#if defined __cplusplus
}
#endif

#endif				/* VIRTQUEUE_H_ */
