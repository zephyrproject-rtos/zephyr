/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rpmsg_lite.h>
#include <rpmsg_ns.h>

#define VIRTIO_DEV_DRIVER 0UL
#define VIRTIO_DEV_DEVICE 1UL

#define ROLE_HOST   VIRTIO_DEV_DRIVER
#define ROLE_REMOTE VIRTIO_DEV_DEVICE

/** Number of endpoints. */
#define NUM_ENDPOINTS CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NUM_ENDPOINTS_PER_INSTANCE

#define RPMSG_NAME_SIZE (32)

struct ipc_rpmsg_lite_ept;

/**
 * @typedef rpmsg_lite_ept_bound_cb
 * @brief Define the bound callback.
 *
 * This callback is defined at instance level and it is called when an endpoint
 * of the instance is bound.
 *
 * @param ept Endpoint of the instance just bound.
 */
typedef void (*rpmsg_lite_ept_bound_cb)(struct ipc_rpmsg_lite_ept *ept);

/** @brief Private Endpoint structure.
 *
 * Used to hold Endpoint Private data.
 */
struct ipc_rpmsg_lite_ept_priv {
	/** Private data to be passed to the endpoint callbacks. */
	void *priv;

	/** Private data to reference associated endpoint instance. */
	void *priv_inst_ref;
};

/** @brief Endpoint structure.
 *
 *  Struct representation of an IPC RPMSG-Lite endpoint.
 */
struct ipc_rpmsg_lite_ept {
	/** RPMSG-Lite endpoint. */
	struct rpmsg_lite_endpoint *ep;

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
	/** RPMSG-Lite endpoint context */
	struct rpmsg_lite_ept_static_context ep_context;
#endif

	/** RPMSG-Lite endpoint private data. */
	void *ep_priv;

	/** Name of the endpoint. */
	char name[RPMSG_NAME_SIZE];

	/** Destination endpoint. */
	uint32_t dest;

	/** Bound flag. */
	volatile bool bound;

	/** Callbacks. */
	const struct ipc_service_cb *cb;

	/** Private data to be passed to the endpoint callbacks. */
	struct ipc_rpmsg_lite_ept_priv priv_data;

	/** Flag: should we hold the last buffer? */
	atomic_t hold_last_buffer;

	/** Pointer to last received buffer */
	void *last_rx_data;
};

/** @brief RPMSG-Lite instance structure.
 *
 *  Struct representation of an IPC RPMSG-Lite instance.
 */
struct ipc_rpmsg_lite_instance {
	/** RPMSG-Lite Instance. */
	struct rpmsg_lite_instance *rpmsg_lite_inst;

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
	/** RPMSG-Lite Static Context. */
	struct rpmsg_lite_instance rpmsg_lite_context;
#endif

	/** RPMSG-Lite NameService Handle. */
	rpmsg_ns_handle ns_handle;

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
	/** RPMSG-Lite Static Context for NameService (only needed for static API). */
	rpmsg_ns_static_context rpmsg_lite_ns_context;
#endif

	/** Endpoints in the instance. */
	struct ipc_rpmsg_lite_ept endpoint[NUM_ENDPOINTS];

	/** EPT (instance) bound callback. */
	rpmsg_lite_ept_bound_cb bound_cb;

	/** EPT (instance) callback. */
	rl_ept_rx_cb_t cb;

	/** Mutex for the instance. */
	struct k_mutex mtx;
};
