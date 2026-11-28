/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/snmp/snmp_ber.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)

bool snmp_oid_equal(const struct snmp_oid *o1, const struct snmp_oid *o2)
{
	if (!o1 || !o2 || o1->len != o2->len) {
		return false;
	}
	for (uint8_t i = 0; i < o1->len; i++) {
		if (o1->ids[i] != o2->ids[i]) {
			return false;
		}
	}
	return true;
}

int snmp_oid_compare(const struct snmp_oid *o1, const struct snmp_oid *o2)
{
	if (!o1 && !o2) {
		return 0;
	}
	if (!o1) {
		return -1;
	}
	if (!o2) {
		return 1;
	}

	uint8_t min_len = (o1->len < o2->len) ? o1->len : o2->len;
	for (uint8_t i = 0; i < min_len; i++) {

		if (o1->ids[i] < o2->ids[i]) {
			return -1;
		}
		if (o1->ids[i] > o2->ids[i]) {
			return 1;
		}
	}
	if (o1->len < o2->len) {
		return -1;
	}
	if (o1->len > o2->len) {
		return 1;
	}
	return 0;
}

int snmp_ber_decode_header(const uint8_t *buf, size_t buf_len, size_t *off, uint8_t *tag,
			   size_t *len)
{
	if (!buf || !off || !tag || !len || *off >= buf_len) {
		return -EINVAL;
	}

	*tag = buf[(*off)++];
	if (*off >= buf_len) {
		return -EINVAL;
	}

	uint8_t len_byte = buf[(*off)++];
	if ((len_byte & 0x80) == 0) {

		*len = len_byte;
	} else {
		/* RFC 1157 §3.2.1 & RFC 3416 §3: Indefinite length (0x80) prohibited */
		uint8_t num_bytes = len_byte & 0x7F;
		if (num_bytes == 0 || num_bytes > 4 || (*off + num_bytes) > buf_len) {

			return -EBADMSG;
		}
		*len = 0;
		for (uint8_t i = 0; i < num_bytes; i++) {
			*len = (*len << 8) | buf[(*off)++];
		}
	}

	/*
	 * Compare against the remaining length instead of computing *off + *len:
	 * a hostile 0xFFFFFFFF length would wrap size_t on 32-bit targets and the
	 * addition would look in-bounds while every later memcpy walks off the
	 * buffer. *off <= buf_len is guaranteed above, so the subtraction is safe.
	 */
	if (*len > buf_len - *off) {
		return -EBADMSG;
	}
	return 0;
}

int snmp_ber_encode_header(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, size_t len)
{
	if (!buf || !off) {
		return -EINVAL;
	}

	size_t needed = 1;
	if (len < 128) {

		needed += 1;
	} else if (len <= 0xFF) {
		needed += 2;
	} else if (len <= 0xFFFF) {
		needed += 3;
	} else {
		needed += 5;
	}

	if (*off + needed > buf_len) {
		return -ENOBUFS;
	}

	buf[(*off)++] = tag;
	if (len < 128) {
		buf[(*off)++] = (uint8_t)len;
	} else if (len <= 0xFF) {
		buf[(*off)++] = 0x81;
		buf[(*off)++] = (uint8_t)len;
	} else if (len <= 0xFFFF) {
		buf[(*off)++] = 0x82;
		buf[(*off)++] = (uint8_t)(len >> 8);
		buf[(*off)++] = (uint8_t)(len & 0xFF);
	} else {
		buf[(*off)++] = 0x84;
		buf[(*off)++] = (uint8_t)(len >> 24);
		buf[(*off)++] = (uint8_t)(len >> 16);
		buf[(*off)++] = (uint8_t)(len >> 8);
		buf[(*off)++] = (uint8_t)(len & 0xFF);
	}

	return 0;
}

int snmp_ber_decode_int(const uint8_t *buf, size_t buf_len, size_t *off, int32_t *val)
{
	uint8_t tag;
	size_t len;
	int ret = snmp_ber_decode_header(buf, buf_len, off, &tag, &len);
	if (ret < 0) {

		return ret;
	}
	if (tag != ASN1_TAG_INTEGER || len > 4 || len == 0) {
		return -EBADMSG;
	}

	int32_t res = (buf[*off] & 0x80) ? -1 : 0;
	for (size_t i = 0; i < len; i++) {

		res = (res << 8) | buf[(*off)++];
	}
	*val = res;
	return 0;
}

int snmp_ber_encode_int(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, int32_t val)
{
	uint8_t tmp[4];
	size_t len = 0;

	if (val >= -128 && val <= 127) {
		tmp[0] = (uint8_t)val;
		len = 1;
	} else if (val >= -32768 && val <= 32767) {
		tmp[0] = (uint8_t)(val >> 8);
		tmp[1] = (uint8_t)val;
		len = 2;
	} else if (val >= -8388608 && val <= 8388607) {
		tmp[0] = (uint8_t)(val >> 16);
		tmp[1] = (uint8_t)(val >> 8);
		tmp[2] = (uint8_t)val;
		len = 3;
	} else {
		tmp[0] = (uint8_t)(val >> 24);
		tmp[1] = (uint8_t)(val >> 16);
		tmp[2] = (uint8_t)(val >> 8);
		tmp[3] = (uint8_t)val;
		len = 4;
	}

	int ret = snmp_ber_encode_header(buf, buf_len, off, tag, len);
	if (ret < 0) {

		return ret;
	}
	if (*off + len > buf_len) {
		return -ENOBUFS;
	}

	memcpy(&buf[*off], tmp, len);
	*off += len;
	return 0;
}

