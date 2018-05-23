/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2015 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       rpmsg_core.c
 *
 * COMPONENT
 *
 *       OpenAMP
 *
 * DESCRIPTION
 *
 * This file provides the core functionality of RPMSG messaging part like
 * message parsing ,Rx/Tx callbacks handling , channel creation/deletion
 * and address management.
 *
 *
 **************************************************************************/
#include <string.h>
#include <openamp/rpmsg.h>
#include <metal/utilities.h>
#include <metal/io.h>
#include <metal/cache.h>
#include <metal/alloc.h>
#include <metal/cpu.h>

/* Internal functions */
static void rpmsg_rx_callback(struct virtqueue *vq);
static void rpmsg_tx_callback(struct virtqueue *vq);

/**
 * rpmsg_memb_cpy
 *
 * This function copies contents from one memory to the other byte by byte.
 *
 * RPMsg can be used across different memories.
 * memcpy/strncpy doesn't always work.
 *
 * @param src - pointer to source memory
 * @param dest - pointer to target memory
 * @param n - number of bytes to copy
 *
 * @return pointer to dest
 */
static void *rpmsg_memb_cpy(void *dest, const void *src, size_t n)
{
	size_t i;
	unsigned char *tmp_dest;
	const unsigned char *tmp_src;

	tmp_dest = dest;
	tmp_src = src;

	for (i = 0; i < n; i++, tmp_dest++, tmp_src++)
		*tmp_dest = *tmp_src;

	return dest;
}

/**
 * rpmsg_start_ipc
 *
 * This function creates communication links(virtqueues) for remote device
 * and notifies it to start IPC.
 *
 * @param rdev - remote device handle
 *
 * @return - status of function execution
 *
 */
int rpmsg_start_ipc(struct remote_device *rdev)
{
	struct virtio_device *virt_dev;
	struct rpmsg_endpoint *ns_ept;
	void (*callback[2]) (struct virtqueue * vq);
	const char *vq_names[2];
	unsigned long dev_features;
	int status;
	struct virtqueue *vqs[2];
	int i;

	virt_dev = &rdev->virt_dev;

	/* Initialize names and callbacks based on the device role */
	if (rdev->role == RPMSG_MASTER) {
		vq_names[0] = "tx_vq";
		vq_names[1] = "rx_vq";
		callback[0] = rpmsg_tx_callback;
		callback[1] = rpmsg_rx_callback;
	} else {
		vq_names[0] = "rx_vq";
		vq_names[1] = "tx_vq";
		callback[0] = rpmsg_rx_callback;
		callback[1] = rpmsg_tx_callback;
	}

	/* Create virtqueues for remote device */
	status = virt_dev->func->create_virtqueues(virt_dev, 0,
						   RPMSG_MAX_VQ_PER_RDEV,
						   vq_names, callback,
						   RPMSG_NULL);
	if (status != RPMSG_SUCCESS)
		return status;

	dev_features = virt_dev->func->get_features(virt_dev);

	/*
	 * Create name service announcement endpoint if device supports name
	 * service announcement feature.
	 */
	if ((dev_features & (1 << VIRTIO_RPMSG_F_NS))) {
		rdev->support_ns = RPMSG_TRUE;
		ns_ept = _create_endpoint(rdev, rpmsg_ns_callback, rdev,
					  RPMSG_NS_EPT_ADDR);
		if (!ns_ept) {
			return RPMSG_ERR_NO_MEM;
		}
	}

	/* Initialize notifications for vring. */
	if (rdev->role == RPMSG_MASTER) {
		vqs[0] = rdev->tvq;
		vqs[1] = rdev->rvq;
	} else {
		vqs[0] = rdev->rvq;
		vqs[1] = rdev->tvq;
	}
	for (i = 0; i <= 1; i++) {
		status = hil_enable_vring_notifications(i, vqs[i]);
		if (status != RPMSG_SUCCESS) {
			return status;
		}
	}

	if (rdev->role == RPMSG_REMOTE) {
		virt_dev->func->set_status(virt_dev,
			VIRTIO_CONFIG_STATUS_DRIVER_OK);
		status = rpmsg_rdev_notify(rdev);
	}
	if (status == RPMSG_SUCCESS)
		rdev->state = RPMSG_DEV_STATE_ACTIVE;

	return status;
}

