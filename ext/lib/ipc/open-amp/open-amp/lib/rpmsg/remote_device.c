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
 *       remote_device.c
 *
 * COMPONENT
 *
 *       OpenAMP Stack
 *
 * DESCRIPTION
 *
 * This file provides services to manage the remote devices.It also implements
 * the interface defined by the virtio and provides few other utility functions.
 *
 *
 **************************************************************************/

#include <string.h>
#include <openamp/rpmsg.h>
#include <openamp/remoteproc.h>
#include <metal/utilities.h>
#include <metal/alloc.h>
#include <metal/atomic.h>
#include <metal/cpu.h>

/* Macro to initialize vring HW info */
#define INIT_VRING_ALLOC_INFO(ring_info,vring_hw)                             \
                         (ring_info).vaddr  = (vring_hw).vaddr;               \
                         (ring_info).align     = (vring_hw).align;             \
                         (ring_info).num_descs = (vring_hw).num_descs

/* Local functions */
static int rpmsg_rdev_init_channels(struct remote_device *rdev);

/* Ops table for virtio device */
virtio_dispatch rpmsg_rdev_config_ops = {
	rpmsg_rdev_create_virtqueues,
	rpmsg_rdev_get_status,
	rpmsg_rdev_set_status,
	rpmsg_rdev_get_feature,
	rpmsg_rdev_set_feature,
	rpmsg_rdev_negotiate_feature,
	rpmsg_rdev_read_config,
	rpmsg_rdev_write_config,
	rpmsg_rdev_reset
};

/**
 * rpmsg_memb_match
 *
 * This internal function checks if the contents in two memories matches byte
 * by byte. This function is needed because memcmp() or strcmp() does not
 * always work across different memories.
 *
 * @param ptr1 - pointer to memory
 * @param ptr2 - pointer to memory
 * @param n - number of bytes to compare
 *
 * @return 0 if the contents in the two memories matches, otherwise -1.
 */
static int rpmsg_memb_match(const void *ptr1, const void *ptr2, size_t n)
{
	size_t i;
	const unsigned char *tmp1, *tmp2;

	tmp1 = ptr1;
	tmp2 = ptr2;
	for (i = 0; i < n; i++, tmp1++, tmp2++) {
		if (*tmp1 != *tmp2)
			return -1;
	}

	return 0;
}

/**
 * rpmsg_rdev_init
 *
 * This function creates and initializes the remote device. The remote device
 * encapsulates virtio device.
 *
 * @param proc              - pointer to hil_proc
 * @param rdev              - pointer to newly created remote device
 * @param role              - role of the other device, Master or Remote
 * @param channel_created   - callback function for channel creation
 * @param channel_destroyed - callback function for channel deletion
 * @param default_cb        - default callback for channel
 *
 * @return - status of function execution
 *
 */
int rpmsg_rdev_init(struct hil_proc *proc,
		    struct remote_device **rdev, int role,
		    rpmsg_chnl_cb_t channel_created,
		    rpmsg_chnl_cb_t channel_destroyed, rpmsg_rx_cb_t default_cb)
{

	struct remote_device *rdev_loc;
	struct virtio_device *virt_dev;
	struct proc_shm *shm;
	int status;

	if (!proc)
		return RPMSG_ERR_PARAM;
	/* Initialize HIL data structures for given device */
	if (hil_init_proc(proc))
		return RPMSG_ERR_DEV_INIT;

	/* Create software representation of remote processor. */
	rdev_loc = (struct remote_device *)metal_allocate_memory(sizeof(struct remote_device));

	if (!rdev_loc) {
		return RPMSG_ERR_NO_MEM;
	}

	memset(rdev_loc, 0x00, sizeof(struct remote_device));
	metal_mutex_init(&rdev_loc->lock);

	rdev_loc->proc = proc;
	rdev_loc->role = role;
	rdev_loc->channel_created = channel_created;
	rdev_loc->channel_destroyed = channel_destroyed;
	rdev_loc->default_cb = default_cb;

	/* Restrict the ept address - zero address can't be assigned */
	rdev_loc->bitmap[0] = 1;

	/* Initialize the virtio device */
	virt_dev = &rdev_loc->virt_dev;
	virt_dev->device = proc;
	virt_dev->func = &rpmsg_rdev_config_ops;
	if (virt_dev->func->set_features != RPMSG_NULL) {
		virt_dev->func->set_features(virt_dev, proc->vdev.dfeatures);
	}

	if (rdev_loc->role == RPMSG_REMOTE) {
		/*
		 * Since device is RPMSG Remote so we need to manage the
		 * shared buffers. Create shared memory pool to handle buffers.
		 */
		shm = hil_get_shm_info(proc);
		rdev_loc->mem_pool =
		    sh_mem_create_pool(shm->start_addr, shm->size,
				       RPMSG_BUFFER_SIZE);

		if (!rdev_loc->mem_pool) {
			return RPMSG_ERR_NO_MEM;
		}
	}

	if (!rpmsg_rdev_remote_ready(rdev_loc))
		return RPMSG_ERR_DEV_INIT;

	/* Initialize endpoints list */
	metal_list_init(&rdev_loc->rp_endpoints);

	/* Initialize channels for RPMSG Remote */
	status = rpmsg_rdev_init_channels(rdev_loc);

	if (status != RPMSG_SUCCESS) {
		return status;
	}

	*rdev = rdev_loc;

	return RPMSG_SUCCESS;
}