int snmp_ber_decode_uint(const uint8_t *buf, size_t buf_len, size_t *off, uint32_t *val)
{
	uint8_t tag;
	size_t len;
	int ret = snmp_ber_decode_header(buf, buf_len, off, &tag, &len);
	if (ret < 0) {

		return ret;
	}
	if (len > 5 || len == 0) {
		return -EBADMSG;
	}

	if (len == 5) {
		if (*off >= buf_len || buf[*off] != 0x00) {
			return -EBADMSG;
		}
	}

	uint32_t res = 0;
	for (size_t i = 0; i < len; i++) {

		res = (res << 8) | buf[(*off)++];
	}
	*val = res;
	return 0;
}

int snmp_ber_encode_uint(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, uint32_t val)
{
	uint8_t tmp[5];
	size_t len = 0;

	if ((val & 0x80000000) != 0) {
		tmp[0] = 0x00;
		tmp[1] = (uint8_t)(val >> 24);
		tmp[2] = (uint8_t)(val >> 16);
		tmp[3] = (uint8_t)(val >> 8);
		tmp[4] = (uint8_t)val;
		len = 5;
	} else if (val >= 0x800000) {
		tmp[0] = (uint8_t)(val >> 24);
		tmp[1] = (uint8_t)(val >> 16);
		tmp[2] = (uint8_t)(val >> 8);
		tmp[3] = (uint8_t)val;
		len = 4;
	} else if (val >= 0x8000) {
		tmp[0] = (uint8_t)(val >> 16);
		tmp[1] = (uint8_t)(val >> 8);
		tmp[2] = (uint8_t)val;
		len = 3;
	} else if (val >= 0x80) {
		tmp[0] = (uint8_t)(val >> 8);
		tmp[1] = (uint8_t)val;
		len = 2;
	} else {
		tmp[0] = (uint8_t)val;
		len = 1;
	}

	int ret = snmp_ber_encode_header(buf, buf_len, off, tag, len);
	if (ret < 0) {

		return ret;
	}
	if (*off + len > buf_len) {
		return -ENOBUFS;
	}

	memcpy(&buf[*off], tmp, len);
	*off += len;
	return 0;
}

int snmp_ber_decode_uint64(const uint8_t *buf, size_t buf_len, size_t *off, uint64_t *val)
{
	uint8_t tag;
	size_t len;
	int ret = snmp_ber_decode_header(buf, buf_len, off, &tag, &len);
	if (ret < 0) {

		return ret;
	}
	if (len > 9 || len == 0) {
		return -EBADMSG;
	}

	/* A 9-octet unsigned integer is only legal when padded with 0x00. */
	if (len == 9 && (*off >= buf_len || buf[*off] != 0x00)) {
		return -EBADMSG;
	}

	uint64_t res = 0;
	for (size_t i = 0; i < len; i++) {

		res = (res << 8) | buf[(*off)++];
	}
	*val = res;
	return 0;
}

int snmp_ber_encode_uint64(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, uint64_t val)
{
	uint8_t tmp[9];
	size_t len = 0;

	if ((val & 0x8000000000000000ULL) != 0) {
		tmp[0] = 0x00;
		for (int i = 0; i < 8; i++) {
			tmp[1 + i] = (uint8_t)(val >> (56 - i * 8));
		}
		len = 9;
	} else {
		uint8_t raw[8];
		for (int i = 0; i < 8; i++) {

			raw[i] = (uint8_t)(val >> (56 - i * 8));
		}
		size_t start = 0;
		while (start < 7 && raw[start] == 0 && (raw[start + 1] & 0x80) == 0) {

			start++;
		}
		len = 8 - start;
		memcpy(tmp, &raw[start], len);
	}

	int ret = snmp_ber_encode_header(buf, buf_len, off, tag, len);
	if (ret < 0) {

		return ret;
	}
	if (*off + len > buf_len) {
		return -ENOBUFS;
	}

	memcpy(&buf[*off], tmp, len);
	*off += len;
	return 0;
}

int snmp_ber_decode_octet_string(const uint8_t *buf, size_t buf_len, size_t *off, uint8_t *out,
				 uint16_t max_out, uint16_t *out_len)
{
	uint8_t tag;
	size_t len;
	int ret;

	if (!buf || !off || !out) {
		return -EINVAL;
	}

	ret = snmp_ber_decode_header(buf, buf_len, off, &tag, &len);
	if (ret < 0) {
		return ret;
	}
	if (tag != ASN1_TAG_OCTET_STRING && tag != SNMP_TAG_IPADDRESS && tag != SNMP_TAG_OPAQUE) {
		return -EBADMSG;
	}

	if (len > max_out || len > buf_len - *off) {
		return -ENOBUFS;
	}
	memcpy(out, &buf[*off], len);
	*off += len;
	if (out_len) {
		*out_len = (uint16_t)len;
	}
	return 0;
}

int snmp_ber_encode_octet_string(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag,
				 const uint8_t *str, uint16_t str_len)
{
	int ret = snmp_ber_encode_header(buf, buf_len, off, tag, str_len);
	if (ret < 0) {

		return ret;
	}
	if (*off + str_len > buf_len) {
		return -ENOBUFS;
	}

	if (str && str_len > 0) {
		memcpy(&buf[*off], str, str_len);
		*off += str_len;
	}
	return 0;
}

