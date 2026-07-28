/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NFC_TAG_TAG_INTERNAL_H_
#define ZEPHYR_SUBSYS_NFC_TAG_TAG_INTERNAL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/nfc/tag.h>
#include <zephyr/types.h>

/**
 * @brief Resolve the tag type from a discovered target.
 *
 * @retval 0 on success (@p out set).
 * @retval -ENOTSUP if the target is not a supported NDEF tag type.
 */
int z_nfc_tag_detect(const struct nfc_target *target, enum nfc_tag_type *out);

/**
 * @brief Locate the NDEF Message TLV (T=0x03) in a Type 2 data area.
 *
 * @param off Out: offset of the NDEF value within @p data.
 * @param ndef_len Out: length of the NDEF value.
 *
 * @p off and @p ndef_len are set as soon as the NDEF TLV header is decoded, so
 * a caller reading incrementally learns the message length before it has all
 * of it.
 *
 * @retval 0 if found and the whole value is present.
 * @retval -EAGAIN if @p data ends inside a TLV, so more of it may help.
 * @retval -ENOENT if a terminator is reached with no NDEF TLV.
 */
int z_nfc_t2t_find_ndef(const uint8_t *data, uint16_t len, uint16_t *off, uint16_t *ndef_len);

/**
 * @brief Parse a Type 4 Capability Container file.
 *
 * @param fileid Out: NDEF file identifier.
 * @param max_len Out: maximum NDEF message length.
 * @param writable Out: whether write access is granted.
 *
 * @retval 0 on success.
 * @retval -EBADMSG on a malformed CC.
 */
int z_nfc_t4t_parse_cc(const uint8_t *cc, uint16_t len, uint16_t *fileid, uint16_t *max_len,
		       bool *writable);

/* Type 2 backend. Callers use nfc_tag_connect() / nfc_tag_read_ndef() instead. */
int z_nfc_t2t_connect(struct nfc_poller *poller, const struct nfc_target *target,
		      struct nfc_t2t_tag *t2t, k_timeout_t timeout);
int z_nfc_t2t_read_ndef(struct nfc_t2t_tag *t2t, uint8_t *buf, uint16_t *len, k_timeout_t timeout);
int z_nfc_t2t_write_ndef(struct nfc_t2t_tag *t2t, const uint8_t *buf, uint16_t len,
			 k_timeout_t timeout);

/* Type 4 backend. Raw access goes through nfc_tag_iso_dep() instead. */
int z_nfc_t4t_connect(struct nfc_poller *poller, const struct nfc_target *target,
		      struct nfc_t4t_tag *t4t, k_timeout_t timeout);
int z_nfc_t4t_read_ndef(struct nfc_t4t_tag *t4t, uint8_t *buf, uint16_t *len, k_timeout_t timeout);
int z_nfc_t4t_write_ndef(struct nfc_t4t_tag *t4t, const uint8_t *buf, uint16_t len,
			 k_timeout_t timeout);

#endif /* ZEPHYR_SUBSYS_NFC_TAG_TAG_INTERNAL_H_ */
