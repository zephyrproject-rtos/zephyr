/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <string.h>
#include <errno.h>
#include "zcbor_bulk/zcbor_bulk_priv.h"

int zcbor_map_decode_bulk(zcbor_state_t *zsd, struct zcbor_map_decode_key_val *map,
	size_t map_size, size_t *matched)
{
	bool ok;
	bool dok;		/* Key decode ok */
	struct zcbor_map_decode_key_val *dptr = map;

	if (!zcbor_map_start_decode(zsd)) {
		return -EBADMSG;
	}

	*matched = 0;
	dok = true;

	do {
		struct zcbor_string key;
		bool found = false;
		size_t map_count = 0;

		dok = zcbor_tstr_decode(zsd, &key);

		while (dok && map_count < map_size) {
			if (dptr >= (map + map_size)) {
				dptr = map;
			}

			if (key.len == dptr->key.len		&&
			    memcmp(key.value, dptr->key.value, key.len) == 0) {

				if (dptr->found) {
					return -EADDRINUSE;
				}
				if (!dptr->decoder(zsd, dptr->value_ptr)) {
					/* Failure to decode value matched to key
					 * means that either decoder has been
					 * incorrectly assigned or SMP payload
					 * is broken anyway.
					 */
					return -ENOMSG;
				}

				dptr->found = true;
				found = true;
				++dptr;
				++(*matched);
				break;
			}

			++dptr;
			++map_count;
		}

		if (!found && dok) {
			ok = zcbor_any_skip(zsd, NULL);
		}
	} while (ok && dok);

	return zcbor_map_end_decode(zsd) ? 0 : -EBADMSG;
}