int snmp_ber_decode_oid(const uint8_t *buf, size_t buf_len, size_t *off, struct snmp_oid *oid)
{
	uint8_t tag;
	size_t len;
	int ret = snmp_ber_decode_header(buf, buf_len, off, &tag, &len);
	if (ret < 0) {

		return ret;
	}
	if (tag != ASN1_TAG_OID || len == 0) {
		return -EBADMSG;
	}

	size_t start_off = *off;
	size_t end_off = start_off + len;

	/*
	 * The first sub-identifier is encoded like any other base-128 value and
	 * carries arcs 0/1/2 plus the second arc. Decoding it as a raw byte
	 * (first / 40) mis-parses every arc-2 OID whose second arc is >= 40,
	 * i.e. every 2.x OID outside the legacy 0/1 space.
	 */
	uint32_t first_val = 0;
	bool first_done = false;

	while (start_off < end_off) {
		uint8_t b = buf[start_off++];

		if (first_val > (UINT32_MAX >> 7)) {
			return -E2BIG;
		}
		first_val = (first_val << 7) | (b & 0x7F);
		if ((b & 0x80) == 0) {
			first_done = true;
			break;
		}
	}

	if (!first_done) {
		return -EBADMSG;
	}

	oid->len = 0;
	if (first_val < 40) {
		oid->ids[oid->len++] = 0;
		oid->ids[oid->len++] = first_val;
	} else if (first_val < 80) {
		oid->ids[oid->len++] = 1;
		oid->ids[oid->len++] = first_val - 40;
	} else {
		oid->ids[oid->len++] = 2;
		oid->ids[oid->len++] = first_val - 80;
	}

	while (start_off < end_off) {
		uint32_t val = 0;
		bool done = false;
		while (start_off < end_off) {

			uint8_t b = buf[start_off++];

			if (val > (UINT32_MAX >> 7)) {
				return -E2BIG;
			}
			val = (val << 7) | (b & 0x7F);
			if ((b & 0x80) == 0) {
				done = true;
				break;
			}
		}
		if (!done) {
			return -EBADMSG;
		}
		if (oid->len >= CONFIG_SNMP_MAX_OID_LEN) {
			return -E2BIG;
		}
		oid->ids[oid->len++] = val;
	}

	*off = end_off;
	return 0;
}

int snmp_ber_encode_oid(uint8_t *buf, size_t buf_len, size_t *off, const struct snmp_oid *oid)
{
	if (!buf || !off || !oid || oid->len < 2) {
		return -EINVAL;
	}

	if (oid->ids[0] > 2 || (oid->ids[0] < 2 && oid->ids[1] > 39)) {
		return -EINVAL;
	}
	if (oid->ids[0] == 2 && oid->ids[1] > (UINT32_MAX - 80U)) {
		return -EINVAL;
	}

	uint8_t tmp[CONFIG_SNMP_MAX_OID_LEN * 5];
	size_t tmp_len = 0;

	/* First sub-identifier (X*40 + Y) may exceed 127 for arc 2 OIDs. */
	uint32_t first_val = oid->ids[0] * 40U + oid->ids[1];
	uint8_t sub_tmp[5];
	int sub_len = 0;

	do {
		sub_tmp[sub_len++] = (uint8_t)(first_val & 0x7F);
		first_val >>= 7;
	} while (first_val > 0);

	for (int j = sub_len - 1; j >= 0; j--) {
		uint8_t b = sub_tmp[j];

		if (j > 0) {
			b |= 0x80;
		}
		tmp[tmp_len++] = b;
	}

	for (uint8_t i = 2; i < oid->len; i++) {
		uint32_t val = oid->ids[i];
		uint8_t arc_tmp[5];
		int arc_len = 0;

		do {
			arc_tmp[arc_len++] = (uint8_t)(val & 0x7F);
			val >>= 7;
		} while (val > 0);

		for (int j = arc_len - 1; j >= 0; j--) {
			uint8_t b = arc_tmp[j];

			if (j > 0) {
				b |= 0x80;
			}
			tmp[tmp_len++] = b;
		}
	}

	int ret = snmp_ber_encode_header(buf, buf_len, off, ASN1_TAG_OID, tmp_len);
	if (ret < 0) {

		return ret;
	}
	if (*off > buf_len || tmp_len > buf_len - *off) {
		return -ENOBUFS;
	}

	memcpy(&buf[*off], tmp, tmp_len);
	*off += tmp_len;
	return 0;
}

int snmp_ber_encode_null(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag)
{
	return snmp_ber_encode_header(buf, buf_len, off, tag, 0);
}

int snmp_ber_decode_pdu_body(const uint8_t *buf, size_t buf_len, size_t *in_out_off,
			     struct snmp_pdu *pdu)
{
	if (!buf || !in_out_off || !pdu) {
		return -EINVAL;
	}

	size_t off = *in_out_off;
	uint8_t tag;
	size_t len;

	int ret = snmp_ber_decode_header(buf, buf_len, &off, &pdu->type, &len);
	if (ret < 0) {

		return ret;
	}

