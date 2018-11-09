/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 * Copyright (c) 2018 Linaro, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/alloc.h>
#include <metal/cache.h>
#include <metal/sleep.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include <openamp/virtqueue.h>

#include "rpmsg_internal.h"

#define RPMSG_NUM_VRINGS (2)

/* Total tick count for 15secs - 1msec tick. */
#define RPMSG_TICK_COUNT                        15000

/* Time to wait - In multiple of 10 msecs. */
#define RPMSG_TICKS_PER_INTERVAL                10

#define WORD_SIZE	sizeof(unsigned long)
#define WORD_ALIGN(a)	((((a) & (WORD_SIZE - 1)) != 0) ? \
			(((a) & (~(WORD_SIZE - 1))) + WORD_SIZE) : (a))

#ifndef VIRTIO_SLAVE_ONLY
metal_weak void *
rpmsg_virtio_shm_pool_get_buffer(struct rpmsg_virtio_shm_pool *shpool,
				 size_t size)
{
	void *buffer;

	if (shpool->avail < size)
		return NULL;
	buffer =  (void *)((char *)shpool->base + shpool->size - shpool->avail);
	shpool->avail -= size;

	return buffer;
}
#endif /*!VIRTIO_SLAVE_ONLY*/

void rpmsg_virtio_init_shm_pool(struct rpmsg_virtio_shm_pool *shpool,
				void *shb, size_t size)
{
	if (!shpool)
		return;
	shpool->base = shb;
	shpool->size = WORD_ALIGN(size);
	shpool->avail = WORD_ALIGN(size);
}

/**
 * rpmsg_virtio_return_buffer
 *
 * Places the used buffer back on the virtqueue.
 *
 * @param rvdev  - pointer to remote core
 * @param buffer - buffer pointer
 * @param len    - buffer length
 * @param idx    - buffer index
 *
 */
static void rpmsg_virtio_return_buffer(struct rpmsg_virtio_device *rvdev,
				       void *buffer, unsigned long len,
				       unsigned short idx)
{
	unsigned int role = rpmsg_virtio_get_role(rvdev);
#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER) {
		struct virtqueue_buf vqbuf;

		(void)idx;
		/* Initialize buffer node */
		vqbuf.buf = buffer;
		vqbuf.len = len;
		virtqueue_add_buffer(rvdev->rvq, &vqbuf, 0, 1, buffer);
	}
#endif /*VIRTIO_SLAVE_ONLY*/

#ifndef VIRTIO_MASTER_ONLY
	if (role == RPMSG_REMOTE) {
		(void)buffer;
		virtqueue_add_consumed_buffer(rvdev->rvq, idx, len);
	}
#endif /*VIRTIO_MASTER_ONLY*/
}

/**
 * rpmsg_virtio_enqueue_buffer
 *
 * Places buffer on the virtqueue for consumption by the other side.
 *
 * @param rvdev  - pointer to rpmsg virtio
 * @param buffer - buffer pointer
 * @param len    - buffer length
 * @param idx    - buffer index
 *
 * @return - status of function execution
 */
static int rpmsg_virtio_enqueue_buffer(struct rpmsg_virtio_device *rvdev,
				       void *buffer, unsigned long len,
				       unsigned short idx)
{
	unsigned int role = rpmsg_virtio_get_role(rvdev);
#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER) {
		struct virtqueue_buf vqbuf;
		(void)idx;

		/* Initialize buffer node */
		vqbuf.buf = buffer;
		vqbuf.len = len;
		return virtqueue_add_buffer(rvdev->svq, &vqbuf, 0, 1, buffer);
	}
#endif /*!VIRTIO_SLAVE_ONLY*/

#ifndef VIRTIO_MASTER_ONLY
	if (role == RPMSG_REMOTE) {
		(void)buffer;
		return virtqueue_add_consumed_buffer(rvdev->svq, idx, len);
	}
#endif /*!VIRTIO_MASTER_ONLY*/
	return 0;
}

/**
 * rpmsg_virtio_get_tx_buffer
 *
 * Provides buffer to transmit messages.
 *
 * @param rvdev - pointer to rpmsg device
 * @param len  - length of returned buffer
 * @param idx  - buffer index
 *
 * return - pointer to buffer.
 */
static void *rpmsg_virtio_get_tx_buffer(struct rpmsg_virtio_device *rvdev,
					unsigned long *len,
					unsigned short *idx)
{
	unsigned int role = rpmsg_virtio_get_role(rvdev);
	void *data = NULL;

#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER) {
		data = virtqueue_get_buffer(rvdev->svq, (uint32_t *)len, idx);
		if (data == NULL) {
			data = rpmsg_virtio_shm_pool_get_buffer(rvdev->shpool,
							RPMSG_BUFFER_SIZE);
			*len = RPMSG_BUFFER_SIZE;
		}
	}
