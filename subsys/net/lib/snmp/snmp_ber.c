/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "snmp_ber.h"

/* Content bytes of the largest integer the codec handles: Counter64 with a
 * leading zero byte.
 */
#define INT_CONTENT_MAX_LEN 9U

static size_t length_field_len(size_t len)
{
	if (len < 0x80U) {
		return 1U;
	}

	if (len <= 0xFFU) {
		return 2U;
	}

	if (len <= 0xFFFFU) {
		return 3U;
	}

	if (len <= 0xFFFFFFU) {
		return 4U;
	}

	return 5U;
}

/* out must hold at least five bytes. Returns the number of bytes written. */
static size_t encode_length(uint8_t *out, size_t len)
{
	size_t n = length_field_len(len);

	if (n == 1U) {
		out[0] = (uint8_t)len;
		return 1U;
	}

	out[0] = 0x80U | (uint8_t)(n - 1U);
	for (size_t i = 1U; i < n; i++) {
		out[i] = (uint8_t)(len >> (8U * (n - 1U - i)));
	}

	return n;
}

/* Minimal two's complement content of a signed value. out must hold eight
 * bytes. Returns the number of bytes written.
 */
static size_t encode_signed(uint8_t *out, int64_t val)
{
	uint64_t u = (uint64_t)val;
	size_t n = sizeof(u);

	/* Drop leading bytes that only repeat the sign bit. */
	while (n > 1U) {
		uint8_t hi = (uint8_t)(u >> 56U);
		uint8_t next = (uint8_t)(u >> 48U);

		if ((hi == 0x00U && (next & 0x80U) == 0U) ||
		    (hi == 0xFFU && (next & 0x80U) != 0U)) {
			u <<= 8U;
			n--;
		} else {
			break;
		}
	}

	for (size_t i = 0U; i < n; i++) {
		out[i] = (uint8_t)(u >> (56U - 8U * i));
	}

	return n;
}

/* Minimal content of a non-negative value, with a zero byte prepended when
 * the top bit would otherwise read as a sign. out must hold nine bytes.
 */
static size_t encode_unsigned(uint8_t *out, uint64_t val)
{
	size_t bytes = 1U;
	size_t n = 0U;

	while (bytes < sizeof(val) && (val >> (8U * bytes)) != 0U) {
		bytes++;
	}

	if (((val >> (8U * (bytes - 1U))) & 0x80U) != 0U) {
		out[n++] = 0x00U;
	}

	for (size_t i = 0U; i < bytes; i++) {
		out[n++] = (uint8_t)(val >> (8U * (bytes - 1U - i)));
	}

	return n;
}

/* max_len is the width of the value's type. */
static int decode_signed(const uint8_t *p, size_t len, size_t max_len, int64_t *val)
{
	uint64_t u;

	if (len == 0U) {
		return -EBADMSG;
	}

	/* Drop leading octets that only repeat the sign bit. */
	while (len > 1U && ((p[0] == 0x00U && (p[1] & 0x80U) == 0U) ||
			    (p[0] == 0xFFU && (p[1] & 0x80U) != 0U))) {
		p++;
		len--;
	}

	if (len > max_len) {
		return -EBADMSG;
	}

	u = ((p[0] & 0x80U) != 0U) ? UINT64_MAX : 0U;
	for (size_t i = 0U; i < len; i++) {
		u = (u << 8U) | p[i];
	}

	*val = (int64_t)u;

	return 0;
}

/* max_len is the width of the value's type. */
static int decode_unsigned(const uint8_t *p, size_t len, size_t max_len, uint64_t *val)
{
	uint64_t u = 0U;

	if (len == 0U) {
		return -EBADMSG;
	}

	/* A negative number is not a valid unsigned value. */
	if ((p[0] & 0x80U) != 0U) {
		return -EBADMSG;
	}

	/* Drop leading zero octets, including the one a set top bit requires. */
	while (len > 1U && p[0] == 0x00U) {
		p++;
		len--;
	}

	if (len > max_len) {
		return -EBADMSG;
	}

	for (size_t i = 0U; i < len; i++) {
		u = (u << 8U) | p[i];
	}

	*val = u;

	return 0;
}

static size_t subid_len(uint32_t v)
{
	size_t n = 1U;

	while (v >= 0x80U) {
		v >>= 7U;
		n++;
	}

	return n;
}