	if (pdu->type == SNMP_PDU_TRAP_V1) {
		ret = snmp_ber_decode_oid(buf, buf_len, &off, &pdu->enterprise);
		if (ret < 0) {
			return ret;
		}

		uint16_t ip_len = 0;
		ret = snmp_ber_decode_octet_string(buf, buf_len, &off, pdu->agent_addr, 4, &ip_len);

		if (ret < 0) {
			return ret;
		}

		ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->generic_trap);
		if (ret < 0) {
			return ret;
		}

		ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->specific_trap);
		if (ret < 0) {
			return ret;
		}

		ret = snmp_ber_decode_uint(buf, buf_len, &off, &pdu->timestamp);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->request_id);
		if (ret < 0) {
			return ret;
		}

		if (pdu->type == SNMP_PDU_GET_BULK_REQ) {
			ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->non_repeaters);
			if (ret < 0) {
				return ret;
			}
			ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->max_repetitions);
			if (ret < 0) {
				return ret;
			}
		} else {
			ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->error_status);
			if (ret < 0) {
				return ret;
			}
			ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->error_index);
			if (ret < 0) {
				return ret;
			}
		}
	}

	size_t vb_seq_len;
	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &vb_seq_len);

	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {
		return -EBADMSG;
	}

	size_t vb_end = off + vb_seq_len;
	if (vb_end > buf_len) {

		return -EBADMSG;
	}

	while (off < vb_end) {
		size_t vb_len;
		uint8_t vb_tag;

		ret = snmp_ber_decode_header(buf, buf_len, &off, &vb_tag, &vb_len);
		if (ret < 0 || vb_tag != ASN1_TAG_SEQUENCE) {
			return -EBADMSG;
		}

		/*
		 * Bound every field of this varbind by its own SEQUENCE length:
		 * without this an inner length can point into the next varbind and
		 * make the decoded value disagree with the encoded structure.
		 */
		if (vb_len > vb_end - off) {
			return -EBADMSG;
		}
		size_t vb_end_inner = off + vb_len;

		if (pdu->varbind_cnt >= CONFIG_SNMP_MAX_VARBINDS) {
			/* RFC 3416: a request we cannot represent is rejected, not silently
			 * truncated (a truncated SET would acknowledge writes we did not do).
			 */
			return -E2BIG;
		}

		struct snmp_varbind *vb = &pdu->varbinds[pdu->varbind_cnt];
		ret = snmp_ber_decode_oid(buf, vb_end_inner, &off, &vb->oid);

		if (ret < 0) {
			return ret;
		}

		size_t val_start = off;
		ret = snmp_ber_decode_header(buf, vb_end_inner, &off, &vb->type, &len);

		if (ret < 0) {
			return ret;
		}
		off = val_start;

		switch (vb->type) {
		case ASN1_TAG_INTEGER:
			ret = snmp_ber_decode_int(buf, vb_end_inner, &off, &vb->val.int_val);
			break;
		case ASN1_TAG_OCTET_STRING:
		case SNMP_TAG_OPAQUE:
			ret = snmp_ber_decode_octet_string(
				buf, vb_end_inner, &off, vb->val.octet_str.data,
				sizeof(vb->val.octet_str.data), &vb->val.octet_str.len);
			break;

		case SNMP_TAG_IPADDRESS:
			ret = snmp_ber_decode_octet_string(buf, vb_end_inner, &off, vb->val.ip_addr,
							   4, NULL);
			break;
		case ASN1_TAG_NULL:
		case SNMP_TAG_EXCEPT_NO_SUCH_OBJ:
		case SNMP_TAG_EXCEPT_NO_SUCH_INST:
		case SNMP_TAG_EXCEPT_END_MIB_VIEW:
			ret = snmp_ber_decode_header(buf, vb_end_inner, &off, &tag, &len);
			if (ret == 0 && len != 0) {
				ret = -EBADMSG;
			}
			break;
		case ASN1_TAG_OID:
			ret = snmp_ber_decode_oid(buf, vb_end_inner, &off, &vb->val.oid);
			break;
		case SNMP_TAG_COUNTER32:
		case SNMP_TAG_GAUGE32:
		case SNMP_TAG_TIMETICKS:
		case SNMP_TAG_UNSIGNED32:
			ret = snmp_ber_decode_uint(buf, vb_end_inner, &off, &vb->val.uint_val);
			break;
		case SNMP_TAG_COUNTER64:
			ret = snmp_ber_decode_uint64(buf, vb_end_inner, &off, &vb->val.uint64_val);
			break;
		default:
			return -EBADMSG;
		}

		if (ret < 0) {
			return ret;
		}
		if (off != vb_end_inner) {
			return -EBADMSG;
		}

		pdu->varbind_cnt++;
	}

	if (off != vb_end) {
		return -EBADMSG;
	}

	*in_out_off = off;
	return 0;
}

int snmp_ber_decode_pdu(const uint8_t *buf, size_t buf_len, struct snmp_pdu *pdu)
{
	if (!buf || !pdu) {
		return -EINVAL;
	}

	size_t off = 0;
	uint8_t tag;
	size_t len;

	memset(pdu, 0, sizeof(*pdu));

	int ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {

		return -EBADMSG;
	}

	ret = snmp_ber_decode_int(buf, buf_len, &off, &pdu->version);
	if (ret < 0) {
		return ret;
	}

	uint16_t comm_len = 0;
	ret = snmp_ber_decode_octet_string(buf, buf_len, &off, pdu->community,

					   sizeof(pdu->community) - 1, &comm_len);
	if (ret < 0) {
		return ret;
	}
	pdu->community_len = comm_len;
	pdu->community[comm_len] = '\0';

	return snmp_ber_decode_pdu_body(buf, buf_len, &off, pdu);
}

/* ---- Reverse BER Encoder Helpers (Zero Heap, Zero Static BSS, Reentrant) ---- */

static int ber_push_bytes(uint8_t *buf, size_t *pos, const uint8_t *data, size_t len)
{
	if (*pos < len) {
		return -ENOBUFS;
	}
	*pos -= len;
	if (data && len > 0) {
		memcpy(&buf[*pos], data, len);
	}
	return 0;
}