/**
 * _rpmsg_create_channel
 *
 * Creates new rpmsg channel with the given parameters.
 *
 * @param rdev - pointer to remote device which contains the channel
 * @param name - name of the device
 * @param src  - source address for the rpmsg channel
 * @param dst  - destination address for the rpmsg channel
 *
 * @return - pointer to new rpmsg channel
 *
 */
struct rpmsg_channel *_rpmsg_create_channel(struct remote_device *rdev,
					    char *name, unsigned long src,
					    unsigned long dst)
{
	struct rpmsg_channel *rp_chnl;

	rp_chnl = metal_allocate_memory(sizeof(struct rpmsg_channel));
	if (rp_chnl) {
		memset(rp_chnl, 0x00, sizeof(struct rpmsg_channel));
		rpmsg_memb_cpy(rp_chnl->name, name, sizeof(rp_chnl->name)-1);
		rp_chnl->src = src;
		rp_chnl->dst = dst;
		rp_chnl->rdev = rdev;
		/* Place channel on channels list */
		metal_mutex_acquire(&rdev->lock);
		metal_list_add_tail(&rdev->rp_channels, &rp_chnl->node);
		metal_mutex_release(&rdev->lock);
	}

	return rp_chnl;
}

/**
 * _rpmsg_delete_channel
 *
 * Deletes given rpmsg channel.
 *
 * @param rp_chnl -  pointer to rpmsg channel to delete
 *
 * return - none
 */
void _rpmsg_delete_channel(struct rpmsg_channel *rp_chnl)
{
	if (rp_chnl) {
		metal_mutex_acquire(&rp_chnl->rdev->lock);
		metal_list_del(&rp_chnl->node);
		metal_mutex_release(&rp_chnl->rdev->lock);
		metal_free_memory(rp_chnl);
	}
}

/**
 * _create_endpoint
 *
 * This function creates rpmsg endpoint.
 *
 * @param rdev    - pointer to remote device
 * @param cb      - Rx completion call back
 * @param priv    - private data
 * @param addr    - endpoint src address
 *
 * @return - pointer to endpoint control block
 *
 */
struct rpmsg_endpoint *_create_endpoint(struct remote_device *rdev,
					rpmsg_rx_cb_t cb, void *priv,
					unsigned long addr)
{

	struct rpmsg_endpoint *rp_ept;
	int status = RPMSG_SUCCESS;

	rp_ept = metal_allocate_memory(sizeof(struct rpmsg_endpoint));
	if (!rp_ept) {
		return RPMSG_NULL;
	}
	memset(rp_ept, 0x00, sizeof(struct rpmsg_endpoint));

	metal_mutex_acquire(&rdev->lock);

	if (addr != RPMSG_ADDR_ANY) {
		/*
		 * Application has requested a particular src address for endpoint,
		 * first check if address is available.
		 */
		if (!rpmsg_is_address_set
		    (rdev->bitmap, RPMSG_ADDR_BMP_SIZE, addr)) {
			/* Mark the address as used in the address bitmap. */
			rpmsg_set_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE,
					  addr);

		} else {
			status = RPMSG_ERR_DEV_ADDR;
		}
	} else {
		addr = rpmsg_get_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE);
		if ((int)addr < 0) {
			status = RPMSG_ERR_DEV_ADDR;
		}
	}

	/* Do cleanup in case of error and return */
	if (RPMSG_SUCCESS != status) {
		metal_free_memory(rp_ept);
		metal_mutex_release(&rdev->lock);
		return RPMSG_NULL;
	}

	rp_ept->addr = addr;
	rp_ept->cb = cb;
	rp_ept->priv = priv;

	metal_list_add_tail(&rdev->rp_endpoints, &rp_ept->node);

	metal_mutex_release(&rdev->lock);

	return rp_ept;
}

