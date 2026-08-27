/*
 * Copyright (c) 2020 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/sys/crc.h>
#include <zephyr/random/random.h>

struct phdr_desc {
	struct phdr_desc  *next;    /* Next pkt descriptor in respective queue */
	uint8_t *ptr;                           /* Pointer to header */
};

struct phdr_desc_queue {
	struct phdr_desc  *head;   /* packet headers are removed from here */
	struct phdr_desc  *tail;   /* packet headers are added here*/
	int count;
};

void phdr_desc_enqueue(struct phdr_desc_queue *queue, struct phdr_desc *desc,
			struct k_mutex *mutex);

struct phdr_desc *phdr_desc_dequeue(struct phdr_desc_queue *queue,
					 struct k_mutex *mutex);
