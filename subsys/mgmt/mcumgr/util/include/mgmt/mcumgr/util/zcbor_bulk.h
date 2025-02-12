/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_ZCBOR_BULK_PRIV_
#define H_ZCBOR_BULK_PRIV_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/toolchain.h>

/** @cond INTERNAL_HIDDEN */

struct zcbor_map_decode_key_val {
	struct zcbor_string key;	/* Map key string */
	zcbor_decoder_t *decoder;	/* Key corresponding decoder */
	void *value_ptr;
	bool found;
};

/** @brief Define single key-decoder mapping
 *
 * The macro creates a single zcbor_map_decode_key_val type object.
 *
 * @param k	key is "" enclosed string representing key;
 * @param dec	decoder function; this should be zcbor_decoder_t
 *		type function from zcbor or a user provided implementation
 *		compatible with the type.
 * @param vp	non-NULL pointer for result of decoding; should correspond
 *		to type served by decoder function for the mapping.
 */
#define ZCBOR_MAP_DECODE_KEY_DECODER(k, dec, vp)		\
	{							\
		{						\
			.value = k,				\
			.len = sizeof(k) - 1,			\
		},						\
		.decoder = (zcbor_decoder_t *)dec,		\
		.value_ptr = vp,				\
		.found = false,					\
	}

/** @brief Define single key-value decode mapping
 *
 * ZCBOR_MAP_DECODE_KEY_DECODER should be used instead of this macro as,
 * this macro does not allow keys with whitespaces embeeded, which CBOR
 * does allow.
 *
 * The macro creates a single zcbor_map_decode_key_val type object.
 *
 * @param k	key; the @p k will be stringified so should be given
 *		without "";
 * @param dec	decoder function; this should be zcbor_decoder_t
 *		type function from zcbor or a user provided implementation
 *		compatible with the type.
 * @param vp	non-NULL pointer for result of decoding; should correspond
 *		to type served by decoder function for the mapping.
 */
#define ZCBOR_MAP_DECODE_KEY_VAL(k, dec, vp)			\
	ZCBOR_MAP_DECODE_KEY_DECODER(STRINGIFY(k), dec, vp)

/** @brief Decodes single level map according to a provided key-decode map.
 *
 * The function takes @p map of key to decoder array defined as:
 *
 *	struct zcbor_map_decode_key_val map[] = {
 *		ZCBOR_MAP_DECODE_KEY_DECODER("key0", decode_fun0, val_ptr0),
 *		ZCBOR_MAP_DECODE_KEY_DECODER("key1", decode_fun1, val_ptr1),
 *		...
 *	};
 *
 * where "key?" is string representing key; the decode_fun? is
 * zcbor_decoder_t compatible function, either from zcbor or defined by
 * user; val_ptr? are pointers to variables where decoder function for
 * a given key will place a decoded value - they have to agree in type
 * with decoder function.
 *
 * Failure to decode any of values will cause the function to return
 * negative error, and leave the map open: map is broken anyway or key-decoder
 * mapping is broken, and we can not really decode the map.
 *
 * Note that the function opens map by itself and will fail if map
 * is already opened.
 *
 * @param zsd		zcbor decoder state;
 * @param map		key-decoder mapping list;
 * @param map_size	size of maps, both maps have to have the same size;
 * @param matched	pointer to the  counter of matched keys, zeroed upon
 *			successful map entry and incremented only for successful
 *			decoded fields.
 * @return		0 when the whole map has been parsed, there have been
 *			no decoding errors, and map has been closed successfully;
 *			-ENOMSG when given decoder function failed to decode
 *			value;
 *			-EADDRINUSE when key appears twice within map, map is then
 *			parsed up to they key that has appeared twice;
 *			-EBADMSG when failed to close map.
 */
int zcbor_map_decode_bulk(zcbor_state_t *zsd, struct zcbor_map_decode_key_val *map,
	size_t map_size, size_t *matched);

/** @brief Check whether key has been found by zcbor_map_decode_bulk
 *
 * @param map		key-decoder mapping list;
 * @param map_size	size of maps, both maps have to have the same size;
 * @param key		string representing key to check with map.
 *
 * @return		true if key has been found during decoding, false otherwise.
 *
 */
bool zcbor_map_decode_bulk_key_found(struct zcbor_map_decode_key_val *map,
	size_t map_size, const char *key);

/** @brief Reset decoding state of key-value
 *
 * The function takes @p map and resets internal fields that mark decoding state
 * of the map. Function needs to be used on @p map after the map has been already
 * used for decoding other buffer, otherwise decoding may fail.
 *
 * @param map		key-decoder mapping list;
 * @param map_size	size of maps, both maps have to have the same size.
 *
 * @return Nothing.
 */
void zcbor_map_decode_bulk_reset(struct zcbor_map_decode_key_val *map, size_t map_size);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* H_ZCBOR_BULK_PRIV_ */