/**
 * rpmsg_destroy_ept
 *
 * This function deletes rpmsg endpoint and performs cleanup.
 *
 * @param rdev   - pointer to remote device
 * @param rp_ept - pointer to endpoint to destroy
 *
 */
void _destroy_endpoint(struct remote_device *rdev,
		       struct rpmsg_endpoint *rp_ept)
{
	metal_mutex_acquire(&rdev->lock);
	rpmsg_release_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE,
			      rp_ept->addr);
	metal_list_del(&rp_ept->node);
	metal_mutex_release(&rdev->lock);
	/* free node and rp_ept */
	metal_free_memory(rp_ept);
}

/**
 * rpmsg_send_ns_message
 *
 * Sends name service announcement to remote device
 *
 * @param rdev    - pointer to remote device
 * @param rp_chnl - pointer to rpmsg channel
 * @param flags   - Channel creation/deletion flags
 *
 */
int rpmsg_send_ns_message(struct remote_device *rdev,
			   struct rpmsg_channel *rp_chnl, unsigned long flags)
{

	struct rpmsg_hdr rp_hdr;
	struct rpmsg_ns_msg ns_msg;
	unsigned short idx;
	unsigned long len;
	struct metal_io_region *io;
	void *shbuf;

	metal_mutex_acquire(&rdev->lock);

	/* Get Tx buffer. */
	shbuf = rpmsg_get_tx_buffer(rdev, &len, &idx);
	if (!shbuf) {
		metal_mutex_release(&rdev->lock);
		return -RPMSG_ERR_NO_BUFF;
	}

	/* Fill out name service data. */
	rp_hdr.dst = RPMSG_NS_EPT_ADDR;
	rp_hdr.len = sizeof(ns_msg);
	ns_msg.flags = flags;
	ns_msg.addr = rp_chnl->src;
	strncpy(ns_msg.name, rp_chnl->name, sizeof(ns_msg.name));

	io = rdev->proc->sh_buff.io;
	metal_io_block_write(io, metal_io_virt_to_offset(io, shbuf),
			&rp_hdr, sizeof(rp_hdr));
	metal_io_block_write(io,
			metal_io_virt_to_offset(io, RPMSG_LOCATE_DATA(shbuf)),
			&ns_msg, rp_hdr.len);

	/* Place the buffer on virtqueue. */
	rpmsg_enqueue_buffer(rdev, shbuf, len, idx);

	/* Notify the other side that it has data to process. */
	virtqueue_kick(rdev->tvq);

	metal_mutex_release(&rdev->lock);
	return RPMSG_SUCCESS;
}

/**
 * rpmsg_enqueue_buffers
 *
 * Places buffer on the virtqueue for consumption by the other side.
 *
 * @param rdev   - pointer to remote core
 * @param buffer - buffer pointer
 * @param len    - buffer length
 * @idx          - buffer index
 *
 * @return - status of function execution
 *
 */
int rpmsg_enqueue_buffer(struct remote_device *rdev, void *buffer,
			 unsigned long len, unsigned short idx)
{
	int status;
	struct metal_sg sg;
	struct metal_io_region *io;

	io = rdev->proc->sh_buff.io;
	if (rdev->role == RPMSG_REMOTE) {
		/* Initialize buffer node */
		sg.virt = buffer;
		sg.len = len;
		sg.io = io;
		status = virtqueue_add_buffer(rdev->tvq, &sg, 0, 1, buffer);
	} else {
		(void)sg;
		status = virtqueue_add_consumed_buffer(rdev->tvq, idx, len);
	}

	return status;
}

/**
 * rpmsg_return_buffer
 *
 * Places the used buffer back on the virtqueue.
 *
 * @param rdev   - pointer to remote core
 * @param buffer - buffer pointer
 * @param len    - buffer length
 * @param idx    - buffer index
 *
 */