/**
 * rpmsg_rdev_deinit
 *
 * This function un-initializes the remote device.
 *
 * @param rdev - pointer to remote device to deinit.
 *
 * @return - none
 *
 */
void rpmsg_rdev_deinit(struct remote_device *rdev)
{
	struct metal_list *node;
	struct rpmsg_channel *rp_chnl;
	struct rpmsg_endpoint *rp_ept;


	while(!metal_list_is_empty(&rdev->rp_channels)) {
		node = rdev->rp_channels.next;
		rp_chnl = metal_container_of(node, struct rpmsg_channel, node);

		if (rdev->channel_destroyed) {
			rdev->channel_destroyed(rp_chnl);
		}

		if ((rdev->support_ns) && (rdev->role == RPMSG_MASTER)) {
			rpmsg_send_ns_message(rdev, rp_chnl, RPMSG_NS_DESTROY);
		}

		/* Delete default endpoint for channel */
		if (rp_chnl->rp_ept) {
			rpmsg_destroy_ept(rp_chnl->rp_ept);
		}

		_rpmsg_delete_channel(rp_chnl);
	}

	/* Delete name service endpoint */
	metal_mutex_acquire(&rdev->lock);
	rp_ept = rpmsg_rdev_get_endpoint_from_addr(rdev, RPMSG_NS_EPT_ADDR);
	metal_mutex_release(&rdev->lock);
	if (rp_ept) {
		_destroy_endpoint(rdev, rp_ept);
	}

	metal_mutex_acquire(&rdev->lock);
	rdev->rvq = 0;
	rdev->tvq = 0;
	if (rdev->mem_pool) {
		sh_mem_delete_pool(rdev->mem_pool);
		rdev->mem_pool = 0;
	}
	metal_mutex_release(&rdev->lock);
	hil_free_vqs(&rdev->virt_dev);

	metal_mutex_deinit(&rdev->lock);

	metal_free_memory(rdev);
}

/**
 * rpmsg_rdev_get_chnl_from_id
 *
 * This function returns channel node based on channel name. It must be called
 * with mutex locked.
 *
 * @param stack      - pointer to remote device
 * @param rp_chnl_id - rpmsg channel name
 *
 * @return - rpmsg channel
 *
 */
struct rpmsg_channel *rpmsg_rdev_get_chnl_from_id(struct remote_device *rdev,
					       char *rp_chnl_id)
{
	struct rpmsg_channel *rp_chnl;
	struct metal_list *node;

	metal_list_for_each(&rdev->rp_channels, node) {
		rp_chnl = metal_container_of(node, struct rpmsg_channel, node);
		if (!rpmsg_memb_match(rp_chnl->name, rp_chnl_id,
				     sizeof(rp_chnl->name))) {
			return rp_chnl;
		}
	}

	return RPMSG_NULL;
}

