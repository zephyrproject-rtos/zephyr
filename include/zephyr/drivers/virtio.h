/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup virtio_interface
 * @brief Main header file for Virtio driver API.
 */

#ifndef ZEPHYR_VIRTIO_VIRTIO_H_
#define ZEPHYR_VIRTIO_VIRTIO_H_
#include <zephyr/device.h>
#include "virtio/virtqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Virtual I/O (VIRTIO) devices.
 * @defgroup virtio_interface VIRTIO
 * @ingroup io_interfaces
 * @{
 */

/**
 * Callback used during virtqueue enumeration
 *
 * @param queue_idx index of currently inspected queue
 * @param max_queue_size maximum permitted size of currently inspected queue
 * @param opaque pointer to user provided data
 * @return the size of currently inspected virtqueue we want to set
 */
typedef uint16_t (*virtio_enumerate_queues)(
	uint16_t queue_idx, uint16_t max_queue_size, void *opaque
);

/**
 * @brief Virtio api structure
 */
__subsystem struct virtio_driver_api {
	struct virtq *(*get_virtqueue)(const struct device *dev, uint16_t queue_idx);
	void (*notify_virtqueue)(const struct device *dev, uint16_t queue_idx);
	void *(*get_device_specific_config)(const struct device *dev);
	bool (*read_device_feature_bit)(const struct device *dev, int bit);
	int (*write_driver_feature_bit)(const struct device *dev, int bit, bool value);
	int (*commit_feature_bits)(const struct device *dev);
	int (*init_virtqueues)(
		const struct device *dev, uint16_t num_queues, virtio_enumerate_queues cb,
		void *opaque
	);
	void (*finalize_init)(const struct device *dev);
};

/**
 * Returns virtqueue at given idx
 *
 * @param dev virtio device it operates on
 * @param queue_idx index of virtqueue to get
 * @return pointer to virtqueue or NULL if not present
 */
static inline struct virtq *virtio_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	const struct virtio_driver_api *api = dev->api;

	return api->get_virtqueue(dev, queue_idx);
}

/**
 * Notifies virtqueue
 *
 * Note that according to spec 2.7.13.3 the device may access the buffers as soon
 * as the avail->idx is increased, which is done by virtq_add_buffer_chain, so the
 * device may access the buffers even without notifying it with virtio_notify_virtqueue
 *
 * @param dev virtio device it operates on
 * @param queue_idx virtqueue to be notified
 */
static inline void virtio_notify_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	const struct virtio_driver_api *api = dev->api;

	api->notify_virtqueue(dev, queue_idx);
}

/**
 * Returns device specific config
 *
 * @param dev virtio device it operates on
 * @return pointer to the device specific config or NULL if its not present
 */
static inline void *virtio_get_device_specific_config(const struct device *dev)
{
	const struct virtio_driver_api *api = dev->api;

	return api->get_device_specific_config(dev);
}

/**
 * Returns feature bit offered by virtio device
 *
 * @param dev virtio device it operates on
 * @param bit selected bit
 * @return value of the offered feature bit
 */
static inline bool virtio_read_device_feature_bit(const struct device *dev, int bit)
{
	const struct virtio_driver_api *api = dev->api;

	return api->read_device_feature_bit(dev, bit);
}

/**
 * Sets feature bit
 *
 * @param dev virtio device it operates on
 * @param bit selected bit
 * @param value bit value to write
 * @return 0 on success or negative error code on failure
 */
static inline int virtio_write_driver_feature_bit(const struct device *dev, int bit, bool value)
{
	const struct virtio_driver_api *api = dev->api;

	return api->write_driver_feature_bit(dev, bit, value);
}

/**
 * Commits feature bits
 *
 * @param dev virtio device it operates on
 * @return 0 on success or negative error code on failure
 */
static inline int virtio_commit_feature_bits(const struct device *dev)
{
	const struct virtio_driver_api *api = dev->api;

	return api->commit_feature_bits(dev);
}

/**
 * Initializes virtqueues
 *
 * @param dev virtio device it operates on
 * @param num_queues number of queues to initialize
 * @param cb callback called for each available virtqueue
 * @param opaque pointer to user provided data that will be passed to the callback
 * @return 0 on success or negative error code on failure
 */
static inline int virtio_init_virtqueues(
	const struct device *dev, uint16_t num_queues, virtio_enumerate_queues cb, void *opaque)
{
	const struct virtio_driver_api *api = dev->api;

	return api->init_virtqueues(dev, num_queues, cb, opaque);
}

/**
 * Finalizes initialization of the virtio device
 *
 * @param dev virtio device it operates on
 */
static inline void virtio_finalize_init(const struct device *dev)
{
	const struct virtio_driver_api *api = dev->api;

	api->finalize_init(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_VIRTIO_VIRTIO_H_ */