void rpmsg_return_buffer(struct remote_device *rdev, void *buffer,
			 unsigned long len, unsigned short idx)
{
	struct metal_sg sg;

	if (rdev->role == RPMSG_REMOTE) {
		/* Initialize buffer node */
		sg.virt = buffer;
		sg.len = len;
		sg.io = rdev->proc->sh_buff.io;
		virtqueue_add_buffer(rdev->rvq, &sg, 0, 1, buffer);
	} else {
		(void)sg;
		virtqueue_add_consumed_buffer(rdev->rvq, idx, len);
	}
}

/**
 * rpmsg_get_tx_buffer
 *
 * Provides buffer to transmit messages.
 *
 * @param rdev - pointer to remote device
 * @param len  - length of returned buffer
 * @param idx  - buffer index
 *
 * return - pointer to buffer.
 */
void *rpmsg_get_tx_buffer(struct remote_device *rdev, unsigned long *len,
			  unsigned short *idx)
{
	void *data;

	if (rdev->role == RPMSG_REMOTE) {
		data = virtqueue_get_buffer(rdev->tvq, (uint32_t *) len, idx);
		if (data == RPMSG_NULL) {
			data = sh_mem_get_buffer(rdev->mem_pool);
			*len = RPMSG_BUFFER_SIZE;
		}
	} else {
		data =
		    virtqueue_get_available_buffer(rdev->tvq, idx,
						   (uint32_t *) len);
	}
	return data;
}

/**
 * rpmsg_get_rx_buffer
 *
 * Retrieves the received buffer from the virtqueue.
 *
 * @param rdev - pointer to remote device
 * @param len  - size of received buffer
 * @param idx  - index of buffer
 *
 * @return - pointer to received buffer
 *
 */
