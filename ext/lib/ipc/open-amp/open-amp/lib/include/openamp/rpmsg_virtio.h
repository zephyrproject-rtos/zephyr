/*
 * rpmsg based on virtio
 *
 * Copyright (C) 2018 Linaro, Inc.
 *
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RPMSG_VIRTIO_H_
#define _RPMSG_VIRTIO_H_

#include <metal/io.h>
#include <metal/mutex.h>
#include <openamp/rpmsg.h>
#include <openamp/virtio.h>

#if defined __cplusplus
extern "C" {
#endif

/* Configurable parameters */
#ifndef RPMSG_BUFFER_SIZE
#define RPMSG_BUFFER_SIZE	(512)
#endif

/* The feature bitmap for virtio rpmsg */
#define VIRTIO_RPMSG_F_NS 0 /* RP supports name service notifications */

struct rpmsg_virtio_shm_pool;
/**
 * struct rpmsg_virtio_shm_pool - shared memory pool used for rpmsg buffers
 * @get_buffer: function to get buffer from the pool
 * @base: base address of the memory pool
 * @avail: available memory size
 * @size: total pool size
 */
struct rpmsg_virtio_shm_pool {
	void *base;
	size_t avail;
	size_t size;
};

/**
 * struct rpmsg_virtio_device - representation of a rpmsg device based on virtio
 * @rdev: rpmsg device, first property in the struct
 * @vdev: pointer to the virtio device
 * @rvq: pointer to receive virtqueue
 * @svq: pointer to send virtqueue
 * @shbuf_io: pointer to the shared buffer I/O region
 * @shpool: pointer to the shared buffers pool
 * @endpoints: list of endpoints.
 */
struct rpmsg_virtio_device {
	struct rpmsg_device rdev;
	struct virtio_device *vdev;
	struct virtqueue *rvq;
	struct virtqueue *svq;
	struct metal_io_region *shbuf_io;
	struct rpmsg_virtio_shm_pool *shpool;
};

#define RPMSG_REMOTE	VIRTIO_DEV_SLAVE
#define RPMSG_MASTER	VIRTIO_DEV_MASTER
static inline unsigned int
	rpmsg_virtio_get_role(struct rpmsg_virtio_device *rvdev)
{
	return rvdev->vdev->role;
}

static inline void rpmsg_virtio_set_status(struct rpmsg_virtio_device *rvdev,
					   uint8_t status)
{
	rvdev->vdev->func->set_status(rvdev->vdev, status);
}

static inline uint8_t rpmsg_virtio_get_status(struct rpmsg_virtio_device *rvdev)
{
	return rvdev->vdev->func->get_status(rvdev->vdev);
}

static inline uint32_t
	rpmsg_virtio_get_features(struct rpmsg_virtio_device *rvdev)
{
	return rvdev->vdev->func->get_features(rvdev->vdev);
}

static inline int
	rpmsg_virtio_create_virtqueues(struct rpmsg_virtio_device *rvdev,
				       int flags, unsigned int nvqs,
				       const char *names[],
				       vq_callback * callbacks[])
{
	return virtio_create_virtqueues(rvdev->vdev, flags, nvqs, names,
					callbacks);
}

/**
 * rpmsg_virtio_get_buffer_size - get rpmsg virtio buffer size
 *
 * @rdev - pointer to the rpmsg device
 *
 * @return - next available buffer size for text, negative value for failure
 */
int rpmsg_virtio_get_buffer_size(struct rpmsg_device *rdev);

/**
 * rpmsg_init_vdev - initialize rpmsg virtio device
 * Master side:
 * Initialize RPMsg virtio queues and shared buffers, the address of shm can be
 * ANY. In this case, function will get shared memory from system shared memory
 * pools. If the vdev has RPMsg name service feature, this API will create an
 * name service endpoint.
 *
 * Slave side:
 * This API will not return until the driver ready is set by the master side.
 *
 * @param rvdev  - pointer to the rpmsg virtio device
 * @param vdev   - pointer to the virtio device
 * @param ns_bind_cb  - callback handler for name service announcement without
 *                      local endpoints waiting to bind.
 * @param shm_io - pointer to the share memory I/O region.
 * @param shpool - pointer to shared memory pool. rpmsg_virtio_init_shm_pool has
 *                 to be called first to fill this structure.
 *
 * @return - status of function execution
 */
int rpmsg_init_vdev(struct rpmsg_virtio_device *rvdev,
		    struct virtio_device *vdev,
		    rpmsg_ns_bind_cb ns_bind_cb,
		    struct metal_io_region *shm_io,
		    struct rpmsg_virtio_shm_pool *shpool);

/**
 * rpmsg_deinit_vdev - deinitialize rpmsg virtio device
 *
 * @param rvdev - pointer to the rpmsg virtio device
 */
void rpmsg_deinit_vdev(struct rpmsg_virtio_device *rvdev);

/**
 * rpmsg_virtio_init_shm_pool - initialize default shared buffers pool
 *
 * RPMsg virtio has default shared buffers pool implementation.
 * The memory assigned to this pool will be dedicated to the RPMsg
 * virtio. This function has to be called before calling rpmsg_init_vdev,
 * to initialize the rpmsg_virtio_shm_pool structure.
 *
 * @param shpool - pointer to the shared buffers pool structure
 * @param shbuf - pointer to the beginning of shared buffers
 * @param size - shared buffers total size
 */
void rpmsg_virtio_init_shm_pool(struct rpmsg_virtio_shm_pool *shpool,
				void *shbuf, size_t size);

/**
 * rpmsg_virtio_get_rpmsg_device - get RPMsg device from RPMsg virtio device
 *
 * @param rvdev - pointer to RPMsg virtio device
 * @return - RPMsg device pointed by RPMsg virtio device
 */
static inline struct rpmsg_device *
rpmsg_virtio_get_rpmsg_device(struct rpmsg_virtio_device *rvdev)
{
	return &rvdev->rdev;
}

/**
 * rpmsg_virtio_shm_pool_get_buffer - get buffer in the shared memory pool
 *
 * RPMsg virtio has default shared buffers pool implementation.
 * The memory assigned to this pool will be dedicated to the RPMsg
 * virtio. If you prefer to have other shared buffers allocation,
 * you can implement your rpmsg_virtio_shm_pool_get_buffer function.
 *
 * @param shpool - pointer to the shared buffers pool
 * @param size - shared buffers total size
 * @return - buffer pointer if free buffer is available, NULL otherwise.
 */
metal_weak void *
rpmsg_virtio_shm_pool_get_buffer(struct rpmsg_virtio_shm_pool *shpool,
				 size_t size);

#if defined __cplusplus
}
#endif

#endif	/* _RPMSG_VIRTIO_H_ */