#endif /*!VIRTIO_SLAVE_ONLY*/

#ifndef VIRTIO_MASTER_ONLY
	if (role == RPMSG_REMOTE) {
		data = virtqueue_get_available_buffer(rvdev->svq, idx,
						      (uint32_t *)len);
	}
#endif /*!VIRTIO_MASTER_ONLY*/

	return data;
}

/**
 * rpmsg_virtio_get_rx_buffer
 *
 * Retrieves the received buffer from the virtqueue.
 *
 * @param rvdev - pointer to rpmsg device
 * @param len  - size of received buffer
 * @param idx  - index of buffer
 *
 * @return - pointer to received buffer
 *
 */
static void *rpmsg_virtio_get_rx_buffer(struct rpmsg_virtio_device *rvdev,
					unsigned long *len,
					unsigned short *idx)
{
	unsigned int role = rpmsg_virtio_get_role(rvdev);
	void *data = NULL;

#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER) {
		data = virtqueue_get_buffer(rvdev->rvq, (uint32_t *)len, idx);
	}
#endif /*!VIRTIO_SLAVE_ONLY*/

#ifndef VIRTIO_MASTER_ONLY
	if (role == RPMSG_REMOTE) {
		data =
		    virtqueue_get_available_buffer(rvdev->rvq, idx,
						   (uint32_t *)len);
	}
#endif /*!VIRTIO_MASTER_ONLY*/

	if (data) {
		/* FIX ME: library should not worry about if it needs
		 * to flush/invalidate cache, it is shared memory.
		 * The shared memory should be mapped properly before
		 * using it.
		 */
		metal_cache_invalidate(data, (unsigned int)(*len));
	}

	return data;
}

#ifndef VIRTIO_MASTER_ONLY
/**
 * check if the remote is ready to start RPMsg communication
 */
static int rpmsg_virtio_wait_remote_ready(struct rpmsg_virtio_device *rvdev)
{
	uint8_t status;

	while (1) {
		status = rpmsg_virtio_get_status(rvdev);
		/* Busy wait until the remote is ready */
		if (status & VIRTIO_CONFIG_STATUS_NEEDS_RESET) {
			rpmsg_virtio_set_status(rvdev, 0);
			/* TODO notify remote processor */
		} else if (status & VIRTIO_CONFIG_STATUS_DRIVER_OK) {
			return true;
		}
		/* TODO: clarify metal_cpu_yield usage*/
		metal_cpu_yield();
	}

	return false;
}
#endif /*!VIRTIO_MASTER_ONLY*/

/**
 * _rpmsg_virtio_get_buffer_size
 *
 * Returns buffer size available for sending messages.
 *
 * @param channel - pointer to rpmsg channel
 *
 * @return - buffer size
 *
 */
static int _rpmsg_virtio_get_buffer_size(struct rpmsg_virtio_device *rvdev)
{
	unsigned int role = rpmsg_virtio_get_role(rvdev);
	int length = 0;

#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER) {
		/*
		 * If device role is Remote then buffers are provided by us
		 * (RPMSG Master), so just provide the macro.
		 */
		length = RPMSG_BUFFER_SIZE - sizeof(struct rpmsg_hdr);
	}
#endif /*!VIRTIO_SLAVE_ONLY*/

#ifndef VIRTIO_MASTER_ONLY
	if (role == RPMSG_REMOTE) {
		/*
		 * If other core is Master then buffers are provided by it,
		 * so get the buffer size from the virtqueue.
		 */
		length =
		    (int)virtqueue_get_desc_size(rvdev->svq) -
		    sizeof(struct rpmsg_hdr);
	}
#endif /*!VIRTIO_MASTER_ONLY*/

	return length;
}

/**
 * This function sends rpmsg "message" to remote device.
 *
 * @param rdev    - pointer to rpmsg device
 * @param src     - source address of channel
 * @param dst     - destination address of channel
 * @param data    - data to transmit
 * @param size    - size of data
 * @param wait    - boolean, wait or not for buffer to become
 *                  available
 *
 * @return - size of data sent or negative value for failure.
 *
 */
