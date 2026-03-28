/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DATA_QUEUE_H
#include <zephyr/kernel.h>


struct data_queue {
	struct k_queue q;
	struct k_heap h;
};

void data_queue_init(struct data_queue *q, void *mem, size_t bytes);

int data_queue_put(struct data_queue *q, const void *data, size_t bytes, k_timeout_t timeout);

void *data_queue_get(struct data_queue *q, size_t *size, k_timeout_t timeout);

void data_queue_release(struct data_queue *q, void *data);

int data_queue_is_empty(struct data_queue *q);

#endif /* DATA_QUEUE_H */
