/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       rpmsg.c
 *
 * COMPONENT
 *
 *       OpenAMP stack.
 *
 * DESCRIPTION
 *
 * Main file for the RPMSG driver. This file implements APIs as defined by
 * RPMSG documentation(Linux docs) and also provides some utility functions.
 *
 * RPMSG driver represents each processor/core to which it communicates with
 * remote_device control block.
 * Each remote device(processor) defines its role in the communication i.e
 * whether it is RPMSG Master or Remote. If the device(processor) to which
 * driver is talking is RPMSG master then RPMSG driver implicitly behaves as
 * Remote and vice versa.
 * RPMSG Master is responsible for initiating communications with the Remote
 * and shared buffers management. Terms remote device/core/proc are used
 * interchangeably for the processor to which RPMSG driver is communicating
 * irrespective of the fact whether it is RPMSG Remote or Master.
 *
 **************************************************************************/
#include <string.h>
#include <openamp/rpmsg.h>
#include <metal/sys.h>
#include <metal/assert.h>
#include <metal/cache.h>
#include <metal/sleep.h>

/**
 * rpmsg_init
 *
 * Thus function allocates and initializes the rpmsg driver resources for
 * given hil_proc. The successful return from this function leaves
 * fully enabled IPC link.
 *
 * @param proc              - pointer to hil_proc
 * @param rdev              - pointer to newly created remote device
 * @param channel_created   - callback function for channel creation
 * @param channel_destroyed - callback function for channel deletion
 * @param default_cb        - default callback for channel I/O
 * @param role              - role of the other device, Master or Remote
 *
 * @return - status of function execution
 *
 */

int rpmsg_init(struct hil_proc *proc,
	       struct remote_device **rdev,
	       rpmsg_chnl_cb_t channel_created,
	       rpmsg_chnl_cb_t channel_destroyed,
	       rpmsg_rx_cb_t default_cb, int role)
{
	int status;

	/* Initialize the remote device for given cpu id */
	status = rpmsg_rdev_init(proc, rdev, role,
				 channel_created,
				 channel_destroyed, default_cb);
	if (status == RPMSG_SUCCESS) {
		/* Kick off IPC with the remote device */
		status = rpmsg_start_ipc(*rdev);
	}

	/* Deinit system in case of error */
	if (status != RPMSG_SUCCESS) {
		rpmsg_deinit(*rdev);
	}

	return status;
}

/**
 * rpmsg_deinit
 *
 * Thus function frees rpmsg driver resources for given remote device.
 *
 * @param rdev  -  pointer to device to de-init
 *
 */

void rpmsg_deinit(struct remote_device *rdev)
{
	if (rdev) {
		rpmsg_rdev_deinit(rdev);
	}
}