static int rpmsg_virtio_send_offchannel_raw(struct rpmsg_device *rdev,
					    uint32_t src, uint32_t dst,
					    const void *data,
					    int size, int wait)
{
	struct rpmsg_virtio_device *rvdev;
	struct rpmsg_hdr rp_hdr;
	void *buffer = NULL;
	unsigned short idx;
	int tick_count = 0;
	unsigned long buff_len;
	int status;
	struct metal_io_region *io;

	/* Get the associated remote device for channel. */
	rvdev = metal_container_of(rdev, struct rpmsg_virtio_device, rdev);

	status = rpmsg_virtio_get_status(rvdev);
	/* Validate device state */
	if (!(status & VIRTIO_CONFIG_STATUS_DRIVER_OK)) {
		return RPMSG_ERR_DEV_STATE;
	}

	if (wait)
		tick_count = RPMSG_TICK_COUNT / RPMSG_TICKS_PER_INTERVAL;
	else
		tick_count = 0;

	while (1) {
		int avail_size;

		/* Lock the device to enable exclusive access to virtqueues */
		metal_mutex_acquire(&rdev->lock);
		avail_size = _rpmsg_virtio_get_buffer_size(rvdev);
		if (size <= avail_size)
			buffer = rpmsg_virtio_get_tx_buffer(rvdev, &buff_len,
							    &idx);
		metal_mutex_release(&rdev->lock);
		if (buffer || !tick_count)
			break;
		if (avail_size != 0)
			return RPMSG_ERR_BUFF_SIZE;
		metal_sleep_usec(RPMSG_TICKS_PER_INTERVAL);
		tick_count--;
	}
	if (!buffer)
		return RPMSG_ERR_NO_BUFF;

	/* Initialize RPMSG header. */
	rp_hdr.dst = dst;
	rp_hdr.src = src;
	rp_hdr.len = size;
	rp_hdr.reserved = 0;

	/* Copy data to rpmsg buffer. */
	io = rvdev->shbuf_io;
	status = metal_io_block_write(io, metal_io_virt_to_offset(io, buffer),
				      &rp_hdr, sizeof(rp_hdr));
	RPMSG_ASSERT(status == sizeof(rp_hdr), "failed to write header\n");

	status = metal_io_block_write(io,
				      metal_io_virt_to_offset(io,
				      RPMSG_LOCATE_DATA(buffer)),
				      data, size);
	RPMSG_ASSERT(status == size, "failed to write buffer\n");
	metal_mutex_acquire(&rdev->lock);

	/* Enqueue buffer on virtqueue. */
	status = rpmsg_virtio_enqueue_buffer(rvdev, buffer, buff_len, idx);
	RPMSG_ASSERT(status == VQUEUE_SUCCESS, "failed to enqueue buffer\n");
	/* Let the other side know that there is a job to process. */
	virtqueue_kick(rvdev->svq);

	metal_mutex_release(&rdev->lock);

	return size;
}

/**
 * rpmsg_virtio_tx_callback
 *
 * Tx callback function.
 *
 * @param vq - pointer to virtqueue on which Tx is has been
 *             completed.
 *
 */
static void rpmsg_virtio_tx_callback(struct virtqueue *vq)
{
	(void)vq;
}

/**
 * rpmsg_virtio_rx_callback
 *
 * Rx callback function.
 *
 * @param vq - pointer to virtqueue on which messages is received
 *
 */
static void rpmsg_virtio_rx_callback(struct virtqueue *vq)
{
	struct virtio_device *vdev = vq->vq_dev;
	struct rpmsg_virtio_device *rvdev = vdev->priv;
	struct rpmsg_device *rdev = &rvdev->rdev;
	struct rpmsg_endpoint *ept;
	struct rpmsg_hdr *rp_hdr;
	unsigned long len;
	unsigned short idx;
	int status;

	metal_mutex_acquire(&rdev->lock);

	/* Process the received data from remote node */
	rp_hdr = (struct rpmsg_hdr *)rpmsg_virtio_get_rx_buffer(rvdev,
								&len, &idx);

	metal_mutex_release(&rdev->lock);

	while (rp_hdr) {
		/* Get the channel node from the remote device channels list. */
		metal_mutex_acquire(&rdev->lock);
		ept = rpmsg_get_ept_from_addr(rdev, rp_hdr->dst);
		metal_mutex_release(&rdev->lock);

		if (!ept)
			/* Fatal error no endpoint for the given dst addr. */
			return;

		if (ept->dest_addr == RPMSG_ADDR_ANY) {
			/*
			 * First message received from the remote side,
			 * update channel destination address
			 */
			ept->dest_addr = rp_hdr->src;
		}
		status = ept->cb(ept, (void *)RPMSG_LOCATE_DATA(rp_hdr),
				   rp_hdr->len, ept->addr, ept->priv);

		RPMSG_ASSERT(status == RPMSG_SUCCESS,
			     "unexpected callback status\n");
		metal_mutex_acquire(&rdev->lock);

		/* Return used buffers. */
		rpmsg_virtio_return_buffer(rvdev, rp_hdr, len, idx);

		rp_hdr = (struct rpmsg_hdr *)
			 rpmsg_virtio_get_rx_buffer(rvdev, &len, &idx);
		metal_mutex_release(&rdev->lock);
	}
}

