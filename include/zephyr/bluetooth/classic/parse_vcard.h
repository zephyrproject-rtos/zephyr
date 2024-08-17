/** @file
 *  @brief Parse vcard handling.
 */

/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_PARSE_VCARD_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_PARSE_VCARD_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PBAP_VCARD_MAX_CACHED 512
#define PBAP_VCARD_MAX_LINE   20

enum VCRAD_RESULT_TYPE {
	VCARD_TYPE_VERSION = 0, /* vcard version */
	VCARD_TYPE_FN = 1,      /* Formatted Name */
	VCARD_TYPE_N = 2,       /* Structured Presentation of Name */
	VCARD_TYPE_TEL = 7,     /* Telephone Number */
	VCARD_TYPE_TZ = 10,     /* Time Zone */
	VCARD_TYPE_TS = 28,     /* Time stamp */
	VCARD_TYPE_CARD_HDL = 31,
};

struct parse_cached_data {
	uint16_t cached_len;
	uint16_t parse_pos;
	uint8_t cached[PBAP_VCARD_MAX_CACHED];
};

struct parse_vcard_result {
	uint8_t type;
	uint16_t len;
	uint8_t *data;
};

uint16_t pbap_parse_copy_to_cached(uint8_t *data, uint16_t len, struct parse_cached_data *vcached);

void pbap_parse_vcached_move(struct parse_cached_data *vcached);

void pbap_parse_vcached_vcard(struct parse_cached_data *vcached, struct parse_vcard_result *result,
			      uint8_t *result_size);

void pbap_parse_vcached_list(struct parse_cached_data *vcached, struct parse_vcard_result *result,
			     uint8_t *result_size);

#ifdef __cplusplus
}
#endif
#endif /* __BT_PARSE_VCARD_H */
