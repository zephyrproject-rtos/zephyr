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
 * @def_driverbackendgroup{VIRTIO,virtio_interface}
 * @{
 */

/**
 * @brief Get virtqueue at given index.
 * See virtio_get_virtqueue() for argument description.
 */
typedef struct virtq *(*virtio_api_get_virtqueue_t)(const struct device *dev, uint16_t queue_idx);

/**
 * @brief Notify virtqueue.
 * See virtio_notify_virtqueue() for argument description.
 */
typedef void (*virtio_api_notify_virtqueue_t)(const struct device *dev, uint16_t queue_idx);

/**
 * @brief Get device specific config.
 * See virtio_get_device_specific_config() for argument description.
 */
typedef void *(*virtio_api_get_device_specific_config_t)(const struct device *dev);

/**
 * @brief Read feature bit offered by the virtio device.
 * See virtio_read_device_feature_bit() for argument description.
 */
typedef bool (*virtio_api_read_device_feature_bit_t)(const struct device *dev, int bit);

/**
 * @brief Set driver feature bit.
 * See virtio_write_driver_feature_bit() for argument description.
 */
typedef int (*virtio_api_write_driver_feature_bit_t)(const struct device *dev, int bit,
						     bool value);

/**
 * @brief Commit feature bits.
 * See virtio_commit_feature_bits() for argument description.
 */
typedef int (*virtio_api_commit_feature_bits_t)(const struct device *dev);

/**
 * @brief Initialize virtqueues.
 * See virtio_init_virtqueues() for argument description.
 */
typedef int (*virtio_api_init_virtqueues_t)(const struct device *dev, uint16_t num_queues,
					    virtio_enumerate_queues cb, void *opaque);

/**
 * @brief Finalize initialization of the virtio device.
 * See virtio_finalize_init() for argument description.
 */
typedef void (*virtio_api_finalize_init_t)(const struct device *dev);

/**
 * @driver_ops{VIRTIO}
 */
__subsystem struct virtio_driver_api {
	/** @driver_ops_mandatory @copybrief virtio_get_virtqueue */
	virtio_api_get_virtqueue_t get_virtqueue;
	/** @driver_ops_mandatory @copybrief virtio_notify_virtqueue */
	virtio_api_notify_virtqueue_t notify_virtqueue;
	/** @driver_ops_mandatory @copybrief virtio_get_device_specific_config */
	virtio_api_get_device_specific_config_t get_device_specific_config;
	/** @driver_ops_mandatory @copybrief virtio_read_device_feature_bit */
	virtio_api_read_device_feature_bit_t read_device_feature_bit;
	/** @driver_ops_mandatory @copybrief virtio_write_driver_feature_bit */
	virtio_api_write_driver_feature_bit_t write_driver_feature_bit;
	/** @driver_ops_mandatory @copybrief virtio_commit_feature_bits */
	virtio_api_commit_feature_bits_t commit_feature_bits;
	/** @driver_ops_mandatory @copybrief virtio_init_virtqueues */
	virtio_api_init_virtqueues_t init_virtqueues;
	/** @driver_ops_mandatory @copybrief virtio_finalize_init */
	virtio_api_finalize_init_t finalize_init;
};

/**
 * @}
 */

/**
 * Returns virtqueue at given idx
 *
 * @param dev virtio device it operates on
 * @param queue_idx index of virtqueue to get
 * @return pointer to virtqueue or NULL if not present
 */
static inline struct virtq *virtio_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	return DEVICE_API_GET(virtio, dev)->get_virtqueue(dev, queue_idx);
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
	DEVICE_API_GET(virtio, dev)->notify_virtqueue(dev, queue_idx);
}

/**
 * Returns device specific config
 *
 * @param dev virtio device it operates on
 * @return pointer to the device specific config or NULL if its not present
 */
static inline void *virtio_get_device_specific_config(const struct device *dev)
{
	return DEVICE_API_GET(virtio, dev)->get_device_specific_config(dev);
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
	return DEVICE_API_GET(virtio, dev)->read_device_feature_bit(dev, bit);
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
	return DEVICE_API_GET(virtio, dev)->write_driver_feature_bit(dev, bit, value);
}

/**
 * Commits feature bits
 *
 * @param dev virtio device it operates on
 * @return 0 on success or negative error code on failure
 */
static inline int virtio_commit_feature_bits(const struct device *dev)
{
	return DEVICE_API_GET(virtio, dev)->commit_feature_bits(dev);
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
	return DEVICE_API_GET(virtio, dev)->init_virtqueues(dev, num_queues, cb, opaque);
}

/**
 * Finalizes initialization of the virtio device
 *
 * @param dev virtio device it operates on
 */
static inline void virtio_finalize_init(const struct device *dev)
{
	DEVICE_API_GET(virtio, dev)->finalize_init(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_VIRTIO_VIRTIO_H_ */
