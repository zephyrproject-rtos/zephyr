/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include <soc.h>

#include "hal/cpu.h"

#include "util/util.h"
#include "dbuf.h"

void *dbuf_alloc(struct dbuf_hdr *hdr, uint8_t *idx)
{
	uint8_t first, last;

	first = hdr->first;
	last = hdr->last;
	if (first == last) {
		/* Return the index of next free PDU in the double buffer */
		last++;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0U;
		}
	} else {
		uint8_t first_latest;

		/* LLL has not consumed the first PDU. Revert back the `last` so
		 * that LLL still consumes the first PDU while the caller of
		 * this function updates/modifies the latest PDU.
		 *
		 * Under race condition:
		 * 1. LLL runs before `pdu->last` is reverted, then `pdu->first`
		 *    has changed, hence restore `pdu->last` and return index of
		 *    next free PDU in the double buffer.
		 * 2. LLL runs after `pdu->last` is reverted, then `pdu->first`
		 *    will not change, return the saved `last` as the index of
		 *    the next free PDU in the double buffer.
		 */
		hdr->last = first;
		cpu_dmb();
		first_latest = hdr->first;
		if (first_latest != first) {
			hdr->last = last;
			last++;
			if (last == DOUBLE_BUFFER_SIZE) {
				last = 0U;
			}
		}
	}

	*idx = last;

	return &hdr->data[last * hdr->elem_size];
}

void *dbuf_latest_get(struct dbuf_hdr *hdr, uint8_t *is_modified)
{
	uint8_t first;

	first = hdr->first;
	if (first != hdr->last) {
		uint8_t cfg_idx;

		cfg_idx = first;

		first += 1U;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0U;
		}
		hdr->first = first;

		if (is_modified) {
			*is_modified = 1U;
		}
	}

	return &hdr->data[first * hdr->elem_size];
}