static int ber_push_header(uint8_t *buf, size_t *pos, uint8_t tag, size_t len)
{
	uint8_t hdr[6];
	size_t hlen = 0;

	hdr[hlen++] = tag;
	if (len < 128) {
		hdr[hlen++] = (uint8_t)len;
	} else if (len <= 0xFF) {
		hdr[hlen++] = 0x81;
		hdr[hlen++] = (uint8_t)len;
	} else if (len <= 0xFFFF) {
		hdr[hlen++] = 0x82;
		hdr[hlen++] = (uint8_t)(len >> 8);
		hdr[hlen++] = (uint8_t)(len & 0xFF);
	} else {
		hdr[hlen++] = 0x84;
		hdr[hlen++] = (uint8_t)(len >> 24);
		hdr[hlen++] = (uint8_t)(len >> 16);
		hdr[hlen++] = (uint8_t)(len >> 8);
		hdr[hlen++] = (uint8_t)(len & 0xFF);
	}

	return ber_push_bytes(buf, pos, hdr, hlen);
}

static int ber_push_int(uint8_t *buf, size_t *pos, uint8_t tag, int32_t val)
{
	uint8_t tmp[4];
	size_t len = 0;

	if (val >= -128 && val <= 127) {
		tmp[0] = (uint8_t)val;
		len = 1;
	} else if (val >= -32768 && val <= 32767) {
		tmp[0] = (uint8_t)(val >> 8);
		tmp[1] = (uint8_t)val;
		len = 2;
	} else if (val >= -8388608 && val <= 8388607) {
		tmp[0] = (uint8_t)(val >> 16);
		tmp[1] = (uint8_t)(val >> 8);
		tmp[2] = (uint8_t)val;
		len = 3;
	} else {
		tmp[0] = (uint8_t)(val >> 24);
		tmp[1] = (uint8_t)(val >> 16);
		tmp[2] = (uint8_t)(val >> 8);
		tmp[3] = (uint8_t)val;
		len = 4;
	}

	int ret = ber_push_bytes(buf, pos, tmp, len);
	if (ret < 0) {

		return ret;
	}
	return ber_push_header(buf, pos, tag, len);
}

static int ber_push_uint(uint8_t *buf, size_t *pos, uint8_t tag, uint32_t val)
{
	uint8_t tmp[5];
	size_t len = 0;

	if ((val & 0x80000000) != 0) {
		tmp[0] = 0x00;
		tmp[1] = (uint8_t)(val >> 24);
		tmp[2] = (uint8_t)(val >> 16);
		tmp[3] = (uint8_t)(val >> 8);
		tmp[4] = (uint8_t)val;
		len = 5;
	} else if (val >= 0x800000) {
		tmp[0] = (uint8_t)(val >> 24);
		tmp[1] = (uint8_t)(val >> 16);
		tmp[2] = (uint8_t)(val >> 8);
		tmp[3] = (uint8_t)val;
		len = 4;
	} else if (val >= 0x8000) {
		tmp[0] = (uint8_t)(val >> 16);
		tmp[1] = (uint8_t)(val >> 8);
		tmp[2] = (uint8_t)val;
		len = 3;
	} else if (val >= 0x80) {
		tmp[0] = (uint8_t)(val >> 8);
		tmp[1] = (uint8_t)val;
		len = 2;
	} else {
		tmp[0] = (uint8_t)val;
		len = 1;
	}

	int ret = ber_push_bytes(buf, pos, tmp, len);
	if (ret < 0) {

		return ret;
	}
	return ber_push_header(buf, pos, tag, len);
}

static int ber_push_uint64(uint8_t *buf, size_t *pos, uint8_t tag, uint64_t val)
{
	uint8_t tmp[9];
	size_t len = 0;

	if ((val & 0x8000000000000000ULL) != 0) {
		tmp[0] = 0x00;
		for (int i = 0; i < 8; i++) {
			tmp[1 + i] = (uint8_t)(val >> (56 - i * 8));
		}
		len = 9;
	} else {
		int first_nonzero = -1;
		for (int i = 7; i >= 0; i--) {

			if ((val >> (i * 8)) & 0xFF) {
				first_nonzero = i;
				break;
			}
		}
		if (first_nonzero == -1) {
			tmp[0] = 0x00;
			len = 1;
		} else {
			uint8_t top_byte = (uint8_t)((val >> (first_nonzero * 8)) & 0xFF);
			if (top_byte & 0x80) {

				tmp[len++] = 0x00;
			}
			for (int i = first_nonzero; i >= 0; i--) {
				tmp[len++] = (uint8_t)((val >> (i * 8)) & 0xFF);
			}
		}
	}

	int ret = ber_push_bytes(buf, pos, tmp, len);
	if (ret < 0) {

		return ret;
	}
	return ber_push_header(buf, pos, tag, len);
}

static int ber_push_octet_string(uint8_t *buf, size_t *pos, uint8_t tag, const uint8_t *data,
				 size_t len)
{
	int ret = ber_push_bytes(buf, pos, data, len);
	if (ret < 0) {

		return ret;
	}
	return ber_push_header(buf, pos, tag, len);
}

static int ber_push_null(uint8_t *buf, size_t *pos, uint8_t tag)
{
	return ber_push_header(buf, pos, tag, 0);
}

static int ber_push_oid(uint8_t *buf, size_t *pos, const struct snmp_oid *oid)
{
	if (oid == NULL || oid->len < 2U) {
		return -EINVAL;
	}

	uint8_t tmp[CONFIG_SNMP_MAX_OID_LEN * 5];
	size_t off = 0;
	int ret = snmp_ber_encode_oid(tmp, sizeof(tmp), &off, oid);
	if (ret < 0) {

		return ret;
	}

	return ber_push_bytes(buf, pos, tmp, off);
}

