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

typedef struct spsc_pbuf SPSC_QUEUE_T;

/**
 * Initialise and allocate SPSC queue.
 *
 * @param[in] address : Address to allocate.
 * @param[in] size : Size in bytes to allocate.
 * @return : SPSC packet queue.
 */
SPSC_QUEUE_T *SPSC32_init(uint32_t address, size_t size);

/**
 * Push a value onto the tail of a queue.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @param[in] value : The value to push to the queue.
 * @return : true if push is successful, false otherwise.
 */
bool SPSC32_push(SPSC_QUEUE_T *pb, uint32_t value);

/**
 * Pop a value from the head of a queue and return it.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @param[out] outValue : Pointer to the value to pop to.
 * @return : true if pop is successful, false otherwise.
 */
bool SPSC32_pop(SPSC_QUEUE_T *pb, uint32_t *outValue);

/**
 * Return a value at the head of a queue without popping it.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @param[out] outValue : Pointer to value to read from the head of the queue.
 * @return : true if read is successful, false otherwise.
 */
bool SPSC32_readHead(SPSC_QUEUE_T *pb, uint32_t *outValue);

/**
 * Test whether a queue is empty.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @return : true if the queue is empty, false otherwise.
 */
bool SPSC32_isEmpty(SPSC_QUEUE_T *pb);

/**
 * Test whether a queue is full.
 *
 * @param[in] pb : Pointer to SPSC packet queue.
 * @return : true if the queue is full, false otherwise.
 */
bool SPSC32_isFull(SPSC_QUEUE_T *pb);

#endif /* SPSC_QM_H */
