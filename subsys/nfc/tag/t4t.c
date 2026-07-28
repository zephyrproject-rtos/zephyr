/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "tag/tag_internal.h"

LOG_MODULE_REGISTER(nfc_t4t, CONFIG_NFC_LOG_LEVEL);

/* SELECT NDEF Tag Application by AID. */
static const uint8_t t4t_select_app[] = {0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76,
					 0x00, 0x00, 0x85, 0x01, 0x01, 0x00};
/* SELECT the Capability Container file (E103). */
static const uint8_t t4t_select_cc[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03};

#define T4T_SW_OK       0x9000U
#define T4T_CC_READ_LEN 0x0FU
#define T4T_READ_CHUNK  0x3BU

int z_nfc_t4t_parse_cc(const uint8_t *cc, uint16_t len, uint16_t *fileid, uint16_t *max_len,
		       bool *writable)
{
	/* CCLEN(2) ver(1) MLe(2) MLc(2) then NDEF File Control TLV (T=04 L=06). */
	if (len < 15U) {
		return -EBADMSG;
	}
	if (cc[7] != 0x04U || cc[8] != 0x06U) {
		return -EBADMSG;
	}

	*fileid = sys_get_be16(&cc[9]);
	*max_len = sys_get_be16(&cc[11]);
	*writable = (cc[14] == 0x00U);
	return 0;
}

static int t4t_apdu(struct nfc_t4t_tag *t4t, const uint8_t *capdu, uint16_t clen, uint8_t *resp,
		    uint16_t *resp_len, k_timeout_t timeout)
{
	uint16_t len = *resp_len;
	uint16_t sw;
	int ret;

	ret = nfc_iso_dep_transceive(&t4t->iso_dep, capdu, clen, resp, &len, 0U, timeout);
	if (ret < 0) {
		return ret;
	}
	if (len < 2U) {
		LOG_DBG("R-APDU too short for a status word (%u bytes)", len);
		return -EIO;
	}

	sw = sys_get_be16(&resp[len - 2U]);
	if (sw != T4T_SW_OK) {
		LOG_DBG("APDU rejected, SW=%04x", sw);
		return -ENOTSUP;
	}

	*resp_len = len - 2U;
	return 0;
}

static int t4t_select_file(struct nfc_t4t_tag *t4t, uint16_t fileid, k_timeout_t timeout)
{
	uint8_t apdu[7] = {0x00, 0xA4, 0x00, 0x0C, 0x02};
	uint8_t resp[2];
	uint16_t resp_len = sizeof(resp);

	sys_put_be16(fileid, &apdu[5]);

	return t4t_apdu(t4t, apdu, sizeof(apdu), resp, &resp_len, timeout);
}

static int t4t_read_binary(struct nfc_t4t_tag *t4t, uint16_t offset, uint8_t le, uint8_t *out,
			   uint16_t *out_len, k_timeout_t timeout)
{
	uint8_t apdu[5] = {0x00, 0xB0};
	uint16_t resp_len = *out_len;
	int ret;

	sys_put_be16(offset, &apdu[2]);
	apdu[4] = le;

	ret = t4t_apdu(t4t, apdu, sizeof(apdu), out, &resp_len, timeout);
	if (ret < 0) {
		return ret;
	}

	*out_len = resp_len;
	return 0;
}

static int t4t_update_binary(struct nfc_t4t_tag *t4t, uint16_t offset, const uint8_t *data,
			     uint8_t len, k_timeout_t timeout)
{
	uint8_t apdu[5U + T4T_READ_CHUNK] = {0x00, 0xD6};
	uint8_t resp[2];
	uint16_t resp_len = sizeof(resp);

	sys_put_be16(offset, &apdu[2]);
	apdu[4] = len;
	memcpy(&apdu[5], data, len);

	return t4t_apdu(t4t, apdu, 5U + len, resp, &resp_len, timeout);
}

static int t4t_read_nlen(struct nfc_t4t_tag *t4t, uint16_t *nlen, k_timeout_t timeout)
{
	uint8_t buf[2];
	uint16_t len = sizeof(buf);
	int ret;

	ret = t4t_read_binary(t4t, 0U, 2U, buf, &len, timeout);
	if (ret < 0) {
		return ret;
	}
	if (len != 2U) {
		return -EBADMSG;
	}

	*nlen = sys_get_be16(buf);

	return 0;
}

static int t4t_write_nlen(struct nfc_t4t_tag *t4t, uint16_t nlen, k_timeout_t timeout)
{
	uint8_t val[2];

	sys_put_be16(nlen, val);

	return t4t_update_binary(t4t, 0U, val, sizeof(val), timeout);
}