static int ber_push_pdu_body(uint8_t *buf, size_t *pos, const struct snmp_pdu *pdu)
{
	size_t vb_list_end = *pos;

	for (int i = (int)pdu->varbind_cnt - 1; i >= 0; i--) {
		const struct snmp_varbind *vb = &pdu->varbinds[i];
		size_t vb_end = *pos;
		int ret = 0;

		switch (vb->type) {
		case ASN1_TAG_INTEGER:
			ret = ber_push_int(buf, pos, vb->type, vb->val.int_val);
			break;
		case ASN1_TAG_OCTET_STRING:
		case SNMP_TAG_OPAQUE:
			ret = ber_push_octet_string(buf, pos, vb->type, vb->val.octet_str.data,
						    vb->val.octet_str.len);
			break;
		case SNMP_TAG_IPADDRESS:
			ret = ber_push_octet_string(buf, pos, vb->type, vb->val.ip_addr, 4);
			break;
		case ASN1_TAG_NULL:
		case SNMP_TAG_EXCEPT_NO_SUCH_OBJ:
		case SNMP_TAG_EXCEPT_NO_SUCH_INST:
		case SNMP_TAG_EXCEPT_END_MIB_VIEW:
			ret = ber_push_null(buf, pos, vb->type);
			break;
		case ASN1_TAG_OID:
			ret = ber_push_oid(buf, pos, &vb->val.oid);
			break;
		case SNMP_TAG_COUNTER32:
		case SNMP_TAG_GAUGE32:
		case SNMP_TAG_TIMETICKS:
		case SNMP_TAG_UNSIGNED32:
			ret = ber_push_uint(buf, pos, vb->type, vb->val.uint_val);
			break;
		case SNMP_TAG_COUNTER64:
			ret = ber_push_uint64(buf, pos, vb->type, vb->val.uint64_val);
			break;
		default:
			return -EINVAL;
		}

		if (ret < 0) {
			return ret;
		}

		ret = ber_push_oid(buf, pos, &vb->oid);
		if (ret < 0) {
			return ret;
		}

		size_t vb_len = vb_end - *pos;
		ret = ber_push_header(buf, pos, ASN1_TAG_SEQUENCE, vb_len);

		if (ret < 0) {
			return ret;
		}
	}

	size_t vb_list_len = vb_list_end - *pos;
	int ret = ber_push_header(buf, pos, ASN1_TAG_SEQUENCE, vb_list_len);
	if (ret < 0) {

		return ret;
	}

	if (pdu->type == SNMP_PDU_TRAP_V1) {
		ret = ber_push_uint(buf, pos, SNMP_TAG_TIMETICKS, pdu->timestamp);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->specific_trap);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->generic_trap);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_octet_string(buf, pos, SNMP_TAG_IPADDRESS, pdu->agent_addr, 4);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_oid(buf, pos, &pdu->enterprise);
		if (ret < 0) {
			return ret;
		}
	} else if (pdu->type == SNMP_PDU_GET_BULK_REQ) {
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->max_repetitions);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->non_repeaters);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->request_id);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->error_index);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->error_status);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_int(buf, pos, ASN1_TAG_INTEGER, pdu->request_id);
		if (ret < 0) {
			return ret;
		}
	}

	size_t pdu_len = vb_list_end - *pos;
	return ber_push_header(buf, pos, pdu->type, pdu_len);
}

int snmp_ber_encode_pdu_body(uint8_t *buf, size_t buf_len, size_t *out_len,
			     const struct snmp_pdu *pdu)
{
	if (!buf || !pdu || !out_len) {
		return -EINVAL;
	}

	size_t pos = buf_len;
	int ret = ber_push_pdu_body(buf, &pos, pdu);
	if (ret < 0) {

		return ret;
	}

	size_t total_len = buf_len - pos;
	if (pos > 0) {

		memmove(buf, &buf[pos], total_len);
	}
	*out_len = total_len;
	return 0;
}

int snmp_ber_encode_pdu(uint8_t *buf, size_t buf_len, size_t *out_len, const struct snmp_pdu *pdu)
{
	if (!buf || !pdu || !out_len) {
		return -EINVAL;
	}

	size_t pos = buf_len;
	size_t msg_end = pos;

	int ret = ber_push_pdu_body(buf, &pos, pdu);
	if (ret < 0) {

		return ret;
	}

	ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING, pdu->community,
				    pdu->community_len);
	if (ret < 0) {
		return ret;
	}

	ret = ber_push_int(buf, &pos, ASN1_TAG_INTEGER, pdu->version);
	if (ret < 0) {
		return ret;
	}

	size_t msg_len = msg_end - pos;
	ret = ber_push_header(buf, &pos, ASN1_TAG_SEQUENCE, msg_len);

	if (ret < 0) {
		return ret;
	}

	size_t total_len = buf_len - pos;
	if (pos > 0) {

		memmove(buf, &buf[pos], total_len);
	}
	*out_len = total_len;
	return 0;
}

/* ---- SNMPv3 message decode / encode (RFC 3412) ---- */

/*
 * RFC 3412 section 6: msgSecurityModel 3 is USM. RFC 3412 section 8.1 sets
 * msgMaxSize's lower bound to 484 octets (a GetResponse carrying 4 varbinds).
 * A message that advertises less than that cannot be answered within its own
 * limit, so accepting it would only guarantee a truncated response.
 */
#define SNMP_BER_SEC_MODEL_USM 3
#define SNMP_BER_MIN_MAX_SIZE  484