/**
 * This function sends rpmsg "message" to remote device.
 *
 * @param rp_chnl - pointer to rpmsg channel
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

int rpmsg_send_offchannel_raw(struct rpmsg_channel *rp_chnl, uint32_t src,
			      uint32_t dst, const void *data,
			      int size, int wait)
{
	struct remote_device *rdev;
	struct rpmsg_hdr rp_hdr;
	void *buffer;
	unsigned short idx;
	int tick_count = 0;
	unsigned long buff_len;
	int ret;
	struct metal_io_region *io;

	if (!rp_chnl || !data) {
		return RPMSG_ERR_PARAM;
	}

	/* Get the associated remote device for channel. */
	rdev = rp_chnl->rdev;

	/* Validate device state */
	if (rp_chnl->state != RPMSG_CHNL_STATE_ACTIVE
	    || rdev->state != RPMSG_DEV_STATE_ACTIVE) {
		return RPMSG_ERR_DEV_STATE;
	}

	if (size > (rpmsg_get_buffer_size(rp_chnl))) {
		return RPMSG_ERR_BUFF_SIZE;
	}

	/* Lock the device to enable exclusive access to virtqueues */
	metal_mutex_acquire(&rdev->lock);
	/* Get rpmsg buffer for sending message. */
	buffer = rpmsg_get_tx_buffer(rdev, &buff_len, &idx);
	/* Unlock the device */
	metal_mutex_release(&rdev->lock);

	if (!buffer && !wait) {
		return RPMSG_ERR_NO_BUFF;
	}

	while (!buffer) {
		/*
		 * Wait parameter is true - pool the buffer for
		 * 15 secs as defined by the APIs.
		 */
		metal_sleep_usec(RPMSG_TICKS_PER_INTERVAL);
		metal_mutex_acquire(&rdev->lock);
		buffer = rpmsg_get_tx_buffer(rdev, &buff_len, &idx);
		metal_mutex_release(&rdev->lock);
		tick_count += RPMSG_TICKS_PER_INTERVAL;
		if (!buffer && (tick_count >=
		    (RPMSG_TICK_COUNT / RPMSG_TICKS_PER_INTERVAL))) {
			return RPMSG_ERR_NO_BUFF;
		}
	}

	/* Initialize RPMSG header. */
	rp_hdr.dst = dst;
	rp_hdr.src = src;
	rp_hdr.len = size;
	rp_hdr.reserved = 0;

	/* Copy data to rpmsg buffer. */
	io = rdev->proc->sh_buff.io;
	metal_io_block_write(io,
			metal_io_virt_to_offset(io, buffer),
			&rp_hdr, sizeof(rp_hdr));
	metal_io_block_write(io,
			metal_io_virt_to_offset(io, RPMSG_LOCATE_DATA(buffer)),
			data, size);
	metal_mutex_acquire(&rdev->lock);

	/* Enqueue buffer on virtqueue. */
	ret = rpmsg_enqueue_buffer(rdev, buffer, buff_len, idx);
	metal_assert(ret == VQUEUE_SUCCESS);
	/* Let the other side know that there is a job to process. */
	virtqueue_kick(rdev->tvq);

	metal_mutex_release(&rdev->lock);

	return size;
}

/**
 * rpmsg_get_buffer_size
 *
 * Returns buffer size available for sending messages.
 *
 * @param channel - pointer to rpmsg channel
 *
 * @return - buffer size
 *
 */
int rpmsg_get_buffer_size(struct rpmsg_channel *rp_chnl)
{
	struct remote_device *rdev;
	int length;

	/* Get associated remote device for channel. */
	rdev = rp_chnl->rdev;

	metal_mutex_acquire(&rdev->lock);

	if (rdev->role == RPMSG_REMOTE) {
		/*
		 * If device role is Remote then buffers are provided by us
		 * (RPMSG Master), so just provide the macro.
		 */
		length = RPMSG_BUFFER_SIZE - sizeof(struct rpmsg_hdr);
	} else {
		/*
		 * If other core is Master then buffers are provided by it,
		 * so get the buffer size from the virtqueue.
		 */
		length =
		    (int)virtqueue_get_desc_size(rdev->tvq) -
		    sizeof(struct rpmsg_hdr);
	}

	metal_mutex_release(&rdev->lock);

	return length;
}

void rpmsg_hold_rx_buffer(struct rpmsg_channel *rpdev, void *rxbuf)
{
	struct rpmsg_hdr *rp_hdr = NULL;
	if (!rpdev || !rxbuf)
	    return;

	rp_hdr = RPMSG_HDR_FROM_BUF(rxbuf);

	/* set held status to keep buffer */
	rp_hdr->reserved |= RPMSG_BUF_HELD;
}

void rpmsg_release_rx_buffer(struct rpmsg_channel *rpdev, void *rxbuf)
{
	struct rpmsg_hdr *hdr;
	struct remote_device *rdev;
	struct rpmsg_hdr_reserved * reserved = NULL;
	unsigned int len;

	if (!rpdev || !rxbuf)
	    return;

	rdev = rpdev->rdev;
	hdr = RPMSG_HDR_FROM_BUF(rxbuf);

	/* Get the pointer to the reserved field that contains buffer size
	 * and the index */
	reserved = (struct rpmsg_hdr_reserved*)&hdr->reserved;
	hdr->reserved &= (~RPMSG_BUF_HELD);
	len = (unsigned int)virtqueue_get_buffer_length(rdev->rvq,
						reserved->idx);

	metal_mutex_acquire(&rdev->lock);

	/* Return used buffer, with total length
	   (header length + buffer size). */
	rpmsg_return_buffer(rdev, hdr, (unsigned long)len, reserved->idx);

	metal_mutex_release(&rdev->lock);
}

