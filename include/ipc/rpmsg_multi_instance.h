/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RPMSG_MULTIPLE_INSTANCE_H_
#define ZEPHYR_INCLUDE_RPMSG_MULTIPLE_INSTANCE_H_

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RPMsg multiple instance API
 * @defgroup rpmsg_multiple_instance_api RPMsg multiple instance APIs
 * @{
 */

#define VDEV_START_ADDR     CONFIG_RPMSG_MULTI_INSTANCE_SHM_BASE_ADDRESS
#define VDEV_SIZE           CONFIG_RPMSG_MULTI_INSTANCE_SHM_SIZE

#define SHM_START_ADDR      VDEV_START_ADDR
#define SHM_SIZE            VDEV_SIZE

#define VRING_ALIGNMENT     (4)     /**< Alignment of vring buffer. */
#define VDEV_STATUS_SIZE    (0x4)   /**< Size of status region. */

/** @brief Event callback structure.
 *
 *  It is registered during endpoint registration.
 *  This structure is packed into the endpoint configuration.
 */
struct rpmsg_mi_cb {
	/** @brief Bind was successful.
	 *
	 *  @param priv Private user data.
	 */
	void (*bound)(void *priv);

	/** @brief New packet arrived.
	 *
	 *  @param data Pointer to data buffer.
	 *  @param len Length of @a data.
	 *  @param priv Private user data.
	 */
	void (*received)(const void *data, size_t len, void *priv);
};

/** @brief Endpoint instance. */
struct rpmsg_mi_ept {

	/** Name of endpoint. */
	const char *name;

	/** RPMsg endpoint. */
	struct rpmsg_endpoint ep;

	/** Event callback structure. */
	struct rpmsg_mi_cb *cb;

	/** Private user data. */
	void *priv;

	/** Endpoint was bound. */
	volatile bool bound;

	/** Linked list node. */
	sys_snode_t node;
};

/** @brief Endpoint configuration. */
struct rpmsg_mi_ept_cfg {

	/** Name of endpoint. */
	const char *name;

	/** Event callback structure. */
	struct rpmsg_mi_cb *cb;

	/** Private user data. */
	void *priv;
};

/** @brief Struct describing the context of the RPMsg instanece. */
struct rpmsg_mi_ctx {
	const char *name;
	struct k_work_q ipm_work_q;
	struct k_work ipm_work;

	const struct device *ipm_tx_handle;
	const struct device *ipm_rx_handle;

	uint32_t shm_status_reg_addr;
	struct metal_io_region *shm_io;
	struct metal_device shm_device;
	metal_phys_addr_t shm_physmap[1];

	struct rpmsg_virtio_device rvdev;
	struct rpmsg_virtio_shm_pool shpool;
	struct rpmsg_device *rdev;

	struct virtqueue *vq[2];
	struct virtio_vring_info rvrings[2];
	struct virtio_device vdev;

	uint32_t vring_tx_addr;
	uint32_t vring_rx_addr;

	sys_slist_t endpoints;
};

/** @brief Configuration of the RPMsg instance. */
struct rpsmg_mi_ctx_cfg {

	/** Name of instance. */
	const char *name;

	/** Stack area for k_work_q. */
	k_thread_stack_t *ipm_stack_area;

	/** Size of stack area. */
	size_t ipm_stack_size;

	/** Priority of work_q. */
	int ipm_work_q_prio;

	/** Name of work_q thread. */
	const char *ipm_thread_name;

	/** Name of the TX IPM channel. */
	const char *ipm_tx_name;

	/** Name of the RX IPM channel. */
	const char *ipm_rx_name;
};

/** @brief Initialization of RPMsg instance.
 *
 *  Each instance has an automatically allocated area of shared memory.
 *
 * @param ctx Pointer to the RPMsg instance.
 * @param cfg Pointer to the configuration structure.
 * @retval 0 if the operation was successful.
 *         -EINVAL when the incorrect parameters have been passed.
 *         -EIO when the configuration is not correct.
 *         -ENODEV failed to get TX or RX IPM handle.
 *         -ENOMEM when there is not enough memory to register virqueue.
 *         < 0 on other negative errno code, reported by rpmsg.
 */
int rpmsg_mi_ctx_init(struct rpmsg_mi_ctx *ctx, const struct rpsmg_mi_ctx_cfg *cfg);

/** @brief Register IPC endpoint.
 *
 *  Registers IPC endpoint to enable communication with a remote device.
 *
 *  @param ctx Pointer to the RPMsg instance.
 *  @param ept Pointer to endpoint object.
 *  @param cfg Pointer to the endpoint configuration.
 *
 *  @retval -EINVAL One of the parameters is incorrect.
 *  @retval other errno code reported by rpmsg.
 */
int rpmsg_mi_ept_register(struct rpmsg_mi_ctx *ctx,
			  struct rpmsg_mi_ept *ept,
			  struct rpmsg_mi_ept_cfg *cfg);

/** @brief Send data using given IPC endpoint.
 *
 *  Note: It is not possible to send a message of zero length.
 *
 *  @param ept Endpoint object.
 *  @param data Pointer to the buffer to send through RPMsg.
 *  @param len Number of bytes to send.
 *
 *  @retval Number of bytes it has sent or negative error value on failure.
 */
int rpmsg_mi_send(struct rpmsg_mi_ept *ept, const void *data, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RPMSG_MULTIPLE_INNSTANCE_H_ */
