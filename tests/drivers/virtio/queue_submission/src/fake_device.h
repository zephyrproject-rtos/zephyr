/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_DRIVERS_VIRTIO_QUEUE_SUBMISSION_FAKE_DEVICE_H_
#define TESTS_DRIVERS_VIRTIO_QUEUE_SUBMISSION_FAKE_DEVICE_H_

#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>

/**
 * Software virtio device used to drive the real interrupt path.
 *
 * Each device owns at most one virtqueue, so two independent devices let a
 * test destroy one queue while the other still has blocked submitters.
 */
const struct device *fake_device_a(void);
const struct device *fake_device_b(void);

/** Attach (or detach, with NULL) the queue served by a fake device. */
void fake_device_attach_queue(const struct device *dev, struct virtq *vq);

struct virtq *fake_device_get_attached_queue(const struct device *dev);

/** Append one used ring entry, as the device would. */
void fake_device_add_completion(const struct device *dev, uint16_t head, uint32_t len);

/** Append a used ring entry and run the real virtio_isr() from interrupt context. */
void fake_device_complete_request(const struct device *dev, uint16_t head, uint32_t len);

/** Index of the last descriptor chain published to the available ring. */
uint16_t fake_device_last_submitted_head(const struct virtq *vq);

#endif /* TESTS_DRIVERS_VIRTIO_QUEUE_SUBMISSION_FAKE_DEVICE_H_ */
