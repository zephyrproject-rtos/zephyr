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

/*
 * Size of the status region (possibly a multiple of the cache line size).
 */
#define VDEV_STATUS_SIZE	CONFIG_IPC_SERVICE_STATIC_VRINGS_MEM_ALIGNMENT

#define VIRTQUEUE_ID_HOST	(0)
#define VIRTQUEUE_ID_REMOTE	(1)

#define ROLE_HOST		VIRTIO_DEV_DRIVER
#define ROLE_REMOTE		VIRTIO_DEV_DEVICE

static inline size_t vq_ring_size(unsigned int num, unsigned int buf_size)
{
	return ROUND_UP((buf_size * num), MEM_ALIGNMENT);
}

static inline size_t shm_size(unsigned int num, unsigned int buf_size)
{
	return (VRING_COUNT * (vq_ring_size(num, buf_size) +
		ROUND_UP(vring_size(num, MEM_ALIGNMENT), MEM_ALIGNMENT)));
}

static inline unsigned int optimal_num_desc(size_t mem_size, unsigned int buf_size)
{
	size_t available;
	unsigned int num_desc = 1;

	available = mem_size - VDEV_STATUS_SIZE;

	while (available > shm_size(num_desc, buf_size)) {
		num_desc++;
	}

	/* if num_desc == 1 there is not enough memory */
	return (--num_desc == 0) ? 0 : (1 << LOG2(num_desc));
}
