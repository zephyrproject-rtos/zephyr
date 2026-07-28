/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NFC_NDEF_H_
#define ZEPHYR_INCLUDE_NFC_NDEF_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NDEF message codec
 * @defgroup nfc_ndef NDEF message codec
 * @ingroup nfc_subsys
 * @{
 */

/** @brief NDEF Type Name Format (record header TNF field). */
enum nfc_ndef_tnf {
	NFC_NDEF_TNF_EMPTY = 0x00,
	NFC_NDEF_TNF_WELL_KNOWN = 0x01, /* NFC Forum RTD */
	NFC_NDEF_TNF_MIME = 0x02,
	NFC_NDEF_TNF_ABS_URI = 0x03,
	NFC_NDEF_TNF_EXTERNAL = 0x04,
	NFC_NDEF_TNF_UNKNOWN = 0x05,
};

/**
 * @brief A single NDEF record.
 *
 * On parse, @a type / @a id / @a payload point into the caller's input buffer
 * (zero-copy): that buffer must outlive the record. Short-record and normal
 * payload lengths are both normalised into @a payload_len.
 */
struct nfc_ndef_record {
	enum nfc_ndef_tnf tnf;
	const uint8_t *type;
	uint8_t type_len;
	const uint8_t *id;
	uint8_t id_len;
	const uint8_t *payload;
	uint32_t payload_len;
};

/**
 * @brief Parse an NDEF message into its records (zero-copy).
 *
 * @param buf Message bytes.
 * @param len Length of @p buf.
 * @param records Output array.
 * @param count In: capacity of @p records. Out: number of records parsed.
 *
 * @retval 0 on success.
 * @retval -EBADMSG on malformed input.
 * @retval -ENOMEM if there are more records than the capacity.
 * @retval -ENOTSUP if the message uses chunked records.
 */
int nfc_ndef_msg_parse(const uint8_t *buf, size_t len, struct nfc_ndef_record *records,
		       size_t *count);

/**
 * @brief Encode records into an NDEF message.
 *
 * @param records Input records.
 * @param count Number of records.
 * @param buf Output buffer.
 * @param len In: capacity of @p buf. Out: bytes written.
 *
 * @retval 0 on success.
 * @retval -ENOMEM if @p buf is too small.
 */
int nfc_ndef_msg_encode(const struct nfc_ndef_record *records, size_t count, uint8_t *buf,
			size_t *len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NFC_NDEF_H_ */