static int t4t_send_fixed(struct nfc_t4t_tag *t4t, const uint8_t *apdu, size_t len,
			  k_timeout_t timeout)
{
	uint8_t resp[32];
	uint16_t resp_len = sizeof(resp);

	return t4t_apdu(t4t, apdu, len, resp, &resp_len, timeout);
}

int z_nfc_t4t_connect(struct nfc_poller *poller, const struct nfc_target *target,
		      struct nfc_t4t_tag *t4t, k_timeout_t timeout)
{
	struct nfc_iso_dep_connect_param param = {.cid = 0, .timeout = timeout};
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	uint8_t cc[32];
	uint16_t cc_len = sizeof(cc);
	int ret;

	if (target == NULL || t4t == NULL) {
		return -EINVAL;
	}

	memset(t4t, 0, sizeof(*t4t));

	ret = nfc_iso_dep_connect(poller, target, &param, &t4t->iso_dep);
	if (ret < 0) {
		return ret;
	}

	ret = t4t_send_fixed(t4t, t4t_select_app, sizeof(t4t_select_app),
			     sys_timepoint_timeout(deadline));
	if (ret < 0) {
		return ret;
	}

	ret = t4t_send_fixed(t4t, t4t_select_cc, sizeof(t4t_select_cc),
			     sys_timepoint_timeout(deadline));
	if (ret < 0) {
		return ret;
	}

	ret = t4t_read_binary(t4t, 0U, T4T_CC_READ_LEN, cc, &cc_len,
			      sys_timepoint_timeout(deadline));
	if (ret < 0) {
		return ret;
	}

	return z_nfc_t4t_parse_cc(cc, cc_len, &t4t->ndef_file_id, &t4t->max_ndef_len,
				  &t4t->writable);
}

static int t4t_read_chunks(struct nfc_t4t_tag *t4t, uint16_t nlen, uint8_t *buf, uint16_t *len,
			   k_timepoint_t deadline)
{
	uint16_t offset = 2U;

	*len = 0U;
	while (*len < nlen) {
		uint8_t chunk[T4T_READ_CHUNK];
		uint16_t chunk_len = sizeof(chunk);
		uint8_t le = (uint8_t)MIN((uint16_t)T4T_READ_CHUNK, nlen - *len);
		int ret;

		ret = t4t_read_binary(t4t, offset, le, chunk, &chunk_len,
				      sys_timepoint_timeout(deadline));
		if (ret < 0) {
			return ret;
		}
		if (chunk_len == 0U) {
			return -EIO;
		}

		memcpy(&buf[*len], chunk, chunk_len);
		*len += chunk_len;
		offset += chunk_len;
	}

	return 0;
}

int z_nfc_t4t_read_ndef(struct nfc_t4t_tag *t4t, uint8_t *buf, uint16_t *len, k_timeout_t timeout)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	uint16_t nlen;
	int ret;

	if (buf == NULL || len == NULL) {
		return -EINVAL;
	}

	ret = t4t_select_file(t4t, t4t->ndef_file_id, sys_timepoint_timeout(deadline));
	if (ret < 0) {
		return ret;
	}

	ret = t4t_read_nlen(t4t, &nlen, sys_timepoint_timeout(deadline));
	if (ret < 0) {
		return ret;
	}
	if (nlen == 0U) {
		return -ENOENT;
	}
	if (nlen > *len) {
		return -ENOMEM;
	}

	return t4t_read_chunks(t4t, nlen, buf, len, deadline);
}

int z_nfc_t4t_write_ndef(struct nfc_t4t_tag *t4t, const uint8_t *buf, uint16_t len,
			 k_timeout_t timeout)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	uint16_t offset = 2U;
	uint16_t written = 0U;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}
	if (!t4t->writable) {
		return -EACCES;
	}
	if (len > t4t->max_ndef_len) {
		return -ENOMEM;
	}

	ret = t4t_select_file(t4t, t4t->ndef_file_id, sys_timepoint_timeout(deadline));
	if (ret < 0) {
		return ret;
	}

	/* NLEN = 0 while writing, so a reader never sees a partial message. */
	ret = t4t_write_nlen(t4t, 0U, sys_timepoint_timeout(deadline));
	if (ret < 0) {
		return ret;
	}

	while (written < len) {
		uint8_t chunk = (uint8_t)MIN((uint16_t)T4T_READ_CHUNK, len - written);

		ret = t4t_update_binary(t4t, offset, &buf[written], chunk,
					sys_timepoint_timeout(deadline));
		if (ret < 0) {
			return ret;
		}
		written += chunk;
		offset += chunk;
	}

	return t4t_write_nlen(t4t, len, sys_timepoint_timeout(deadline));
}