static size_t encode_subid(uint8_t *out, uint32_t v)
{
	size_t n = subid_len(v);

	for (size_t i = 0U; i < n; i++) {
		uint8_t b = (uint8_t)((v >> (7U * (n - 1U - i))) & 0x7FU);

		out[i] = (i + 1U < n) ? (b | 0x80U) : b;
	}

	return n;
}

/* The first two sub-identifiers share one encoded value. */
static int first_subid(const uint32_t *arcs, uint32_t *v)
{
	if (arcs[0] > 2U) {
		return -EINVAL;
	}

	if (arcs[0] < 2U && arcs[1] >= 40U) {
		return -EINVAL;
	}

	if (arcs[0] == 2U && arcs[1] > UINT32_MAX - 80U) {
		return -EINVAL;
	}

	*v = arcs[0] * 40U + arcs[1];

	return 0;
}

int snmp_ber_peek_tag(const struct snmp_ber_reader *r, uint8_t *tag)
{
	if (r->pos >= r->len) {
		return -EBADMSG;
	}

	*tag = r->data[r->pos];

	return 0;
}

int snmp_ber_read_header(struct snmp_ber_reader *r, uint8_t *tag, size_t *len)
{
	size_t content_len;
	uint8_t b;

	if (r->len - r->pos < 2U) {
		return -EBADMSG;
	}

	*tag = r->data[r->pos++];

	/* Multi-byte tags do not occur in SNMP. */
	if ((*tag & 0x1FU) == 0x1FU) {
		return -EBADMSG;
	}

	b = r->data[r->pos++];
	if (b < 0x80U) {
		content_len = b;
	} else {
		size_t n = b & 0x7FU;

		/* Indefinite form (0x80) and the reserved value 0xFF. */
		if (n == 0U || n == 0x7FU) {
			return -EBADMSG;
		}

		if (r->len - r->pos < n) {
			return -EBADMSG;
		}

		/* Any number of length octets is allowed, as long as the
		 * significant ones fit the type.
		 */
		content_len = 0U;
		for (size_t i = 0U; i < n; i++) {
			if (content_len > (SIZE_MAX >> 8U)) {
				return -EBADMSG;
			}

			content_len = (content_len << 8U) | r->data[r->pos++];
		}
	}

	if (content_len > r->len - r->pos) {
		return -EBADMSG;
	}

	*len = content_len;

	return 0;
}

int snmp_ber_read_sequence(struct snmp_ber_reader *r, uint8_t tag, struct snmp_ber_reader *inner)
{
	uint8_t got;
	size_t len;
	int ret;

	ret = snmp_ber_read_header(r, &got, &len);
	if (ret != 0) {
		return ret;
	}

	if (got != tag) {
		return -EBADMSG;
	}

	snmp_ber_reader_init(inner, &r->data[r->pos], len);
	r->pos += len;

	return 0;
}

int snmp_ber_skip(struct snmp_ber_reader *r)
{
	uint8_t tag;
	size_t len;
	int ret;

	ret = snmp_ber_read_header(r, &tag, &len);
	if (ret != 0) {
		return ret;
	}

	r->pos += len;

	return 0;
}

static int read_content(struct snmp_ber_reader *r, uint8_t tag, const uint8_t **data, size_t *len)
{
	uint8_t got;
	int ret;

	ret = snmp_ber_read_header(r, &got, len);
	if (ret != 0) {
		return ret;
	}

	if (got != tag) {
		return -EBADMSG;
	}

	*data = &r->data[r->pos];
	r->pos += *len;

	return 0;
}

int snmp_ber_read_int32(struct snmp_ber_reader *r, int32_t *val)
{
	const uint8_t *p;
	size_t len;
	int64_t v;
	int ret;

	ret = read_content(r, SNMP_BER_TAG_INTEGER, &p, &len);
	if (ret != 0) {
		return ret;
	}

	ret = decode_signed(p, len, sizeof(*val), &v);
	if (ret != 0) {
		return ret;
	}

	*val = (int32_t)v;

	return 0;
}

int snmp_ber_read_uint32(struct snmp_ber_reader *r, uint8_t tag, uint32_t *val)
{
	const uint8_t *p;
	size_t len;
	uint64_t v;
	int ret;

	ret = read_content(r, tag, &p, &len);
	if (ret != 0) {
		return ret;
	}

	ret = decode_unsigned(p, len, sizeof(*val), &v);
	if (ret != 0) {
		return ret;
	}

	*val = (uint32_t)v;

	return 0;
}