/**
 * rpmsg_virtio_ns_callback
 *
 * This callback handles name service announcement from the remote device
 * and creates/deletes rpmsg channels.
 *
 * @param server_chnl - pointer to server channel control block.
 * @param data        - pointer to received messages
 * @param len         - length of received data
 * @param priv        - any private data
 * @param src         - source address
 *
 * @return - rpmag endpoint callback handled
 */
static int rpmsg_virtio_ns_callback(struct rpmsg_endpoint *ept, void *data,
				    size_t len, uint32_t src, void *priv)
{
	struct rpmsg_device *rdev = ept->rdev;
	struct rpmsg_virtio_device *rvdev = (struct rpmsg_virtio_device *)rdev;
	struct metal_io_region *io = rvdev->shbuf_io;
	struct rpmsg_endpoint *_ept;
	struct rpmsg_ns_msg *ns_msg;
	uint32_t dest;
	char name[RPMSG_NAME_SIZE];

	(void)priv;
	(void)src;

	ns_msg = (struct rpmsg_ns_msg *)data;
	if (len != sizeof(*ns_msg))
		/* Returns as the message is corrupted */
		return RPMSG_SUCCESS;
	metal_io_block_read(io,
			    metal_io_virt_to_offset(io, ns_msg->name),
			    &name, sizeof(name));
	dest = ns_msg->addr;

	/* check if a Ept has been locally registered */
	metal_mutex_acquire(&rdev->lock);
	_ept = rpmsg_get_endpoint(rdev, name, RPMSG_ADDR_ANY, dest);

	if (ns_msg->flags & RPMSG_NS_DESTROY) {
		if (_ept)
			_ept->dest_addr = RPMSG_ADDR_ANY;
		metal_mutex_release(&rdev->lock);
		if (_ept && _ept->ns_unbind_cb)
			_ept->ns_unbind_cb(ept);
	} else {
		if (!_ept) {
			/*
			 * send callback to application, that can
			 * - create the associated endpoints.
			 * - store information for future use.
			 * - just ignore the request as service not supported.
			 */
			metal_mutex_release(&rdev->lock);
			if (rdev->ns_bind_cb)
				rdev->ns_bind_cb(rdev, name, dest);
		} else {
			_ept->dest_addr = dest;
			metal_mutex_release(&rdev->lock);
		}
	}

	return RPMSG_SUCCESS;
}

int rpmsg_virtio_get_buffer_size(struct rpmsg_device *rdev)
{
	int size;
	struct rpmsg_virtio_device *rvdev;

	if (!rdev)
		return RPMSG_ERR_PARAM;
	metal_mutex_acquire(&rdev->lock);
	rvdev = (struct rpmsg_virtio_device *)rdev;
	size = _rpmsg_virtio_get_buffer_size(rvdev);
	metal_mutex_release(&rdev->lock);
	return size;
}

int rpmsg_init_vdev(struct rpmsg_virtio_device *rvdev,
		    struct virtio_device *vdev,
		    rpmsg_ns_bind_cb ns_bind_cb,
		    struct metal_io_region *shm_io,
		    struct rpmsg_virtio_shm_pool *shpool)
{
	struct rpmsg_device *rdev;
	const char *vq_names[RPMSG_NUM_VRINGS];
	typedef void (*vqcallback)(struct virtqueue *vq);
	vqcallback callback[RPMSG_NUM_VRINGS];
	unsigned long dev_features;
	int status;
	unsigned int i, role;

	rdev = &rvdev->rdev;
	memset(rdev, 0, sizeof(*rdev));
	metal_mutex_init(&rdev->lock);
	rvdev->vdev = vdev;
	rdev->ns_bind_cb = ns_bind_cb;
	vdev->priv = rvdev;
	rdev->ops.send_offchannel_raw = rpmsg_virtio_send_offchannel_raw;
	role = rpmsg_virtio_get_role(rvdev);

#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER) {
		/*
		 * Since device is RPMSG Remote so we need to manage the
		 * shared buffers. Create shared memory pool to handle buffers.
		 */
		if (!shpool)
			return RPMSG_ERR_PARAM;
		if (!shpool->size)
			return RPMSG_ERR_NO_BUFF;
		rvdev->shpool = shpool;