/**
 * rpmsg_rdev_get_endpoint_from_addr
 *
 * This function returns endpoint node based on src address. It must be called
 * with mutex locked.
 *
 * @param rdev - pointer remote device control block
 * @param addr - src address
 *
 * @return - rpmsg endpoint
 *
 */
struct rpmsg_endpoint *rpmsg_rdev_get_endpoint_from_addr(struct remote_device *rdev,
						unsigned long addr)
{
	struct rpmsg_endpoint *rp_ept;
	struct metal_list *node;

	metal_list_for_each(&rdev->rp_endpoints, node) {
		rp_ept = metal_container_of(node,
				struct rpmsg_endpoint, node);
		if (rp_ept->addr == addr) {
			return rp_ept;
		}
	}

	return RPMSG_NULL;
}

/*
 * rpmsg_rdev_notify
 *
 * This function checks whether remote device is up or not. If it is up then
 * notification is sent based on device role to start IPC.
 *
 * @param rdev - pointer to remote device
 *
 * @return - status of function execution
 *
 */
int rpmsg_rdev_notify(struct remote_device *rdev)
{
	struct virtio_device *vdev = &rdev->virt_dev;

	hil_vdev_notify(vdev);

	return RPMSG_SUCCESS;
}

/**
 * rpmsg_rdev_init_channels
 *
 * This function is only applicable to RPMSG remote. It obtains channel IDs
 * from the HIL and creates RPMSG channels corresponding to each ID.
 *
 * @param rdev - pointer to remote device
 *
 * @return  - status of function execution
 *
 */
int rpmsg_rdev_init_channels(struct remote_device *rdev)
{
	struct rpmsg_channel *rp_chnl;
	struct proc_chnl *chnl_info;
	int num_chnls, idx;

	metal_list_init(&rdev->rp_channels);
	if (rdev->role == RPMSG_MASTER) {

		chnl_info = hil_get_chnl_info(rdev->proc, &num_chnls);
		for (idx = 0; idx < num_chnls; idx++) {

			rp_chnl =
			    _rpmsg_create_channel(rdev, chnl_info[idx].name,
						  0x00, RPMSG_NS_EPT_ADDR);
			if (!rp_chnl) {
				return RPMSG_ERR_NO_MEM;
			}

			rp_chnl->rp_ept =
			    rpmsg_create_ept(rp_chnl, rdev->default_cb, rdev,
					     RPMSG_ADDR_ANY);

			if (!rp_chnl->rp_ept) {
				return RPMSG_ERR_NO_MEM;
			}

			rp_chnl->src = rp_chnl->rp_ept->addr;
		}
	}

	return RPMSG_SUCCESS;
}

/**
 * check if the remote is ready to start RPMsg communication
 */
int rpmsg_rdev_remote_ready(struct remote_device *rdev)
{
	struct virtio_device *vdev = &rdev->virt_dev;
	uint8_t status;
	if (rdev->role == RPMSG_MASTER) {
		while (1) {
			status = vdev->func->get_status(vdev);
			/* Busy wait until the remote is ready */
			if (status & VIRTIO_CONFIG_STATUS_NEEDS_RESET) {
				rpmsg_rdev_set_status(vdev, 0);
				hil_vdev_notify(vdev);
			} else if (status & VIRTIO_CONFIG_STATUS_DRIVER_OK) {
				return true;
			}
			metal_cpu_yield();
		}
	} else {
		return true;
	}
	/* Never come here */
	return false;
}

/**
 *------------------------------------------------------------------------
 * The rest of the file implements the virtio device interface as defined
 * by the virtio.h file.
 *------------------------------------------------------------------------
 */
int rpmsg_rdev_create_virtqueues(struct virtio_device *dev, int flags, int nvqs,
				 const char *names[], vq_callback * callbacks[],
				 struct virtqueue *vqs_[])
{
	struct remote_device *rdev;
	struct vring_alloc_info ring_info;
	struct virtqueue *vqs[RPMSG_MAX_VQ_PER_RDEV];
	struct proc_vring *vring_table;
	void *buffer;
	struct metal_sg sg;
	int idx, num_vrings, status;

	(void)flags;
	(void)vqs_;

	rdev = (struct remote_device *)dev;

	/* Get the vring HW info for the given virtio device */
	vring_table = hil_get_vring_info(&rdev->proc->vdev, &num_vrings);