void *rpmsg_get_rx_buffer(struct remote_device *rdev, unsigned long *len,
			  unsigned short *idx)
{

	void *data;
	if (rdev->role == RPMSG_REMOTE) {
		data = virtqueue_get_buffer(rdev->rvq, (uint32_t *) len, idx);
	} else {
		data =
		    virtqueue_get_available_buffer(rdev->rvq, idx,
						   (uint32_t *) len);
	}
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

/**
 * rpmsg_free_buffer
 *
 * Frees the allocated buffers.
 *
 * @param rdev   - pointer to remote device
 * @param buffer - pointer to buffer to free
 *
 */
void rpmsg_free_buffer(struct remote_device *rdev, void *buffer)
{
	if (rdev->role == RPMSG_REMOTE) {
		sh_mem_free_buffer(buffer, rdev->mem_pool);
	}
}

/**
 * rpmsg_tx_callback
 *
 * Tx callback function.
 *
 * @param vq - pointer to virtqueue on which Tx is has been
 *             completed.
 *
 */
static void rpmsg_tx_callback(struct virtqueue *vq)
{
	struct remote_device *rdev;
	struct virtio_device *vdev;
	struct rpmsg_channel *rp_chnl;
	struct metal_list *node;

	vdev = (struct virtio_device *)vq->vq_dev;
	rdev = (struct remote_device *)vdev;

	/* Check if the remote device is master. */
	if (rdev->role == RPMSG_MASTER) {

		/* Notification is received from the master. Now the remote(us) can
		 * performs one of two operations;
		 *
		 * a. If name service announcement is supported then it will send NS message.
		 *    else
		 * b. It will update the channel state to active so that further communication
		 *    can take place.
		 */
		metal_list_for_each(&rdev->rp_channels, node) {
			rp_chnl = metal_container_of(node,
				struct rpmsg_channel, node);

			if (rp_chnl->state == RPMSG_CHNL_STATE_IDLE) {

				if (rdev->support_ns) {
					if (rpmsg_send_ns_message(rdev, rp_chnl,
						      RPMSG_NS_CREATE) ==
						RPMSG_SUCCESS)
						rp_chnl->state =
							RPMSG_CHNL_STATE_NS;
				} else {
					rp_chnl->state =
					    RPMSG_CHNL_STATE_ACTIVE;
				}

			}

		}
	}
}

/**
 * rpmsg_rx_callback
 *
 * Rx callback function.
 *
 * @param vq - pointer to virtqueue on which messages is received
 *
 */
void rpmsg_rx_callback(struct virtqueue *vq)
{
	struct remote_device *rdev;
	struct virtio_device *vdev;
	struct rpmsg_channel *rp_chnl;
	struct rpmsg_endpoint *rp_ept;
	struct rpmsg_hdr *rp_hdr;
	struct rpmsg_hdr_reserved *reserved;
	unsigned long len;
	unsigned short idx;

	vdev = (struct virtio_device *)vq->vq_dev;
	rdev = (struct remote_device *)vdev;

	metal_mutex_acquire(&rdev->lock);

	/* Process the received data from remote node */
	rp_hdr = (struct rpmsg_hdr *)rpmsg_get_rx_buffer(rdev, &len, &idx);

	metal_mutex_release(&rdev->lock);

	while (rp_hdr) {
		/* Get the channel node from the remote device channels list. */
		metal_mutex_acquire(&rdev->lock);
		rp_ept = rpmsg_rdev_get_endpoint_from_addr(rdev, rp_hdr->dst);
		metal_mutex_release(&rdev->lock);

		if (!rp_ept) {
			/* Fatal error no endpoint for the given dst addr. */
			return;
		}

		rp_chnl = rp_ept->rp_chnl;

		if ((rp_chnl) && (rp_chnl->state == RPMSG_CHNL_STATE_NS)) {
			/* First message from RPMSG Master, update channel
			 * destination address and state */
			if (rp_ept->addr == RPMSG_NS_EPT_ADDR) {
				rp_ept->cb(rp_chnl,
						  (void *)RPMSG_LOCATE_DATA(rp_hdr),
						  rp_hdr->len, rdev,
						  rp_hdr->src);
			} else {
				rp_chnl->dst = rp_hdr->src;
				rp_chnl->state = RPMSG_CHNL_STATE_ACTIVE;

				/* Notify channel creation to application */
				if (rdev->channel_created) {
					rdev->channel_created(rp_chnl);
				}
			}
		} else {
			rp_ept->cb(rp_chnl, (void *)RPMSG_LOCATE_DATA(rp_hdr), rp_hdr->len,
				   rp_ept->priv, rp_hdr->src);
		}

		metal_mutex_acquire(&rdev->lock);

		/* Check whether callback wants to hold buffer */
		if (rp_hdr->reserved & RPMSG_BUF_HELD)
		{
			/* 'rp_hdr->reserved' field is now used as storage for
			 * 'idx' to release buffer later */
			reserved = (struct rpmsg_hdr_reserved*)&rp_hdr->reserved;
			reserved->idx = (uint16_t)idx;
		} else {
			/* Return used buffers. */
			rpmsg_return_buffer(rdev, rp_hdr, len, idx);
		}

		rp_hdr =
		    (struct rpmsg_hdr *)rpmsg_get_rx_buffer(rdev, &len, &idx);
		metal_mutex_release(&rdev->lock);
	}
}

/**
 * rpmsg_ns_callback
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
 * @return - none
 */
void rpmsg_ns_callback(struct rpmsg_channel *server_chnl, void *data, int len,
		       void *priv, unsigned long src)
{
	struct remote_device *rdev;
	struct rpmsg_channel *rp_chnl;
	struct rpmsg_ns_msg *ns_msg;

	(void)server_chnl;
	(void)src;
	(void)len;

	rdev = (struct remote_device *)priv;

	//FIXME: This assumes same name string size for channel name both on master
	//and remote. If this is not the case then we will have to parse the
	//message contents.

	ns_msg = (struct rpmsg_ns_msg *)data;

	if (ns_msg->flags & RPMSG_NS_DESTROY) {
		metal_mutex_acquire(&rdev->lock);
		rp_chnl = rpmsg_rdev_get_chnl_from_id(rdev, ns_msg->name);
		metal_mutex_release(&rdev->lock);
		if (rp_chnl) {
			if (rdev->channel_destroyed) {
				rdev->channel_destroyed(rp_chnl);
			}
			rpmsg_destroy_ept(rp_chnl->rp_ept);
			_rpmsg_delete_channel(rp_chnl);
		}
	} else {
		metal_mutex_acquire(&rdev->lock);
		rp_chnl = rpmsg_rdev_get_chnl_from_id(rdev, ns_msg->name);
		metal_mutex_release(&rdev->lock);
		if (!rp_chnl) {
			rp_chnl = _rpmsg_create_channel(rdev, ns_msg->name,
							0x00,
							ns_msg->addr);
		}
		if (rp_chnl) {
			metal_mutex_acquire(&rdev->lock);
			rp_chnl->state = RPMSG_CHNL_STATE_ACTIVE;
			rp_chnl->dst = ns_msg->addr;
			metal_mutex_release(&rdev->lock);
			/* Create default endpoint for channel */
			if (!rp_chnl->rp_ept) {
				rp_chnl->rp_ept =
					rpmsg_create_ept(rp_chnl,
							 rdev->default_cb, rdev,
							 RPMSG_ADDR_ANY);
				if (rp_chnl->rp_ept) {
					rp_chnl->src = rp_chnl->rp_ept->addr;
					rpmsg_send_ns_message(rdev,
							      rp_chnl,
							      RPMSG_NS_CREATE);
				}
			}
			if (rdev->channel_created)
				rdev->channel_created(rp_chnl);
		}
	}
}

/**
 * rpmsg_get_address
 *
 * This function provides unique 32 bit address.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 *
 * return - a unique address
 */
int rpmsg_get_address(unsigned long *bitmap, int size)
{
	int addr = -1;
	int i, tmp32;

	/* Find first available buffer */
	for (i = 0; i < size; i++) {
		tmp32 = get_first_zero_bit(bitmap[i]);

		if (tmp32 < 32) {
			addr = tmp32 + (i*32);
			bitmap[i] |= (1 << tmp32);
			break;
		}
	}

	return addr;
}

/**
 * rpmsg_release_address
 *
 * Frees the given address.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 * @param addr   - address to free
 *
 * return - none
 */
int rpmsg_release_address(unsigned long *bitmap, int size, int addr)
{
	unsigned int i, j;
	unsigned long mask = 1;

	if (addr >= size * 32)
		return -1;

	/* Mark the addr as available */
	i = addr / 32;
	j = addr % 32;

	mask = mask << j;
	bitmap[i] = bitmap[i] & (~mask);

	return RPMSG_SUCCESS;
}

/**
 * rpmsg_is_address_set
 *
 * Checks whether address is used or free.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 * @param addr   - address to free
 *
 * return - TRUE/FALSE
 */
int rpmsg_is_address_set(unsigned long *bitmap, int size, int addr)
{
	int i, j;
	unsigned long mask = 1;

	if (addr >= size * 32)
		return -1;

	/* Mark the id as available */
	i = addr / 32;
	j = addr % 32;
	mask = mask << j;

	return (bitmap[i] & mask);
}

/**
 * rpmsg_set_address
 *
 * Marks the address as consumed.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 * @param addr   - address to free
 *
 * return - none
 */
int rpmsg_set_address(unsigned long *bitmap, int size, int addr)
{
	int i, j;
	unsigned long mask = 1;

	if (addr >= size * 32)
		return -1;

	/* Mark the id as available */
	i = addr / 32;
	j = addr % 32;
	mask = mask << j;
	bitmap[i] |= mask;

	return RPMSG_SUCCESS;
}
