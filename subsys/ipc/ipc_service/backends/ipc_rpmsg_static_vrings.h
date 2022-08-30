/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openamp/virtio_ring.h>
#include <openamp/rpmsg_virtio.h>

#include <zephyr/ipc/ipc_static_vrings.h>

/*
 * Endpoint registration flow:
 *
 *	>>> Case #1: Endpoint registered on HOST first <<<
 *
 *  [B] backend
 *  [O] OpenAMP
 *
 * REMOTE				HOST
 * -----------------------------------------------------------------
 *					[B] register_ept **
 * [B] register_ept **
 * [B] ipc_rpmsg_register_ept
 * [B] rpmsg_create_ept
 * [O] rpmsg_send_ns_message
 * [O] virtqueue_kick
 * [O] virtio_notify_cb
 * [B] mbox_send
 *					[B] mbox_callback
 *					[B] mbox_callback_process
 *					[B] virtqueue_notification
 *					[O] rpmsg_virtio_rx_callback
 *					[B] ns_bind_cb
 *					[B] rpmsg_create_ept
 *					[B] bound_cb
 *					[B] rpmsg_send
 *					[B] virtio_notify_cb
 *					[B] mbox_send
 * [B] mbox_callback
 * [B] mbox_callback_process
 * [B] virtqueue_notification
 * [O] rpmsg_virtio_rx_callback
 * [O] ept_cb
 * [B] bound_cb
 *
 *	>>> Case #2: Endpoint registered on REMOTE first <<<
 *
 *  [B] backend
 *  [O] OpenAMP
 *
 * REMOTE				HOST
 * -----------------------------------------------------------------
 * [B] register_ept **
 * [B] ipc_rpmsg_register_ept
 * [B] rpmsg_create_ept
 * [O] rpmsg_send_ns_message
 * [O] virtqueue_kick
 * [O] virtio_notify_cb
 * [O] mbox_send
 *					[B] mbox_callback
 *					[B] mbox_callback_process
 *					[B] virtqueue_notification
 *					[O] rpmsg_virtio_rx_callback
 *					[B] ns_bind_cb
 *
 *					[B] register_ept **
 *					[B] rpmsg_create_ept
 *					[B] bound_cb
 *					[B] rpmsg_send
 *					[B] virtio_notify_cb
 *					[B] mbox_send
 * [B] mbox_callback
 * [B] mbox_callback_process
 * [B] virtqueue_notification
 * [O] rpmsg_virtio_rx_callback
 * [O] ept_cb
 * [B] bound_cb
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Endpoint registration flow (with focus on backend):
 *
 *	>>> Case #1: Endpoint registered on HOST first <<<
 *
 * REMOTE				HOST
 * -----------------------------------------------------------------
 *					register_ept()
 *					register_ept_on_host()
 *					get_ept() returns NULL
 *					name is cached in rpmsg_ept->name
 * register_ept()
 * register_ept_on_remote()
 * ipc_rpmsg_register_ept()
 *					ns_bind_cb()
 *					get_ept() returns endpoint with cached name
 *					advertise_ept()
 *					rpmsg_create_ept()
 *					bound_cb()
 *					rpmsg_send()
 * mbox_callback()
 * mbox_callback_process()
 * virtqueue_notification()
 * ept_cb()
 * bound_cb()
 *
 *	>>> Case #2: Endpoint registered on REMOTE first <<<
 *
 * REMOTE				HOST
 * -----------------------------------------------------------------
 * register_ept()
 * register_ept_on_remote()
 * ipc_rpmsg_register_ept()
 *					ns_bind_cb()
 *					get_ept() return NULL
 *					name is cached in rpmsg_ept->name
 *					...
 *					register_ept()
 *					register_ept_on_host()
 *					get_ept() returns endpoint with cached name
 *					advertise_ept()
 *					rpmsg_create_ept()
 *					bound_cb()
 *					rpmsg-send()
 * mbox_callback()
 * mbox_callback_process()
 * virtqueue_notification()
 * ept_cb()
 * bound_cb()
 *
 */

#define VDEV_STATUS_SIZE	(4) /* Size of status region */

#define VIRTQUEUE_ID_HOST	(0)
#define VIRTQUEUE_ID_REMOTE	(1)

#define ROLE_HOST		VIRTIO_DEV_DRIVER
#define ROLE_REMOTE		VIRTIO_DEV_DEVICE

static inline size_t vq_ring_size(unsigned int num, unsigned int buf_size)
{
	return (buf_size * num);
}

static inline size_t shm_size(unsigned int num, unsigned int buf_size)
{
	return (VDEV_STATUS_SIZE + (VRING_COUNT * vq_ring_size(num, buf_size)) +
	       (VRING_COUNT * vring_size(num, VRING_ALIGNMENT)));
}

static inline unsigned int optimal_num_desc(size_t shm_size, unsigned int buf_size)
{
	size_t available, single_alloc;
	unsigned int num_desc;

	available = shm_size - VDEV_STATUS_SIZE;
	single_alloc = VRING_COUNT * (vq_ring_size(1, buf_size) + vring_size(1, VRING_ALIGNMENT));

	num_desc = (unsigned int) (available / single_alloc);

	return (1 << (find_msb_set(num_desc) - 1));
}