	if (num_vrings > nvqs) {
		return RPMSG_ERR_MAX_VQ;
	}

	/* Create virtqueue for each vring. */
	for (idx = 0; idx < num_vrings; idx++) {

		INIT_VRING_ALLOC_INFO(ring_info, vring_table[idx]);

		if (rdev->role == RPMSG_REMOTE) {
			metal_io_block_set(vring_table[idx].io,
				metal_io_virt_to_offset(vring_table[idx].io,
							ring_info.vaddr),
				0x00,
				vring_size(vring_table[idx].num_descs,
				vring_table[idx].align));
		}

		status =
		    virtqueue_create(dev, idx, (char *)names[idx], &ring_info,
				     callbacks[idx], hil_vring_notify,
				     rdev->proc->sh_buff.io,
				     &vqs[idx]);

		if (status != RPMSG_SUCCESS) {
			return status;
		}
	}

	//FIXME - a better way to handle this , tx for master is rx for remote and vice versa.
	if (rdev->role == RPMSG_MASTER) {
		rdev->tvq = vqs[0];
		rdev->rvq = vqs[1];
	} else {
		rdev->tvq = vqs[1];
		rdev->rvq = vqs[0];
	}

	if (rdev->role == RPMSG_REMOTE) {
		sg.io = rdev->proc->sh_buff.io;
		sg.len = RPMSG_BUFFER_SIZE;
		for (idx = 0; ((idx < rdev->rvq->vq_nentries)
			       && ((unsigned)idx < rdev->mem_pool->total_buffs / 2));
		     idx++) {

			/* Initialize TX virtqueue buffers for remote device */
			buffer = sh_mem_get_buffer(rdev->mem_pool);

			if (!buffer) {
				return RPMSG_ERR_NO_BUFF;
			}

			sg.virt = buffer;

			metal_io_block_set(sg.io,
				metal_io_virt_to_offset(sg.io, buffer),
				0x00,
				RPMSG_BUFFER_SIZE);
			status =
			    virtqueue_add_buffer(rdev->rvq, &sg, 0, 1,
						 buffer);

			if (status != RPMSG_SUCCESS) {
				return status;
			}
		}
	}

	return RPMSG_SUCCESS;
}

unsigned char rpmsg_rdev_get_status(struct virtio_device *dev)
{
	struct hil_proc *proc = dev->device;
	struct proc_vdev *pvdev = &proc->vdev;
	struct fw_rsc_vdev *vdev_rsc = pvdev->vdev_info;

	if (!vdev_rsc)
		return -1;

	atomic_thread_fence(memory_order_seq_cst);
	return vdev_rsc->status;
}

void rpmsg_rdev_set_status(struct virtio_device *dev, unsigned char status)
{
	struct hil_proc *proc = dev->device;
	struct proc_vdev *pvdev = &proc->vdev;
	struct fw_rsc_vdev *vdev_rsc = pvdev->vdev_info;

	if (!vdev_rsc)
		return;

	vdev_rsc->status = status;

	atomic_thread_fence(memory_order_seq_cst);
}

uint32_t rpmsg_rdev_get_feature(struct virtio_device *dev)
{
	return dev->features;
}

void rpmsg_rdev_set_feature(struct virtio_device *dev, uint32_t feature)
{
	dev->features |= feature;
}

uint32_t rpmsg_rdev_negotiate_feature(struct virtio_device *dev,
				      uint32_t features)
{
	(void)dev;
	(void)features;

	return 0;
}

/*
 * Read/write a variable amount from the device specific (ie, network)
 * configuration region. This region is encoded in the same endian as
 * the guest.
 */
void rpmsg_rdev_read_config(struct virtio_device *dev, uint32_t offset,
			    void *dst, int length)
{
	(void)dev;
	(void)offset;
	(void)dst;
	(void)length;

	return;
}

void rpmsg_rdev_write_config(struct virtio_device *dev, uint32_t offset,
			     void *src, int length)
{
	(void)dev;
	(void)offset;
	(void)src;
	(void)length;

	return;
}

void rpmsg_rdev_reset(struct virtio_device *dev)
{
	(void)dev;

	return;
}

