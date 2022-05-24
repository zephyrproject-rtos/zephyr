/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_SERVICE_IPC_RPMSG_H_
#define ZEPHYR_INCLUDE_IPC_SERVICE_IPC_RPMSG_H_

#include <zephyr/ipc/ipc_service.h>
#include <openamp/open_amp.h>
#include <metal/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC service RPMsg API
 * @defgroup ipc_service_rpmsg_api IPC service RPMsg API
 * @{
 */

/** Number of endpoints. */
#define NUM_ENDPOINTS	CONFIG_IPC_SERVICE_BACKEND_RPMSG_NUM_ENDPOINTS_PER_INSTANCE

struct ipc_rpmsg_ept;

/**
 * @typedef rpmsg_ept_bound_cb
 * @brief Define the bound callback.
 *
 * This callback is defined at instance level and it is called when an endpoint
 * of the instance is bound.
 *
 * @param ept Endpoint of the instance just bound.
 */
typedef void (*rpmsg_ept_bound_cb)(struct ipc_rpmsg_ept *ept);

/** @brief Endpoint structure.
 *
 *  Used to define an endpoint to be encapsulated in an RPMsg instance.
 */
struct ipc_rpmsg_ept {
	/** RPMsg endpoint. */
	struct rpmsg_endpoint ep;

	/** Name of the endpoint. */
	char name[RPMSG_NAME_SIZE];

	/** Destination endpoint. */
	uint32_t dest;

	/** Bound flag. */
	volatile bool bound;

	/** Callbacks. */
	const struct ipc_service_cb *cb;

	/** Private data to be passed to the endpoint callbacks. */
	void *priv;
};

/** @brief RPMsg instance structure.
 *
 *  Struct representation of an RPMsg instance.
 */
struct ipc_rpmsg_instance {
	/** Endpoints in the instance. */
	struct ipc_rpmsg_ept endpoint[NUM_ENDPOINTS];

	/** RPMsg virtIO device. */
	struct rpmsg_virtio_device rvdev;

	/** SHM pool. */
	struct rpmsg_virtio_shm_pool shm_pool;

	/** EPT (instance) bound callback. */
	rpmsg_ept_bound_cb bound_cb;

	/** EPT (instance) callback. */
	rpmsg_ept_cb cb;

	/** Mutex for the instance. */
	struct k_mutex mtx;
};

/** @brief Init an RPMsg instance.
 *
 *  Init an RPMsg instance.
 *
 *  @param instance Pointer to the RPMsg instance struct.
 *  @param role Host / Remote role.
 *  @param buffer_size Size of the buffer used to send data between host and remote.
 *  @param shm_io SHM IO region pointer.
 *  @param vdev VirtIO device pointer.
 *  @param shb Shared memory region pointer.
 *  @param size Size of the shared memory region.
 *  @param ns_bind_cb callback handler for name service announcement without
 *		      local endpoints waiting to bind. If NULL the
 *		      implementation falls back to the internal implementation.
 *
 *  @retval -EINVAL When some parameter is missing.
 *  @retval 0 If successful.
 *  @retval Other errno codes depending on the OpenAMP implementation.
 */
int ipc_rpmsg_init(struct ipc_rpmsg_instance *instance,
		   unsigned int role,
		   unsigned int buffer_size,
		   struct metal_io_region *shm_io,
		   struct virtio_device *vdev,
		   void *shb, size_t size,
		   rpmsg_ns_bind_cb ns_bind_cb);

/** @brief Register an endpoint.
 *
 *  Register an endpoint to a provided RPMsg instance.
 *
 *  @param instance Pointer to the RPMsg instance struct.
 *  @param role Host / Remote role.
 *  @param ept Endpoint to register.
 *
 *  @retval -EINVAL When some parameter is missing.
 *  @retval 0 If successful.
 *  @retval Other errno codes depending on the OpenAMP implementation.
 */
int ipc_rpmsg_register_ept(struct ipc_rpmsg_instance *instance, unsigned int role,
			   struct ipc_rpmsg_ept *ept);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_SERVICE_IPC_RPMSG_H_ */