		vq_names[0] = "rx_vq";
		vq_names[1] = "tx_vq";
		callback[0] = rpmsg_virtio_rx_callback;
		callback[1] = rpmsg_virtio_tx_callback;
		rvdev->rvq  = vdev->vrings_info[0].vq;
		rvdev->svq  = vdev->vrings_info[1].vq;
	}
#endif /*!VIRTIO_SLAVE_ONLY*/

#ifndef VIRTIO_MASTER_ONLY
	(void)shpool;
	if (role == RPMSG_REMOTE) {
		vq_names[0] = "tx_vq";
		vq_names[1] = "rx_vq";
		callback[0] = rpmsg_virtio_tx_callback;
		callback[1] = rpmsg_virtio_rx_callback;
		rvdev->rvq  = vdev->vrings_info[1].vq;
		rvdev->svq  = vdev->vrings_info[0].vq;
	}
#endif /*!VIRTIO_MASTER_ONLY*/
	rvdev->shbuf_io = shm_io;

#ifndef VIRTIO_MASTER_ONLY
	if (role == RPMSG_REMOTE) {
		/* wait synchro with the master */
		rpmsg_virtio_wait_remote_ready(rvdev);
	}
#endif /*!VIRTIO_MASTER_ONLY*/

	/* Create virtqueues for remote device */
	status = rpmsg_virtio_create_virtqueues(rvdev, 0, RPMSG_NUM_VRINGS,
						vq_names, callback);
	if (status != RPMSG_SUCCESS)
		return status;

	/* TODO: can have a virtio function to set the shared memory I/O */
	for (i = 0; i < RPMSG_NUM_VRINGS; i++) {
		struct virtqueue *vq;

		vq = vdev->vrings_info[i].vq;
		vq->shm_io = shm_io;
	}

#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER) {
		struct virtqueue_buf vqbuf;
		unsigned int idx;
		void *buffer;

		vqbuf.len = RPMSG_BUFFER_SIZE;
		for (idx = 0; idx < rvdev->rvq->vq_nentries; idx++) {
			/* Initialize TX virtqueue buffers for remote device */
			buffer = rpmsg_virtio_shm_pool_get_buffer(shpool,
							RPMSG_BUFFER_SIZE);

			if (!buffer) {
				return RPMSG_ERR_NO_BUFF;
			}

			vqbuf.buf = buffer;

			metal_io_block_set(shm_io,
					   metal_io_virt_to_offset(shm_io,
								   buffer),
					   0x00, RPMSG_BUFFER_SIZE);
			status =
				virtqueue_add_buffer(rvdev->rvq, &vqbuf, 0, 1,
						     buffer);

			if (status != RPMSG_SUCCESS) {
				return status;
			}
		}
	}
#endif /*!VIRTIO_SLAVE_ONLY*/

	/* Initialize channels and endpoints list */
	metal_list_init(&rdev->endpoints);

	dev_features = rpmsg_virtio_get_features(rvdev);

	/*
	 * Create name service announcement endpoint if device supports name
	 * service announcement feature.
	 */
	if ((dev_features & (1 << VIRTIO_RPMSG_F_NS))) {
		rpmsg_init_ept(&rdev->ns_ept, "NS",
			       RPMSG_NS_EPT_ADDR, RPMSG_NS_EPT_ADDR,
			       rpmsg_virtio_ns_callback, NULL);
		(void)rpmsg_register_endpoint(rdev, &rdev->ns_ept);
	}

#ifndef VIRTIO_SLAVE_ONLY
	if (role == RPMSG_MASTER)
		rpmsg_virtio_set_status(rvdev, VIRTIO_CONFIG_STATUS_DRIVER_OK);
#endif /*!VIRTIO_SLAVE_ONLY*/

	return status;
}

void rpmsg_deinit_vdev(struct rpmsg_virtio_device *rvdev)
{
	struct metal_list *node;
	struct rpmsg_device *rdev;
	struct rpmsg_endpoint *ept;

	rdev = &rvdev->rdev;
	while (!metal_list_is_empty(&rdev->endpoints)) {
		node = rdev->endpoints.next;
		ept = metal_container_of(node, struct rpmsg_endpoint, node);
		rpmsg_destroy_ept(ept);
	}

	rvdev->rvq = 0;
	rvdev->svq = 0;

	metal_mutex_deinit(&rdev->lock);
}
