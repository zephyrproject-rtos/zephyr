/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VHOST_H_
#define ZEPHYR_DRIVERS_VHOST_H_

/**
 * @brief VHost API
 *
 * The VHost provides functions for VIRTIO device backends in
 * a hypervisor environment.
 * VHost backends handle guest VIRTIO requests and respond to them.
 *
 * This is the VHost device class for VIRTIO backends. It is the boundary
 * between transport or hypervisor-specific code and a VIRTIO device backend.
 * It deliberately does not prescribe buffer allocation, locking, callback
 * dispatch, or request scheduling.
 * Implementations may process requests in parallel when they preserve the
 * lifetime and completion rules described by each operation below.
 *
 * A buffer returned by vhost_prepare_iovec() remains valid until the matching
 * vhost_release_iovec() has completed. A queue reset invalidates outstanding
 * queue references; users must stop accessing them before reset reclamation
 * starts. Callback execution context and queue-to-queue concurrency are
 * controller-specific.
 *
 * This class does not require the vringh helper. A backend may use vringh or
 * another ring-processing implementation while using this controller API for
 * transport-specific operations.
 *
 * @defgroup vhost_apis VHost Controller APIs
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/kernel.h>

/**
 * Represents a memory buffer segment for VHost operations.
 *
 * The address is valid only for the lifetime of the mapping returned by
 * vhost_prepare_iovec(). The controller may map or copy the guest buffer;
 * this type does not require either strategy.
 */
struct vhost_iovec {
	void *iov_base;
	size_t iov_len;
};

/**
 * Represents a guest physical address and length pair for VHost operations.
 */
struct vhost_buf {
	uint64_t gpa;    /**< Guest physical address. */
	size_t len;      /**< Buffer length in bytes. */
	bool is_write;   /**< True when the backend writes to the guest buffer. */
};

/**
 * VHost controller API structure.
 *
 * A controller owns transport-specific state and may support multiple
 * outstanding requests. Operations referring to different queue/head pairs
 * may be executed concurrently when the controller supports it. Callers must
 * provide any additional synchronization required for concurrent operations
 * on the same queue.
 */
__subsystem struct vhost_controller_api {
	int (*prepare_iovec)(const struct device *dev, uint16_t queue_id, uint16_t head,
			     const struct vhost_buf *bufs, size_t bufs_count,
			     struct vhost_iovec *read_iovec, size_t max_read_iovecs,
			     struct vhost_iovec *write_iovec, size_t max_write_iovecs,
			     size_t *read_count, size_t *write_count);
	int (*release_iovec)(const struct device *dev, uint16_t queue_id, uint16_t head);
	int (*get_virtq)(const struct device *dev, uint16_t queue_id, void **parts,
			 size_t *queue_size);
	int (*get_driver_features)(const struct device *dev, uint64_t *drv_feats);
	bool (*virtq_is_ready)(const struct device *dev, uint16_t queue_id);
	int (*register_virtq_ready_cb)(const struct device *dev,
				       void (*callback)(const struct device *dev, uint16_t queue_id,
							void *data),
				       void *data);
	int (*register_virtq_notify_cb)(const struct device *dev, uint16_t queue_id,
					void (*callback)(const struct device *dev,
							 uint16_t queue_id, void *data),
					void *data);
	int (*notify_virtq)(const struct device *dev, uint16_t queue_id);
	int (*set_device_status)(const struct device *dev, uint32_t status);
};

/**
 * @brief Prepare iovecs for virtq process
 *
 * Maps guest physical addresses to host virtual addresses for the given
 * GPAs and fills the provided read and write iovec arrays.
 *
 * @param dev              VHost device
 * @param queue_id         Queue identifier
 * @param slot_id          Descriptor head identifying the request
 * @param bufs             Array of GPA/length pairs
 * @param bufs_count       Number of bufs in the array
 * @param read_iovec       Array to fill with read iovecs
 * @param max_read_iovecs  Maximum number of read iovecs that can be stored
 * @param write_iovec      Array to fill with write iovecs
 * @param max_write_iovecs Maximum number of write iovecs that can be stored
 * @param read_count       Number of read iovecs prepared
 * @param write_count      Number of write iovecs prepared
 *
 * @retval 0             Success
 * @retval -EINVAL       Invalid parameters
 * @retval -ENOMEM       Insufficient memory
 * @retval -E2BIG        Buffer too large (in other word, iovecs are too small)
 *
 * @note On success, the returned iovecs and their backing mappings remain
 *       valid until vhost_release_iovec() is called for the same queue and
 *       descriptor head. The caller must release the mapping exactly once
 *       after the last access. On failure, the controller releases any
 *       partial mapping it created.
 */
static inline int vhost_prepare_iovec(const struct device *dev, uint16_t queue_id, uint16_t slot_id,
				      const struct vhost_buf *bufs, size_t bufs_count,
				      struct vhost_iovec *read_iovec, size_t max_read_iovecs,
				      struct vhost_iovec *write_iovec, size_t max_write_iovecs,
				      size_t *read_count, size_t *write_count)
{
	const struct vhost_controller_api *api = dev->api;

	return api->prepare_iovec(dev, queue_id, slot_id, bufs, bufs_count, read_iovec,
				  max_read_iovecs, write_iovec, max_write_iovecs, read_count,
				  write_count);
}

