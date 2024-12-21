/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing API definitions for the
 * SPSC queue management layer of the nRF71 Wi-Fi driver.

 * SPSC Queue Manager API for handling 32-bit values.
 *
 * The Queue Manager API for Single-Producer, Single-Consumer (SPSC) queue. This
 * API allows queues to be allocated, pushed and popped.
 */

#ifndef SPSC_QM_H
#define SPSC_QM_H

#include <zephyr/sys/spsc_pbuf.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct spsc_pbuf spsc_queue_t;

/**
 * Initialise and allocate SPSC queue.
 *
 * @param[in] address : Address to allocate.
 * @param[in] size : Size in bytes to allocate.
 * @return : SPSC packet queue.
 */
spsc_queue_t *spsc32_init(uint32_t address, size_t size);

/**
 * Push a value onto the tail of a queue.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @param[in] value : The value to push to the queue.
 * @return : true if push is successful, false otherwise.
 */
bool spsc32_push(spsc_queue_t *pb, uint32_t value);

/**
 * Pop a value from the head of a queue and return it.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @param[out] out_value : Pointer to the value to pop to.
 * @return : true if pop is successful, false otherwise.
 */
bool spsc32_pop(spsc_queue_t *pb, uint32_t *out_value);

/**
 * Return a value at the head of a queue without popping it.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @param[out] out_value : Pointer to value to read from the head of the queue.
 * @return : true if read is successful, false otherwise.
 */
bool spsc32_read_head(spsc_queue_t *pb, uint32_t *out_value);

/**
 * Test whether a queue is empty.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @return : true if the queue is empty, false otherwise.
 */
bool spsc32_is_empty(spsc_queue_t *pb);

/**
 * Test whether a queue is full.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @return : true if the queue is full, false otherwise.
 */
bool spsc32_is_full(spsc_queue_t *pb);

#endif /* SPSC_QM_H */
