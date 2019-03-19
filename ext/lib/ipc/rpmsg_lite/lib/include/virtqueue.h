#ifndef VIRTQUEUE_H_
#define VIRTQUEUE_H_

/*-
 * Copyright (c) 2011, Bryan Venteicher <bryanv@FreeBSD.org>
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <stdint.h>
#include "rpmsg_default_config.h"
typedef uint8_t boolean;

#include "virtio_ring.h"
#include "llist.h"

/*Error Codes*/
#define VQ_ERROR_BASE (-3000)
#define ERROR_VRING_FULL (VQ_ERROR_BASE - 1)
#define ERROR_INVLD_DESC_IDX (VQ_ERROR_BASE - 2)
#define ERROR_EMPTY_RING (VQ_ERROR_BASE - 3)
#define ERROR_NO_MEM (VQ_ERROR_BASE - 4)
#define ERROR_VRING_MAX_DESC (VQ_ERROR_BASE - 5)
#define ERROR_VRING_ALIGN (VQ_ERROR_BASE - 6)
#define ERROR_VRING_NO_BUFF (VQ_ERROR_BASE - 7)
#define ERROR_VQUEUE_INVLD_PARAM (VQ_ERROR_BASE - 8)

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#define VQUEUE_SUCCESS (0)
#define VQUEUE_DEBUG (false)

/* This is temporary macro to replace C NULL support.
 * At the moment all the RTL specific functions are present in env.
 * */
#define VQ_NULL ((void *)0)

/* The maximum virtqueue size is 2^15. Use that value as the end of
 * descriptor chain terminator since it will never be a valid index
 * in the descriptor table. This is used to verify we are correctly
 * handling vq_free_cnt.
 */
#define VQ_RING_DESC_CHAIN_END (32768)
#define VIRTQUEUE_FLAG_INDIRECT (0x0001U)
#define VIRTQUEUE_FLAG_EVENT_IDX (0x0002U)
#define VIRTQUEUE_MAX_NAME_SZ (32) /* mind the alignment */

/* Support for indirect buffer descriptors. */
#define VIRTIO_RING_F_INDIRECT_DESC (1 << 28)

/* Support to suppress interrupt until specific index is reached. */
#define VIRTIO_RING_F_EVENT_IDX (1 << 29)

/*
 * Hint on how long the next interrupt should be postponed. This is
 * only used when the EVENT_IDX feature is negotiated.
 */
typedef enum
{
    VQ_POSTPONE_SHORT,
    VQ_POSTPONE_LONG,
    VQ_POSTPONE_EMPTIED /* Until all available desc are used. */
} vq_postpone_t;

/* local virtqueue representation, not in shared memory */
struct virtqueue
{
    /* 32bit aligned { */
    char vq_name[VIRTQUEUE_MAX_NAME_SZ];
    uint32_t vq_flags;
    int vq_alignment;
    int vq_ring_size;
    void *vq_ring_mem;
    void (*callback)(struct virtqueue *vq);
    void (*notify)(struct virtqueue *vq);
    int vq_max_indirect_size;
    int vq_indirect_mem_size;
    struct vring vq_ring;
    /* } 32bit aligned */

    /* 16bit aligned { */
    uint16_t vq_queue_index;
    uint16_t vq_nentries;
    uint16_t vq_free_cnt;
    uint16_t vq_queued_cnt;

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
    /* } 16bit aligned */

    boolean avail_read;  /* 8bit wide */
    boolean avail_write; /* 8bit wide */
    boolean used_read;   /* 8bit wide */
    boolean used_write;  /* 8bit wide */

    uint16_t padd; /* aligned to 32bits after this: */

    void *priv; /* private pointer, upper layer instance pointer */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
    void *env; /* private pointer to environment layer internal context */
#endif
};

/* struct to hold vring specific information */
struct vring_alloc_info
{
    void *phy_addr;
    uint32_t align;
    uint16_t num_descs;
    uint16_t pad;
};

struct vq_static_context
{
    struct virtqueue vq;
};

typedef void vq_callback(struct virtqueue *vq);
typedef void vq_notify(struct virtqueue *vq);

#if (VQUEUE_DEBUG == true)
#define VQASSERT_BOOL(_vq, _exp, _msg)                             \
    do                                                             \
    {                                                              \
        if (!(_exp))                                               \
        {                                                          \
            env_print("%s: %s - " _msg, __func__, (_vq)->vq_name); \
            while (1)                                              \
                ;                                                  \
        }                                                          \
    } while (0)
#define VQASSERT(_vq, _exp, _msg) VQASSERT_BOOL(_vq, (_exp) != 0, _msg)

#define VQ_RING_ASSERT_VALID_IDX(_vq, _idx) VQASSERT((_vq), (_idx) < (_vq)->vq_nentries, "invalid ring index")

#define VQ_PARAM_CHK(condition, status_var, status_err) \
    if ((status_var == 0) && (condition))               \
    {                                                   \
        status_var = status_err;                        \
    }

#define VQUEUE_BUSY(vq, dir) \
    if ((vq)->dir == false)  \
        (vq)->dir = true;    \
    else                     \
    VQASSERT(vq, (vq)->dir == false, "VirtQueue already in use")

#define VQUEUE_IDLE(vq, dir) ((vq)->dir = false)

#else

#define KASSERT(cond, str)
#define VQASSERT(_vq, _exp, _msg)
#define VQ_RING_ASSERT_VALID_IDX(_vq, _idx)
#define VQ_PARAM_CHK(condition, status_var, status_err)
#define VQUEUE_BUSY(vq, dir)
#define VQUEUE_IDLE(vq, dir)

#endif

int virtqueue_create(unsigned short id,
                     char *name,
                     struct vring_alloc_info *ring,
                     void (*callback)(struct virtqueue *vq),
                     void (*notify)(struct virtqueue *vq),
                     struct virtqueue **v_queue);

int virtqueue_create_static(unsigned short id,
                            char *name,
                            struct vring_alloc_info *ring,
                            void (*callback)(struct virtqueue *vq),
                            void (*notify)(struct virtqueue *vq),
                            struct virtqueue **v_queue,
                            struct vq_static_context *vq_ctxt);

int virtqueue_add_buffer(struct virtqueue *vq, uint16_t head_idx);

int virtqueue_fill_used_buffers(struct virtqueue *vq, void *buffer, uint32_t len);

int virtqueue_fill_avail_buffers(struct virtqueue *vq, void *buffer, uint32_t len);

void *virtqueue_get_buffer(struct virtqueue *vq, uint32_t *len, uint16_t *idx);

void *virtqueue_get_available_buffer(struct virtqueue *vq, uint16_t *avail_idx, uint32_t *len);

int virtqueue_add_consumed_buffer(struct virtqueue *vq, uint16_t head_idx, uint32_t len);

void virtqueue_disable_cb(struct virtqueue *vq);

int virtqueue_enable_cb(struct virtqueue *vq);

void virtqueue_kick(struct virtqueue *vq);

void virtqueue_free(struct virtqueue *vq);

void virtqueue_free_static(struct virtqueue *vq);

void virtqueue_dump(struct virtqueue *vq);

void virtqueue_notification(struct virtqueue *vq);

uint32_t virtqueue_get_desc_size(struct virtqueue *vq);

uint32_t virtqueue_get_buffer_length(struct virtqueue *vq, uint16_t idx);

void vq_ring_init(struct virtqueue *vq);

#endif /* VIRTQUEUE_H_ */
