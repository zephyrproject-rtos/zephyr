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
 * @defgroup vhost_apis VHost Controller APIs
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/kernel.h>

/**
 * Represents a memory buffer segment for VHost operations.
 */
struct vhost_iovec {
	void *iov_base;
	size_t iov_len;
};

/**
 * Represents a guest physical address and length pair for VHost operations.
 */
struct vhost_buf {
	uint64_t gpa;
	size_t len;
	bool is_write;
};

/**
 * VHost controller API structure
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
 * @param slot_id          Slot identifier
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
 * Release iovecs that prepared by host_prepare_iovec.
 *
 * @param dev       VHost controller device
 * @param queue_id  Queue ID
 * @param slot_id   Slot ID.
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
 * This callback will unregister on device reset.
 *
 * @param dev         VHost controller device
 * @param callback    Function to call when any queue becomes ready
 * @param user_data   User data for callback
 *
 * @retval 0          Success
 * @retval -EINVAL    Invalid parameters
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
 * This callback will unregister on queue reset.
 *
 * @param dev         VHost controller device
 * @param queue_id    Queue ID (0-based)
 * @param callback    Function to call on guest notifications
 * @param user_data   User data for callback
 *
 * @retval 0          Success
 * @retval -EINVAL    Invalid parameters
 * @retval -ENODEV    Queue not found
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
