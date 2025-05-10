/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_VIRTIO_VIRTIO_H_
#define ZEPHYR_VIRTIO_VIRTIO_H_
#include <zephyr/device.h>
#include "virtqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Virtio Interface
 * @defgroup virtio_interface Virtio Interface
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
	/**
	 * Returns virtqueue at given idx
	 *
	 * @param dev virtio device it operates on
	 * @param queue_idx index of virtqueue to get
	 * @return pointer to virtqueue or NULL if not present
	 */
	struct virtq *(*get_virtqueue)(const struct device *dev, uint16_t queue_idx);
	/**
	 * Notifies queue
	 *
	 * @param dev virtio device it operates on
	 * @param queue_idx virtqueue to be notified
	 */
	void (*notify_queue)(const struct device *dev, uint16_t queue_idx);
	/**
	 * Returns device specific config
	 *
	 * @param dev virtio device it operates on
	 * @return pointer to the device specific virtqueue or NULL if its not present
	 */
	void *(*get_device_specific_config)(const struct device *dev);
	/**
	 * Returns feature bit offered by virtio device
	 *
	 * @param dev virtio device it operates on
	 * @param bit selected bit
	 * @return value of the offered feature bit
	 */
	bool (*read_device_feature_bit)(const struct device *dev, int bit);
	/**
	 * Returns feature bit selected by driver
	 *
	 * @param dev virtio device it operates on
	 * @param bit selected bit
	 * @return value of the selected bit
	 */
	bool (*read_driver_feature_bit)(const struct device *dev, int bit);
	/**
	 * Sets feature bit
	 *
	 * @param dev virtio device it operates on
	 * @param bit selected bit
	 * @param value bit value to write
	 * @return 0 on success or negative error code on failure
	 */
	int (*write_driver_feature_bit)(const struct device *dev, int bit, bool value);
	/**
	 * Commits feature bits
	 *
	 * @param dev virtio device it operates on
	 * @return 0 on success or negative error code on failure
	 */
	int (*commit_feature_bits)(const struct device *dev);
	/**
	 * Initializes virtqueues
	 *
	 * @param dev virtio device it operates on
	 * @param cb callback called for each available virtqueue
	 * @param opaque pointer to user provided data that will be passed to the callback
	 * @return 0 on success or negative error code on failure
	 */
	int (*init_virtqueues)(const struct device *dev, virtio_enumerate_queues cb, void *opaque);
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_VIRTIO_VIRTIO_H_ */
