/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_SERVICE_IPC_STATIC_VRINGS_H_
#define ZEPHYR_INCLUDE_IPC_SERVICE_IPC_STATIC_VRINGS_H_

#include <zephyr/ipc/ipc_service.h>
#include <openamp/open_amp.h>
#include <metal/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC service static VRINGs API
 * @defgroup ipc_service_static_vrings_api IPC service static VRINGs API
 * @ingroup ipc
 * @{
 */

/** Number of used VRING buffers. */
#define VRING_COUNT	(2)

/**
 * Memory alignment.
 *
 * This should take into account the cache line if the cache is enabled, otherwise
 * it should be naturally aligned to the machine word size.
 */
#define MEM_ALIGNMENT	CONFIG_IPC_SERVICE_STATIC_VRINGS_MEM_ALIGNMENT

/**
 * @typedef ipc_notify_cb
 * @brief Define the notify callback.
 *
 * This callback is defined at instance level and it is called on virtqueue notify.
 *
 * @param vq Virtqueue.
 * @param priv Priv data.
 */
typedef void (*ipc_notify_cb)(struct virtqueue *vq, void *priv);

/** @brief Static VRINGs structure.
 *
 *  Struct used to represent and carry information about static allocation of VRINGs.
 */
struct ipc_static_vrings {
	/** virtIO device. */
	struct virtio_device vdev;

	/** SHM physmap. */
	metal_phys_addr_t shm_physmap[1];

	/** SHM device. */
	struct metal_device shm_device;

	/** SHM and addresses. */
	uintptr_t status_reg_addr;

	/** TX VRING address. */
	uintptr_t tx_addr;

	/** RX VRING address. */
	uintptr_t rx_addr;

	/** VRING size. */
	size_t vring_size;

	/** Shared memory region address. */
	uintptr_t shm_addr;

	/** Share memory region size. */
	size_t shm_size;

	/** SHM IO region. */
	struct metal_io_region *shm_io;

	/** VRINGs */
	struct virtio_vring_info rvrings[VRING_COUNT];

	/** Virtqueues. */
	struct virtqueue *vq[VRING_COUNT];

	/** Private data to be passed to the notify callback. */
	void *priv;

	/** Notify callback. */
	ipc_notify_cb notify_cb;
};

/** @brief Init the static VRINGs.
 *
 *  Init VRINGs and Virtqueues of an OpenAMP / RPMsg instance.
 *
 *  @param vr Pointer to the VRINGs instance struct.
 *  @param role Host / Remote role.
 *
 *  @retval -EINVAL When some parameter is missing.
 *  @retval -ENOMEM When memory is not enough for VQs allocation.
 *  @retval 0 If successful.
 *  @retval Other errno codes depending on the OpenAMP implementation.
 */
int ipc_static_vrings_init(struct ipc_static_vrings *vr, unsigned int role);

/** @brief Deinitialise the static VRINGs.
 *
 *  Deinitialise VRINGs and Virtqueues of an OpenAMP / RPMsg instance.
 *
 *  @param vr Pointer to the VRINGs instance struct.
 *  @param role Host / Remote role.
 *
 *  @retval 0 If successful.
 *  @retval Other errno codes depending on the OpenAMP implementation.
 */
int ipc_static_vrings_deinit(struct ipc_static_vrings *vr, unsigned int role);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_SERVICE_IPC_STATIC_VRINGS_H_ */
