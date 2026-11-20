/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* BER subset used by SNMPv1 and SNMPv2c messages. Definite lengths only. */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_BER_H_
#define ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_BER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Universal types */
#define SNMP_BER_TAG_INTEGER      0x02U
#define SNMP_BER_TAG_OCTET_STRING 0x04U
#define SNMP_BER_TAG_NULL         0x05U
#define SNMP_BER_TAG_OID          0x06U
#define SNMP_BER_TAG_SEQUENCE     0x30U

/* SMI application types */
#define SNMP_BER_TAG_IPADDRESS 0x40U
#define SNMP_BER_TAG_COUNTER32 0x41U
#define SNMP_BER_TAG_GAUGE32   0x42U
#define SNMP_BER_TAG_TIMETICKS 0x43U
#define SNMP_BER_TAG_OPAQUE    0x44U
#define SNMP_BER_TAG_COUNTER64 0x46U

/* SNMPv2c exceptions, carried in place of a value */
#define SNMP_BER_TAG_NO_SUCH_OBJECT   0x80U
#define SNMP_BER_TAG_NO_SUCH_INSTANCE 0x81U
#define SNMP_BER_TAG_END_OF_MIB_VIEW  0x82U

/* PDU types */
#define SNMP_BER_TAG_GET_REQUEST      0xA0U
#define SNMP_BER_TAG_GET_NEXT_REQUEST 0xA1U
#define SNMP_BER_TAG_RESPONSE         0xA2U
#define SNMP_BER_TAG_SET_REQUEST      0xA3U
#define SNMP_BER_TAG_TRAP_V1          0xA4U
#define SNMP_BER_TAG_GET_BULK_REQUEST 0xA5U
#define SNMP_BER_TAG_INFORM_REQUEST   0xA6U
#define SNMP_BER_TAG_TRAP_V2          0xA7U
#define SNMP_BER_TAG_REPORT           0xA8U

/* Cursor over encoded data. pos never exceeds len. */
struct snmp_ber_reader {
	const uint8_t *data;
	size_t len;
	size_t pos;
};

/* Cursor over an output buffer, writing forward. */
struct snmp_ber_writer {
	uint8_t *buf;
	size_t size;
	size_t pos;
};

static inline void snmp_ber_reader_init(struct snmp_ber_reader *r, const uint8_t *data, size_t len)
{
	r->data = data;
	r->len = len;
	r->pos = 0U;
}

static inline bool snmp_ber_reader_at_end(const struct snmp_ber_reader *r)
{
	return r->pos >= r->len;
}

static inline void snmp_ber_writer_init(struct snmp_ber_writer *w, uint8_t *buf, size_t size)
{
	w->buf = buf;
	w->size = size;
	w->pos = 0U;
}

/*
 * Decoding. All functions return 0 on success and -EBADMSG on malformed or
 * truncated input; the reader position is unspecified after an error.
 *
 * The decoder is lenient where SNMP peers are known to be: a length may use
 * more octets than necessary (RFC 3417 section 8) and integer contents may
 * carry redundant leading octets, which X.690 8.3.2 forbids senders to
 * produce but which are still unambiguous. It is strict where leniency would
 * change a value: indefinite lengths, values wider than their type and
 * non-minimal sub-identifiers are rejected.
 */

/* Return the tag of the next element without consuming anything. */
int snmp_ber_peek_tag(const struct snmp_ber_reader *r, uint8_t *tag);

/* Consume tag and length; the content of *len bytes follows at r->pos. */
int snmp_ber_read_header(struct snmp_ber_reader *r, uint8_t *tag, size_t *len);

/* Consume a constructed element of the given tag. inner spans its content,
 * and r is advanced past it.
 */
int snmp_ber_read_sequence(struct snmp_ber_reader *r, uint8_t tag, struct snmp_ber_reader *inner);

/* Consume one element of any type. */
int snmp_ber_skip(struct snmp_ber_reader *r);

int snmp_ber_read_int32(struct snmp_ber_reader *r, int32_t *val);
int snmp_ber_read_uint32(struct snmp_ber_reader *r, uint8_t tag, uint32_t *val);
int snmp_ber_read_uint64(struct snmp_ber_reader *r, uint8_t tag, uint64_t *val);
/* data points into the reader's buffer. */
int snmp_ber_read_octets(struct snmp_ber_reader *r, uint8_t tag, const uint8_t **data, size_t *len);
/* Any tag whose content must be empty, such as NULL and the exceptions. */
int snmp_ber_read_empty(struct snmp_ber_reader *r, uint8_t tag);
/* -EMSGSIZE when the identifier has more than max_arcs sub-identifiers. */
int snmp_ber_read_oid(struct snmp_ber_reader *r, uint32_t *arcs, size_t max_arcs, size_t *len);

/*
 * Sizes of complete elements, header included, for computing the length of
 * enclosing constructed elements before encoding them.
 */
size_t snmp_ber_sizeof_element(size_t content_len);
/* Size of the tag and length octets alone for a content of the given size. */
size_t snmp_ber_sizeof_header(size_t content_len);
size_t snmp_ber_sizeof_int32(int32_t val);
size_t snmp_ber_sizeof_uint32(uint32_t val);
size_t snmp_ber_sizeof_uint64(uint64_t val);
size_t snmp_ber_sizeof_octets(size_t len);
size_t snmp_ber_sizeof_empty(void);
size_t snmp_ber_sizeof_oid(const uint32_t *arcs, size_t len);

/*
 * Encoding. All functions return 0 on success, -EMSGSIZE when the output
 * buffer is full and -EINVAL for a value that cannot be encoded. Nothing is
 * written on error.
 */
int snmp_ber_write_header(struct snmp_ber_writer *w, uint8_t tag, size_t content_len);
int snmp_ber_write_int32(struct snmp_ber_writer *w, int32_t val);
int snmp_ber_write_uint32(struct snmp_ber_writer *w, uint8_t tag, uint32_t val);
int snmp_ber_write_uint64(struct snmp_ber_writer *w, uint8_t tag, uint64_t val);
int snmp_ber_write_octets(struct snmp_ber_writer *w, uint8_t tag, const uint8_t *data, size_t len);
int snmp_ber_write_empty(struct snmp_ber_writer *w, uint8_t tag);
int snmp_ber_write_oid(struct snmp_ber_writer *w, const uint32_t *arcs, size_t len);

/*
 * Prepending, for wrapping already encoded content in its enclosing elements.
 * *start is the offset of the first used byte in buf and moves towards zero.
 * Returns 0 on success and -EMSGSIZE when the space before *start is too small.
 */
int snmp_ber_prepend_header(uint8_t *buf, size_t *start, uint8_t tag, size_t content_len);
int snmp_ber_prepend_int32(uint8_t *buf, size_t *start, int32_t val);
int snmp_ber_prepend_octets(uint8_t *buf, size_t *start, uint8_t tag, const uint8_t *data,
			    size_t len);

#endif /* ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_BER_H_ */
