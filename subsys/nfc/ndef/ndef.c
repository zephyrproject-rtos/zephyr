/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/nfc/ndef.h>
#include <zephyr/sys/byteorder.h>

#define NDEF_HDR_MB  0x80U
#define NDEF_HDR_ME  0x40U
#define NDEF_HDR_CF  0x20U
#define NDEF_HDR_SR  0x10U
#define NDEF_HDR_IL  0x08U
#define NDEF_HDR_TNF 0x07U

static int ndef_record_fields(const uint8_t *buf, size_t len, size_t *pos, uint8_t hdr,
			      struct nfc_ndef_record *r)
{
	bool sr = (hdr & NDEF_HDR_SR) != 0U;
	bool il = (hdr & NDEF_HDR_IL) != 0U;
	size_t p = *pos;

	r->tnf = hdr & NDEF_HDR_TNF;

	if (p + 1U + (sr ? 1U : 4U) + (il ? 1U : 0U) > len) {
		return -EBADMSG;
	}

	r->type_len = buf[p++];
	if (sr) {
		r->payload_len = buf[p++];
	} else {
		r->payload_len = sys_get_be32(&buf[p]);
		p += 4U;
	}
	r->id_len = il ? buf[p++] : 0U;

	if (p + r->type_len + r->id_len + r->payload_len > len) {
		return -EBADMSG;
	}

	r->type = r->type_len ? &buf[p] : NULL;
	p += r->type_len;
	r->id = r->id_len ? &buf[p] : NULL;
	p += r->id_len;
	r->payload = r->payload_len ? &buf[p] : NULL;
	p += r->payload_len;

	*pos = p;

	return 0;
}

int nfc_ndef_msg_parse(const uint8_t *buf, size_t len, struct nfc_ndef_record *records,
		       size_t *count)
{
	size_t cap = *count;
	size_t pos = 0;
	size_t n = 0;
	bool ended = false;
	int ret;

	if (buf == NULL || records == NULL || count == NULL) {
		return -EINVAL;
	}

	while (pos < len) {
		uint8_t hdr;

		if (n >= cap) {
			return -ENOMEM;
		}

		hdr = buf[pos++];

		if ((hdr & NDEF_HDR_CF) != 0U) {
			return -ENOTSUP;
		}
		if (n == 0U && (hdr & NDEF_HDR_MB) == 0U) {
			return -EBADMSG;
		}

		ret = ndef_record_fields(buf, len, &pos, hdr, &records[n]);
		if (ret < 0) {
			return ret;
		}
		n++;

		if ((hdr & NDEF_HDR_ME) != 0U) {
			ended = true;
			break;
		}
	}

	if (len != 0U && !ended) {
		return -EBADMSG;
	}

	*count = n;

	return 0;
}

static size_t ndef_record_size(const struct nfc_ndef_record *r)
{
	return 2U + (r->payload_len <= UINT8_MAX ? 1U : 4U) + (r->id_len > 0U ? 1U : 0U) +
	       r->type_len + r->id_len + r->payload_len;
}

static void ndef_record_write(const struct nfc_ndef_record *r, bool first, bool last, uint8_t *buf,
			      size_t *pos)
{
	bool sr = r->payload_len <= UINT8_MAX;
	bool il = r->id_len > 0U;
	uint8_t hdr = r->tnf & NDEF_HDR_TNF;
	size_t p = *pos;

	if (first) {
		hdr |= NDEF_HDR_MB;
	}
	if (last) {
		hdr |= NDEF_HDR_ME;
	}
	if (sr) {
		hdr |= NDEF_HDR_SR;
	}
	if (il) {
		hdr |= NDEF_HDR_IL;
	}

	buf[p++] = hdr;
	buf[p++] = r->type_len;
	if (sr) {
		buf[p++] = (uint8_t)r->payload_len;
	} else {
		sys_put_be32(r->payload_len, &buf[p]);
		p += 4U;
	}
	if (il) {
		buf[p++] = r->id_len;
	}
	if (r->type_len) {
		memcpy(&buf[p], r->type, r->type_len);
		p += r->type_len;
	}
	if (il) {
		memcpy(&buf[p], r->id, r->id_len);
		p += r->id_len;
	}
	if (r->payload_len) {
		memcpy(&buf[p], r->payload, r->payload_len);
		p += r->payload_len;
	}

	*pos = p;
}

int nfc_ndef_msg_encode(const struct nfc_ndef_record *records, size_t count, uint8_t *buf,
			size_t *len)
{
	size_t cap = *len;
	size_t pos = 0;

	if (records == NULL || buf == NULL || len == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		if (pos + ndef_record_size(&records[i]) > cap) {
			return -ENOMEM;
		}

		ndef_record_write(&records[i], i == 0U, i == count - 1U, buf, &pos);
	}

	*len = pos;

	return 0;
}
