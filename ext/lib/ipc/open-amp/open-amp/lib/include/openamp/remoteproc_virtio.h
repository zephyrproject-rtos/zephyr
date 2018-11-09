/*
 * Remoteproc Virtio Framework
 *
 * Copyright(c) 2018 Xilinx Ltd.
 * Copyright(c) 2011 Texas Instruments, Inc.
 * Copyright(c) 2011 Google, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name Texas Instruments nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef REMOTEPROC_VIRTIO_H
#define REMOTEPROC_VIRTIO_H

#include <metal/io.h>
#include <metal/list.h>
#include <openamp/virtio.h>

#if defined __cplusplus
extern "C" {
#endif

/* define vdev notification funciton user should implement */
typedef int (*rpvdev_notify_func)(void *priv, uint32_t id);

/**
 * struct remoteproc_virtio
 * @priv pointer to private data
 * @notifyid notification id
 * @vdev_rsc address of vdev resource
 * @vdev_rsc_io metal I/O region of vdev_info, can be NULL
 * @notify notification function
 * @vdev virtio device
 * @node list node
 */
struct remoteproc_virtio {
	void *priv;
	uint32_t notify_id;
	void *vdev_rsc;
	struct metal_io_region *vdev_rsc_io;
	rpvdev_notify_func notify;
	struct virtio_device vdev;
	struct metal_list node;
};

/**
 * rproc_virtio_create_vdev
 *
 * Create rproc virtio vdev
 *
 * @role: 0 - virtio master, 1 - virtio slave
 * @notifyid: virtio device notification id
 * @rsc: pointer to the virtio device resource
 * @rsc_io: pointer to the virtio device resource I/O region
 * @priv: pointer to the private data
 * @notify: vdev and virtqueue notification function
 * @rst_cb: reset virtio device callback
 *
 * return pointer to the created virtio device for success,
 * NULL for failure.
 */
struct virtio_device *
rproc_virtio_create_vdev(unsigned int role, unsigned int notifyid,
			 void *rsc, struct metal_io_region *rsc_io,
			 void *priv,
			 rpvdev_notify_func notify,
			 virtio_dev_reset_cb rst_cb);

/**
 * rproc_virtio_remove_vdev
 *
 * Create rproc virtio vdev
 *
 * @vdev - pointer to the virtio device
 */
void rproc_virtio_remove_vdev(struct virtio_device *vdev);

/**
 * rproc_virtio_create_vring
 *
 * Create rproc virtio vring
 *
 * @vdev: pointer to the virtio device
 * @index: vring index in the virtio device
 * @notifyid: remoteproc vring notification id
 * @va: vring virtual address
 * @io: pointer to vring I/O region
 * @num_desc: number of descriptors
 * @align: vring alignment
 *
 * return 0 for success, negative value for failure.
 */
int rproc_virtio_init_vring(struct virtio_device *vdev, unsigned int index,
			    unsigned int notifyid, void *va,
			    struct metal_io_region *io,
			    unsigned int num_descs, unsigned int align);

/**
 * rproc_virtio_notified
 *
 * remoteproc virtio is got notified
 *
 * @vdev - pointer to the virtio device
 * @notifyid - notify id
 *
 * return 0 for successful, negative value for failure
 */
int rproc_virtio_notified(struct virtio_device *vdev, uint32_t notifyid);

/**
 * rproc_virtio_wait_remote_ready
 *
 * Blocking function, waiting for the remote core is ready to start
 * communications.
 *
 * @vdev - pointer to the virtio device
 *
 * return true when remote processor is ready.
 */
void rproc_virtio_wait_remote_ready(struct virtio_device *vdev);

#if defined __cplusplus
}
#endif

#endif /* REMOTEPROC_VIRTIO_H */
