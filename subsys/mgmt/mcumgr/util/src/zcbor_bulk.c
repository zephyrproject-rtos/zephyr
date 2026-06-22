/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>

int zcbor_map_decode_bulk(zcbor_state_t *zsd, struct zcbor_map_decode_key_val *map,
	size_t map_size, size_t *matched)
{
	bool ok;
	struct zcbor_map_decode_key_val *dptr = map;

	if (!zcbor_map_start_decode(zsd)) {
		return -EBADMSG;
	}

	*matched = 0;
	ok = true;

	do {
		struct zcbor_string key;
		bool found = false;
		size_t map_count = 0;

		ok = zcbor_tstr_decode(zsd, &key);

		while (ok && map_count < map_size) {
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

		if (!found && ok) {
			ok = zcbor_any_skip(zsd, NULL);
		}
	} while (ok);

	return zcbor_map_end_decode(zsd) ? 0 : -EBADMSG;
}

bool zcbor_map_decode_bulk_key_found(struct zcbor_map_decode_key_val *map, size_t map_size,
	const char *key)
{
	size_t key_len;
	struct zcbor_map_decode_key_val *dptr = map;

	/* Lazy run, comparing pointers only assuming that compiler will be able to store
	 * read-only string of the same value as single instance.
	 */
	while (dptr < (map + map_size)) {
		if (dptr->key.value == (const uint8_t *)key) {
			return dptr->found;
		}
		++dptr;
	}

	/* Lazy run failed so need to do real comprison */
	key_len = strlen(key);
	dptr = map;

	while (dptr < (map + map_size)) {
		if (dptr->key.len == key_len &&
		    memcmp(key, dptr->key.value, key_len) == 0) {
			return dptr->found;
		}
		++dptr;
	}

	return false;
}

void zcbor_map_decode_bulk_reset(struct zcbor_map_decode_key_val *map, size_t map_size)
{
	for (size_t map_index = 0; map_index < map_size; ++map_index) {
		map[map_index].found = false;
	}
}