void *rpmsg_get_tx_payload_buffer(struct rpmsg_channel *rpdev, uint32_t *size,
				 int wait)
{
	struct rpmsg_hdr *hdr;
	struct remote_device *rdev;
	struct rpmsg_hdr_reserved *reserved;
	unsigned short idx;
	unsigned long buff_len, tick_count = 0;

	if (!rpdev || !size)
		return NULL;

	rdev = rpdev->rdev;

	metal_mutex_acquire(&rdev->lock);

	/* Get tx buffer from vring */
	hdr = (struct rpmsg_hdr *) rpmsg_get_tx_buffer(rdev, &buff_len, &idx);

	metal_mutex_release(&rdev->lock);

	if (!hdr && !wait) {
		return NULL;
	} else {
		while (!hdr) {
			/*
			 * Wait parameter is true - pool the buffer for
			 * 15 secs as defined by the APIs.
			 */
			metal_sleep_usec(RPMSG_TICKS_PER_INTERVAL);
			metal_mutex_acquire(&rdev->lock);
			hdr = (struct rpmsg_hdr *) rpmsg_get_tx_buffer(rdev, &buff_len, &idx);
			metal_mutex_release(&rdev->lock);
			tick_count += RPMSG_TICKS_PER_INTERVAL;
			if (tick_count >= (RPMSG_TICK_COUNT / RPMSG_TICKS_PER_INTERVAL)) {
					return NULL;
			}
		}

		/* Store the index into the reserved field to be used when sending */
		reserved = (struct rpmsg_hdr_reserved*)&hdr->reserved;
		reserved->idx = (uint16_t)idx;

		/* Actual data buffer size is vring buffer size minus rpmsg header length */
		*size = (uint32_t)(buff_len - sizeof(struct rpmsg_hdr));
		return (void *)RPMSG_LOCATE_DATA(hdr);
	}
}

int rpmsg_send_offchannel_nocopy(struct rpmsg_channel *rpdev, uint32_t src,
				 uint32_t dst, void *txbuf, int len)
{
	struct rpmsg_hdr *hdr;
	struct remote_device *rdev;
	struct rpmsg_hdr_reserved * reserved = NULL;
	int status;

	if (!rpdev || !txbuf)
	    return RPMSG_ERR_PARAM;

	rdev = rpdev->rdev;
	hdr = RPMSG_HDR_FROM_BUF(txbuf);

	/* Initialize RPMSG header. */
	hdr->dst = dst;
	hdr->src = src;
	hdr->len = len;
	hdr->flags = 0;
	hdr->reserved &= (~RPMSG_BUF_HELD);

	/* Get the pointer to the reserved field that contains buffer size and
	 * the index */
	reserved = (struct rpmsg_hdr_reserved*)&hdr->reserved;

	metal_mutex_acquire(&rdev->lock);

	status = rpmsg_enqueue_buffer(rdev, hdr,
			(unsigned long)virtqueue_get_buffer_length(
			rdev->tvq, reserved->idx),
			reserved->idx);
	if (status == RPMSG_SUCCESS) {
		/* Let the other side know that there is a job to process. */
		virtqueue_kick(rdev->tvq);
		/* Return size of data sent */
		status = len;
	}

	metal_mutex_release(&rdev->lock);

	return status;
}

/**
 * rpmsg_create_ept
 *
 * This function creates rpmsg endpoint for the rpmsg channel.
 *
 * @param channel - pointer to rpmsg channel
 * @param cb      - Rx completion call back
 * @param priv    - private data
 * @param addr    - endpoint src address
 *
 * @return - pointer to endpoint control block
 *
 */