/**
 * @brief Release all iovecs
 *
 * Release iovecs that were prepared by vhost_prepare_iovec.
 *
 * The caller must not access the corresponding iovecs after this function
 * returns. A release is associated with one successful prepare operation and
 * must not be repeated for the same queue and descriptor head.
 *
 * @param dev       VHost controller device
 * @param queue_id  Queue ID
 * @param slot_id   Descriptor head identifying the request.
 *
 * @retval 0        Success
 * @retval -EINVAL  Invalid parameters
 */
static inline int vhost_release_iovec(const struct device *dev, uint16_t queue_id, uint16_t slot_id)
{
	const struct vhost_controller_api *api = dev->api;

	return api->release_iovec(dev, queue_id, slot_id);
}

/**
 * @brief Get VirtQueue components
 *
 * @param dev         VHost controller device
 * @param queue_id    Queue ID
 * @param parts       Array for descriptor, available, used ring pointers
 * @param queue_size  Queue size output
 *
 * @retval 0          Success
 * @retval -EINVAL    Invalid parameters
 * @retval -ENODEV    Queue not ready
 *
 * @note The returned ring pointers remain valid only while the queue remains
 *       configured. A queue reset or reconfiguration invalidates them.
 */
static inline int vhost_get_virtq(const struct device *dev, uint16_t queue_id, void **parts,
				  size_t *queue_size)
{
	const struct vhost_controller_api *api = dev->api;

	return api->get_virtq(dev, queue_id, parts, queue_size);
}

/**
 * @brief Get negotiated VirtIO feature bits
 *
 * @param dev         VHost controller device
 * @param drv_feats   Output for feature mask
 *
 * @retval 0          Success
 * @retval -EINVAL    Invalid parameters
 */
static inline int vhost_get_driver_features(const struct device *dev, uint64_t *drv_feats)
{
	const struct vhost_controller_api *api = dev->api;

	return api->get_driver_features(dev, drv_feats);
}

/**
 * @brief Check if queue is ready for processing
 *
 * @param dev       VHost controller device
 * @param queue_id  Queue ID (0-based)
 *
 * @retval true     Queue is ready
 * @retval false    Queue not ready or invalid
 */
static inline bool vhost_queue_ready(const struct device *dev, uint16_t queue_id)
{
	const struct vhost_controller_api *api = dev->api;

	return api->virtq_is_ready(dev, queue_id);
}

/**
 * @brief Register device-wide queue ready callback
 *
 * The controller may unregister this callback when the device or a queue is
 * reset. The callback can be delivered asynchronously and may be coalesced;
 * its execution context is controller-specific.
 *
 * @param dev         VHost controller device
 * @param callback    Function to call when any queue becomes ready
 * @param user_data   User data for callback
 *
 * @retval 0          Success
 * @retval -EINVAL    Invalid parameters
 *
 * @note The callback must not retain queue pointers across reset. The API does
 *       not require a particular worker, interrupt, or thread context.
 */
static inline int vhost_register_virtq_ready_cb(const struct device *dev,
						void (*callback)(const struct device *dev,
								 uint16_t queue_id, void *data),
						void *user_data)
{
	const struct vhost_controller_api *api = dev->api;

	return api->register_virtq_ready_cb(dev, callback, user_data);
}

/**
 * @brief Register per-queue guest notification callback
 *
 * The controller may unregister this callback when the queue is reset. The
 * callback can be delivered asynchronously and may be coalesced; its
 * execution context is controller-specific.
 *
 * @param dev         VHost controller device
 * @param queue_id    Queue ID (0-based)
 * @param callback    Function to call on guest notifications
 * @param user_data   User data for callback
 *
 * @retval 0          Success
 * @retval -EINVAL    Invalid parameters
 * @retval -ENODEV    Queue not found
 *
 * @note The callback must not retain queue pointers across reset. The API does
 *       not require a particular worker, interrupt, or thread context.
 */
static inline int vhost_register_virtq_notify_cb(const struct device *dev, uint16_t queue_id,
						 void (*callback)(const struct device *dev,
								  uint16_t queue_id, void *data),
						 void *user_data)
{
	const struct vhost_controller_api *api = dev->api;

	return api->register_virtq_notify_cb(dev, queue_id, callback, user_data);
}

/**
 * @brief Send interrupt notification to guest
 *
 * @param dev        VHost controller device
 * @param queue_id   Queue ID (0-based)
 *
 * @retval 0         Success
 * @retval -EINVAL   Invalid parameters
 * @retval -ENODEV   Queue not ready
 * @retval -EIO      Interrupt delivery failed
 */
static inline int vhost_notify_virtq(const struct device *dev, uint16_t queue_id)
{
	const struct vhost_controller_api *api = dev->api;

	return api->notify_virtq(dev, queue_id);
}

/**
 * @brief Set device status and notify guest
 *
 * @param dev         VHost controller device
 * @param status      VirtIO device status bits to set
 *
 * @retval 0          Success
 * @retval -EINVAL    Invalid parameters
 * @retval -EIO       Notification failed
 */
static inline int vhost_set_device_status(const struct device *dev, uint32_t status)
{
	const struct vhost_controller_api *api = dev->api;

	return api->set_device_status(dev, status);
}

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_VHOST_H_ */