int snmp_ber_read_uint64(struct snmp_ber_reader *r, uint8_t tag, uint64_t *val)
{
	const uint8_t *p;
	size_t len;
	int ret;

	ret = read_content(r, tag, &p, &len);
	if (ret != 0) {
		return ret;
	}

	return decode_unsigned(p, len, sizeof(*val), val);
}

int snmp_ber_read_octets(struct snmp_ber_reader *r, uint8_t tag, const uint8_t **data, size_t *len)
{
	return read_content(r, tag, data, len);
}

int snmp_ber_read_empty(struct snmp_ber_reader *r, uint8_t tag)
{
	const uint8_t *p;
	size_t len;
	int ret;

	ret = read_content(r, tag, &p, &len);
	if (ret != 0) {
		return ret;
	}

	return (len == 0U) ? 0 : -EBADMSG;
}

int snmp_ber_read_oid(struct snmp_ber_reader *r, uint32_t *arcs, size_t max_arcs, size_t *len)
{
	const uint8_t *p;
	size_t content_len;
	size_t count = 0U;
	size_t nbytes = 0U;
	uint32_t v = 0U;
	bool first = true;
	int ret;

	ret = read_content(r, SNMP_BER_TAG_OID, &p, &content_len);
	if (ret != 0) {
		return ret;
	}

	/* Every identifier has at least two sub-identifiers. */
	if (content_len == 0U) {
		return -EBADMSG;
	}

	for (size_t i = 0U; i < content_len; i++) {
		uint8_t b = p[i];

		/* A leading 0x80 byte is a non-minimal encoding. */
		if (nbytes == 0U && b == 0x80U) {
			return -EBADMSG;
		}

		if (v > (UINT32_MAX >> 7U)) {
			return -EBADMSG;
		}

		v = (v << 7U) | (b & 0x7FU);
		nbytes++;

		if ((b & 0x80U) != 0U) {
			continue;
		}

		if (first) {
			first = false;

			if (max_arcs < 2U) {
				return -EMSGSIZE;
			}

			if (v < 40U) {
				arcs[0] = 0U;
				arcs[1] = v;
			} else if (v < 80U) {
				arcs[0] = 1U;
				arcs[1] = v - 40U;
			} else {
				arcs[0] = 2U;
				arcs[1] = v - 80U;
			}

			count = 2U;
		} else {
			if (count >= max_arcs) {
				return -EMSGSIZE;
			}

			arcs[count++] = v;
		}

		v = 0U;
		nbytes = 0U;
	}

	/* The last sub-identifier had its continuation bit set. */
	if (nbytes != 0U) {
		return -EBADMSG;
	}

	*len = count;

	return 0;
}

size_t snmp_ber_sizeof_element(size_t content_len)
{
	return 1U + length_field_len(content_len) + content_len;
}

size_t snmp_ber_sizeof_header(size_t content_len)
{
	return 1U + length_field_len(content_len);
}

size_t snmp_ber_sizeof_int32(int32_t val)
{
	uint8_t tmp[sizeof(int64_t)];

	return snmp_ber_sizeof_element(encode_signed(tmp, val));
}

size_t snmp_ber_sizeof_uint32(uint32_t val)
{
	uint8_t tmp[INT_CONTENT_MAX_LEN];

	return snmp_ber_sizeof_element(encode_unsigned(tmp, val));
}

size_t snmp_ber_sizeof_uint64(uint64_t val)
{
	uint8_t tmp[INT_CONTENT_MAX_LEN];

	return snmp_ber_sizeof_element(encode_unsigned(tmp, val));
}

size_t snmp_ber_sizeof_octets(size_t len)
{
	return snmp_ber_sizeof_element(len);
}

size_t snmp_ber_sizeof_empty(void)
{
	return snmp_ber_sizeof_element(0U);
}

static size_t oid_content_len(const uint32_t *arcs, size_t len)
{
	uint32_t v;
	size_t n;

	if (len < 2U || first_subid(arcs, &v) != 0) {
		return 0U;
	}

	n = subid_len(v);
	for (size_t i = 2U; i < len; i++) {
		n += subid_len(arcs[i]);
	}

	return n;
}

