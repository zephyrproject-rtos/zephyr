/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <string.h>
#include <stdbool.h>

/**
 * Memory FIFO with unique elements, permitting enqueue at tail
 * (in) and dequeue from head (out), as well as
 * remove/delete of individual elements identified by idx or
 * value.
 *
 * Implemented as a circular queue of elements. Elements lie
 * contiguously in the backing storage.
 *
 */
static uint8_t ufifo_get_idx(uint8_t *fifo_m,
			     uint8_t fifo_in, uint8_t fifo_out,
			     uint8_t fifo_s, uint8_t fifo_n,
			     uint8_t const *entry)
{
	uint8_t idx = fifo_out;

	while (idx != fifo_in) {
		if (!memcmp(entry, &fifo_m[idx], fifo_s)) {
			break;
		}
		if (++idx == fifo_n) {
			idx = 0;
		}
	}
	return idx;
}

bool ufifo_enqueue(uint8_t *fifo_m,
			  uint8_t *fifo_in, uint8_t *fifo_out, uint8_t *fifo_empty,
			  uint8_t fifo_s, uint8_t fifo_n,
			  uint8_t const *entry)
{
	uint8_t idx = ufifo_get_idx(fifo_m, *fifo_in, *fifo_out, fifo_s, fifo_n, entry);

	if (*fifo_in != *fifo_out || *fifo_empty) {
		if (idx == *fifo_in) {
			/* requester is not already in line */
			memcpy(&fifo_m[(*fifo_in)++], entry, fifo_s);
			*fifo_empty = 0;
			if (*fifo_in == fifo_n) {
				*fifo_in = 0;
			}
		}
	}
	return (idx == *fifo_out);
}

void ufifo_unqueue(uint8_t *fifo_m,
			  uint8_t *fifo_in, uint8_t *fifo_out, uint8_t *fifo_empty,
			  uint8_t fifo_s, uint8_t fifo_n,
			  uint8_t const *entry)
{
	uint8_t idx = ufifo_get_idx(fifo_m, *fifo_in, *fifo_out, fifo_s, fifo_n, entry);
	uint8_t curr_idx;

	if (idx != *fifo_in) {
		/* requester is in line, so lets remove */
		while (idx != *fifo_in) {
			curr_idx = idx;
			if (++idx == fifo_n) {
				idx = 0;
			}
			memcpy(&fifo_m[curr_idx], &fifo_m[idx], fifo_s);
		}

		/* one entry was removed, so decrement the in-'ptr' */
		if (*fifo_in == 0) {
			*fifo_in = fifo_n;
		}
		(*fifo_in)--;

		if (*fifo_in == *fifo_out) {
			*fifo_empty = 1;
		}
	}
}

void ufifo_dequeue_head(uint8_t *fifo_m,
			       uint8_t *fifo_in, uint8_t *fifo_out, uint8_t *fifo_empty,
			       uint8_t fifo_s, uint8_t fifo_n,
			       uint8_t *head)
{
	if (head) {
		memcpy(head, &fifo_m[*fifo_out], fifo_s);
	}
	if (++*fifo_out == fifo_n) {
		*fifo_out = 0;
	}
	/* we're removing entry, so not full anymore */
	if (*fifo_in == *fifo_out) {
		*fifo_empty = 1;
	}
}