struct rpmsg_endpoint *rpmsg_create_ept(struct rpmsg_channel *rp_chnl,
					rpmsg_rx_cb_t cb, void *priv,
					uint32_t addr)
{

	struct remote_device *rdev = RPMSG_NULL;
	struct rpmsg_endpoint *rp_ept = RPMSG_NULL;

	if (!rp_chnl || !cb) {
		return RPMSG_NULL;
	}

	rdev = rp_chnl->rdev;

	metal_mutex_acquire(&rdev->lock);
	rp_ept = rpmsg_rdev_get_endpoint_from_addr(rdev, addr);
	metal_mutex_release(&rdev->lock);
	if (!rp_ept) {
		rp_ept = _create_endpoint(rdev, cb, priv, addr);

		if (rp_ept) {
			rp_ept->rp_chnl = rp_chnl;
		}
	} else {
		return RPMSG_NULL;
	}

	return rp_ept;
}

/**
 * rpmsg_destroy_ept
 *
 * This function deletes rpmsg endpoint and performs cleanup.
 *
 * @param rp_ept - pointer to endpoint to destroy
 *
 */
void rpmsg_destroy_ept(struct rpmsg_endpoint *rp_ept)
{

	struct remote_device *rdev;
	struct rpmsg_channel *rp_chnl;

	if (!rp_ept)
		return;

	rp_chnl = rp_ept->rp_chnl;
	rdev = rp_chnl->rdev;

	_destroy_endpoint(rdev, rp_ept);
}

/**
 * rpmsg_create_channel
 *
 * This function provides facility to create channel dynamically. It sends
 * Name Service announcement to remote device to let it know about the channel
 * creation. There must be an active communication among the cores (or atleast
 * one rpmsg channel must already exist) before using this API to create new
 * channels.
 *
 * @param rdev - pointer to remote device
 * @param name - channel name
 *
 * @return - pointer to new rpmsg channel
 *
 */
struct rpmsg_channel *rpmsg_create_channel(struct remote_device *rdev,
					   char *name)
{

	struct rpmsg_channel *rp_chnl;
	struct rpmsg_endpoint *rp_ept;

	if (!rdev || !name) {
		return RPMSG_NULL;
	}

	/* Create channel instance */
	rp_chnl = _rpmsg_create_channel(rdev, name, RPMSG_NS_EPT_ADDR,
					RPMSG_NS_EPT_ADDR);
	if (!rp_chnl) {
		return RPMSG_NULL;
	}

	/* Create default endpoint for the channel */
	rp_ept = rpmsg_create_ept(rp_chnl, rdev->default_cb, rdev,
				  RPMSG_ADDR_ANY);

	if (!rp_ept) {
		_rpmsg_delete_channel(rp_chnl);
		return RPMSG_NULL;
	}

	rp_chnl->rp_ept = rp_ept;
	rp_chnl->src = rp_ept->addr;
	rp_chnl->state = RPMSG_CHNL_STATE_NS;

	/* Notify the application of channel creation event */
	if (rdev->channel_created) {
		rdev->channel_created(rp_chnl);
	}

	/* Send NS announcement to remote processor */
	rpmsg_send_ns_message(rdev, rp_chnl, RPMSG_NS_CREATE);

	return rp_chnl;
}

/**
 * rpmsg_delete_channel
 *
 * Deletes the given RPMSG channel. The channel must first be created with the
 * rpmsg_create_channel API.
 *
 * @param rp_chnl - pointer to rpmsg channel to delete
 *
 */
void rpmsg_delete_channel(struct rpmsg_channel *rp_chnl)
{

	struct remote_device *rdev;

	if (!rp_chnl) {
		return;
	}

	rdev = rp_chnl->rdev;

	if (rp_chnl->state > RPMSG_CHNL_STATE_IDLE) {
		/* Notify the other processor that channel no longer exists */
		rpmsg_send_ns_message(rdev, rp_chnl, RPMSG_NS_DESTROY);
	}

	/* Notify channel deletion to application */
	if (rdev->channel_destroyed) {
		rdev->channel_destroyed(rp_chnl);
	}

	rpmsg_destroy_ept(rp_chnl->rp_ept);
	_rpmsg_delete_channel(rp_chnl);

	return;
}