size_t snmp_ber_sizeof_oid(const uint32_t *arcs, size_t len)
{
	size_t n = oid_content_len(arcs, len);

	return (n == 0U) ? 0U : snmp_ber_sizeof_element(n);
}

int snmp_ber_write_header(struct snmp_ber_writer *w, uint8_t tag, size_t content_len)
{
	size_t n = 1U + length_field_len(content_len);

	if (w->size - w->pos < n) {
		return -EMSGSIZE;
	}

	w->buf[w->pos] = tag;
	(void)encode_length(&w->buf[w->pos + 1U], content_len);
	w->pos += n;

	return 0;
}

static int write_element(struct snmp_ber_writer *w, uint8_t tag, const uint8_t *content,
			 size_t content_len)
{
	size_t n = snmp_ber_sizeof_element(content_len);
	int ret;

	if (w->size - w->pos < n) {
		return -EMSGSIZE;
	}

	ret = snmp_ber_write_header(w, tag, content_len);
	if (ret != 0) {
		return ret;
	}

	if (content_len > 0U) {
		memcpy(&w->buf[w->pos], content, content_len);
		w->pos += content_len;
	}

	return 0;
}

int snmp_ber_write_int32(struct snmp_ber_writer *w, int32_t val)
{
	uint8_t tmp[sizeof(int64_t)];
	size_t n = encode_signed(tmp, val);

	return write_element(w, SNMP_BER_TAG_INTEGER, tmp, n);
}

int snmp_ber_write_uint32(struct snmp_ber_writer *w, uint8_t tag, uint32_t val)
{
	uint8_t tmp[INT_CONTENT_MAX_LEN];
	size_t n = encode_unsigned(tmp, val);

	return write_element(w, tag, tmp, n);
}

int snmp_ber_write_uint64(struct snmp_ber_writer *w, uint8_t tag, uint64_t val)
{
	uint8_t tmp[INT_CONTENT_MAX_LEN];
	size_t n = encode_unsigned(tmp, val);

	return write_element(w, tag, tmp, n);
}

int snmp_ber_write_octets(struct snmp_ber_writer *w, uint8_t tag, const uint8_t *data, size_t len)
{
	return write_element(w, tag, data, len);
}

int snmp_ber_write_empty(struct snmp_ber_writer *w, uint8_t tag)
{
	return write_element(w, tag, NULL, 0U);
}

int snmp_ber_write_oid(struct snmp_ber_writer *w, const uint32_t *arcs, size_t len)
{
	size_t content_len = oid_content_len(arcs, len);
	uint32_t v;
	int ret;

	if (content_len == 0U) {
		return -EINVAL;
	}

	if (w->size - w->pos < snmp_ber_sizeof_element(content_len)) {
		return -EMSGSIZE;
	}

	ret = snmp_ber_write_header(w, SNMP_BER_TAG_OID, content_len);
	if (ret != 0) {
		return ret;
	}

	(void)first_subid(arcs, &v);
	w->pos += encode_subid(&w->buf[w->pos], v);
	for (size_t i = 2U; i < len; i++) {
		w->pos += encode_subid(&w->buf[w->pos], arcs[i]);
	}

	return 0;
}

int snmp_ber_prepend_header(uint8_t *buf, size_t *start, uint8_t tag, size_t content_len)
{
	size_t n = 1U + length_field_len(content_len);

	if (*start < n) {
		return -EMSGSIZE;
	}

	*start -= n;
	buf[*start] = tag;
	(void)encode_length(&buf[*start + 1U], content_len);

	return 0;
}

static int prepend_element(uint8_t *buf, size_t *start, uint8_t tag, const uint8_t *content,
			   size_t content_len)
{
	if (*start < snmp_ber_sizeof_element(content_len)) {
		return -EMSGSIZE;
	}

	if (content_len > 0U) {
		*start -= content_len;
		memcpy(&buf[*start], content, content_len);
	}

	return snmp_ber_prepend_header(buf, start, tag, content_len);
}

int snmp_ber_prepend_int32(uint8_t *buf, size_t *start, int32_t val)
{
	uint8_t tmp[sizeof(int64_t)];
	size_t n = encode_signed(tmp, val);

	return prepend_element(buf, start, SNMP_BER_TAG_INTEGER, tmp, n);
}

int snmp_ber_prepend_octets(uint8_t *buf, size_t *start, uint8_t tag, const uint8_t *data,
			    size_t len)
{
	return prepend_element(buf, start, tag, data, len);
}