int snmp_ber_decode_v3_msg(const uint8_t *buf, size_t buf_len, struct snmp_v3_msg *msg)
{
	if (!buf || !msg) {
		return -EINVAL;
	}

	size_t off = 0;
	uint8_t tag;
	size_t len;

	memset(msg, 0, sizeof(*msg));
	/*
	 * msgData carries an SNMPv1/v2c-style PDU: there is no version field on the
	 * wire. Mark it v3 so the MIB engine uses v2 error semantics (not the v1
	 * noSuchName path). Without this the whole v3 path silently behaved as v1.
	 */
	msg->scoped.pdu.version = 3;

	/* Outer SEQUENCE */
	int ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {

		return -EBADMSG;
	}

	/* Version = 3 */
	int32_t version = 0;
	ret = snmp_ber_decode_int(buf, buf_len, &off, &version);

	if (ret < 0 || version != 3) {
		return -EBADMSG;
	}

	/* msgGlobalData SEQUENCE */
	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {
		return -EBADMSG;
	}

	ret = snmp_ber_decode_int(buf, buf_len, &off, &msg->global.msg_id);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_ber_decode_int(buf, buf_len, &off, &msg->global.msg_max_size);
	if (ret < 0) {
		return ret;
	}
	if (msg->global.msg_max_size < SNMP_BER_MIN_MAX_SIZE) {
		return -EBADMSG;
	}

	/* msgFlags: OCTET STRING of size 1 */
	uint16_t flags_len = 0;
	uint8_t flags_byte = 0;
	ret = snmp_ber_decode_octet_string(buf, buf_len, &off, &flags_byte, 1, &flags_len);

	if (ret < 0 || flags_len != 1) {
		return -EBADMSG;
	}
	msg->global.msg_flags = flags_byte;

	ret = snmp_ber_decode_int(buf, buf_len, &off, &msg->global.msg_security_model);
	if (ret < 0) {
		return ret;
	}
	/* Only USM exists in this agent; any other model is unimplemented (RFC 3412 §7.1). */
	if (msg->global.msg_security_model != SNMP_BER_SEC_MODEL_USM) {
		return -ENOTSUP;
	}

	/*
	 * msgSecurityParameters: OCTET STRING wrapper, then decode the inner USM
	 * SEQUENCE. The wrapper value is captured separately so the absolute
	 * offset of msgAuthenticationParameters inside the *original* datagram can
	 * be recorded for RFC 3414 section 6.3.1 HMAC verification.
	 */
	uint8_t sec_tag;
	size_t sec_wrapper_len;

	uint8_t sec_raw[256];
	uint16_t sec_len = 0;
	size_t sec_val_off;

	ret = snmp_ber_decode_header(buf, buf_len, &off, &sec_tag, &sec_wrapper_len);
	if (ret < 0 || sec_tag != ASN1_TAG_OCTET_STRING) {
		return -EBADMSG;
	}
	if (sec_wrapper_len > sizeof(sec_raw) || sec_wrapper_len > buf_len - off) {
		return -ENOBUFS;
	}
	sec_val_off = off;
	memcpy(sec_raw, &buf[off], sec_wrapper_len);
	off += sec_wrapper_len;
	sec_len = (uint16_t)sec_wrapper_len;

	/* Decode USM SEQUENCE inside */
	size_t usm_off = 0;
	ret = snmp_ber_decode_header(sec_raw, sec_len, &usm_off, &tag, &len);

	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {
		return -EBADMSG;
	}

	uint16_t tmp_len;
	ret = snmp_ber_decode_octet_string(sec_raw, sec_len, &usm_off, msg->usm.engine_id,

					   SNMP_V3_ENGINE_ID_MAX_LEN, &tmp_len);
	if (ret < 0) {
		return ret;
	}
	msg->usm.engine_id_len = tmp_len;

	ret = snmp_ber_decode_int(sec_raw, sec_len, &usm_off, &msg->usm.engine_boots);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_ber_decode_int(sec_raw, sec_len, &usm_off, &msg->usm.engine_time);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_ber_decode_octet_string(sec_raw, sec_len, &usm_off, msg->usm.user_name,
					   sizeof(msg->usm.user_name) - 1, &tmp_len);
	if (ret < 0) {
		return ret;
	}
	msg->usm.user_name_len = tmp_len;
	msg->usm.user_name[tmp_len] = '\0';

	/*
	 * Decode msgAuthenticationParameters by hand: snmp_ber_decode_octet_string()
	 * consumes the header internally, and the HMAC has to be computed over the
	 * datagram as received with exactly this value zeroed.
	 */
	uint8_t auth_tag;
	size_t auth_val_len;

	ret = snmp_ber_decode_header(sec_raw, sec_len, &usm_off, &auth_tag, &auth_val_len);
	if (ret < 0 || auth_tag != ASN1_TAG_OCTET_STRING) {
		return -EBADMSG;
	}
	if (auth_val_len > SNMP_V3_AUTH_PARAMS_LEN || auth_val_len > sec_len - usm_off) {
		return -ENOBUFS;
	}
	memcpy(msg->usm.auth_params, &sec_raw[usm_off], auth_val_len);
	msg->usm.auth_params_len = (uint16_t)auth_val_len;
	msg->usm.auth_params_raw_off = sec_val_off + usm_off;
	usm_off += auth_val_len;

	ret = snmp_ber_decode_octet_string(sec_raw, sec_len, &usm_off, msg->usm.priv_params,
					   SNMP_V3_PRIV_PARAMS_LEN, &tmp_len);
	if (ret < 0) {
		return ret;
	}
	msg->usm.priv_params_len = tmp_len;

	/* msgData: ScopedPduData CHOICE { plaintext ScopedPDU, encryptedPDU OCTET STRING } */
	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0) {
		return -EBADMSG;
	}

	/* When priv flag is set, scopedPDU is a single OCTET STRING
	 * (encrypted data). Store offset/length for later decryption. */
	if ((msg->global.msg_flags & 0x02U) != 0U) {
		if (tag != ASN1_TAG_OCTET_STRING) {
			return -EBADMSG;
		}
		msg->scoped.encrypted_raw_off = off;
		msg->scoped.encrypted_raw_len = len;
		return 0;
	}

	if (tag != ASN1_TAG_SEQUENCE) {
		return -EBADMSG;
	}

	ret = snmp_ber_decode_octet_string(buf, buf_len, &off, msg->scoped.context_engine_id,
					   SNMP_V3_ENGINE_ID_MAX_LEN, &tmp_len);
	if (ret < 0) {
		return ret;
	}
	msg->scoped.context_engine_id_len = tmp_len;

	ret = snmp_ber_decode_octet_string(buf, buf_len, &off, msg->scoped.context_name,
					   sizeof(msg->scoped.context_name) - 1, &tmp_len);
	if (ret < 0) {
		return ret;
	}
	msg->scoped.context_name_len = tmp_len;
	msg->scoped.context_name[tmp_len] = '\0';

	/* Inner PDU — decode directly using snmp_ber_decode_pdu_body */
	ret = snmp_ber_decode_pdu_body(buf, buf_len, &off, &msg->scoped.pdu);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int snmp_ber_encode_v3_msg(uint8_t *buf, size_t buf_len, size_t *out_len,
			   const struct snmp_v3_msg *msg)
{
	if (!buf || !msg || !out_len) {
		return -EINVAL;
	}

	size_t pos = buf_len;
	size_t outer_end = pos;
	int ret;

	/* 1. ScopedPduData CHOICE { plaintext ScopedPDU, encryptedPDU OCTET STRING } */
	if ((msg->global.msg_flags & 0x02U) != 0U) {
		ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING,
					    msg->scoped.priv_ciphertext,
					    msg->scoped.priv_ciphertext_len);
		if (ret < 0) {
			return ret;
		}
	} else {
		size_t scoped_end = pos;

		ret = ber_push_pdu_body(buf, &pos, &msg->scoped.pdu);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING,
					    msg->scoped.context_name, msg->scoped.context_name_len);
		if (ret < 0) {
			return ret;
		}
		ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING,
					    msg->scoped.context_engine_id,
					    msg->scoped.context_engine_id_len);
		if (ret < 0) {
			return ret;
		}
		size_t scoped_len = scoped_end - pos;
		ret = ber_push_header(buf, &pos, ASN1_TAG_SEQUENCE, scoped_len);

		if (ret < 0) {
			return ret;
		}
	}

	/* 2. msgSecurityParameters (USM SEQUENCE inside OCTET STRING) */
	size_t sec_params_end = pos;
	size_t usm_end = pos;
	ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING, msg->usm.priv_params,

				    msg->usm.priv_params_len);
	if (ret < 0) {
		return ret;
	}
	ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING, msg->usm.auth_params,
				    msg->usm.auth_params_len);
	if (ret < 0) {
		return ret;
	}
	ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING,
				    (const uint8_t *)msg->usm.user_name, msg->usm.user_name_len);
	if (ret < 0) {
		return ret;
	}
	ret = ber_push_int(buf, &pos, ASN1_TAG_INTEGER, msg->usm.engine_time);
	if (ret < 0) {
		return ret;
	}
	ret = ber_push_int(buf, &pos, ASN1_TAG_INTEGER, msg->usm.engine_boots);
	if (ret < 0) {
		return ret;
	}
	ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING, msg->usm.engine_id,
				    msg->usm.engine_id_len);
	if (ret < 0) {
		return ret;
	}
	size_t usm_len = usm_end - pos;
	ret = ber_push_header(buf, &pos, ASN1_TAG_SEQUENCE, usm_len);

	if (ret < 0) {
		return ret;
	}

	size_t sec_len = sec_params_end - pos;
	ret = ber_push_header(buf, &pos, ASN1_TAG_OCTET_STRING, sec_len);

	if (ret < 0) {
		return ret;
	}

	/* 3. msgGlobalData */
	size_t gl_end = pos;
	ret = ber_push_int(buf, &pos, ASN1_TAG_INTEGER, msg->global.msg_security_model);

	if (ret < 0) {
		return ret;
	}
	uint8_t flags_byte = msg->global.msg_flags;
	ret = ber_push_octet_string(buf, &pos, ASN1_TAG_OCTET_STRING, &flags_byte, 1);

	if (ret < 0) {
		return ret;
	}
	ret = ber_push_int(buf, &pos, ASN1_TAG_INTEGER, msg->global.msg_max_size);
	if (ret < 0) {
		return ret;
	}
	ret = ber_push_int(buf, &pos, ASN1_TAG_INTEGER, msg->global.msg_id);
	if (ret < 0) {
		return ret;
	}
	size_t gl_len = gl_end - pos;
	ret = ber_push_header(buf, &pos, ASN1_TAG_SEQUENCE, gl_len);

	if (ret < 0) {
		return ret;
	}

	/* 4. SNMP Version 3 */
	ret = ber_push_int(buf, &pos, ASN1_TAG_INTEGER, 3);
	if (ret < 0) {
		return ret;
	}

	/* 5. Outer SEQUENCE */
	size_t total_payload_len = outer_end - pos;
	ret = ber_push_header(buf, &pos, ASN1_TAG_SEQUENCE, total_payload_len);

	if (ret < 0) {
		return ret;
	}

	size_t total_len = buf_len - pos;
	if (pos > 0) {

		memmove(buf, &buf[pos], total_len);
	}
	*out_len = total_len;
	return 0;
}

#endif /* CONFIG_SNMP_AGENT */
