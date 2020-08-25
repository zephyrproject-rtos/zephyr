/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <net/buf.h>

#include "dns_pack.h"

#include "dns_internal.h"

static inline uint16_t dns_strlen(const char *str)
{
	if (str == NULL) {
		return 0;
	}
	return (uint16_t)strlen(str);
}

int dns_msg_pack_qname(uint16_t *len, uint8_t *buf, uint16_t size,
		       const char *domain_name)
{
	uint16_t dn_size;
	uint16_t lb_start;
	uint16_t lb_index;
	uint16_t lb_size;
	uint16_t i;

	lb_start = 0U;
	lb_index = 1U;
	lb_size = 0U;

	dn_size = dns_strlen(domain_name);
	if (dn_size == 0U) {
		return -EINVAL;
	}

	/* traverse the domain name str, including the null-terminator :) */
	for (i = 0U; i < dn_size + 1; i++) {
		if (lb_index >= size) {
			return -ENOMEM;
		}

		switch (domain_name[i]) {
		default:
			buf[lb_index] = domain_name[i];
			lb_size += 1U;
			break;
		case '.':
			buf[lb_start] = lb_size;
			lb_size = 0U;
			lb_start = lb_index;
			break;
		case '\0':
			buf[lb_start] = lb_size;
			buf[lb_index] = 0U;
			break;
		}
		lb_index += 1U;
	}

	*len = lb_index;

	return 0;
}

static inline void set_dns_msg_response(struct dns_msg_t *dns_msg, int type,
					uint16_t pos, uint16_t len)
{
	dns_msg->response_type = type;
	dns_msg->response_position = pos;
	dns_msg->response_length = len;
}

/*
 * Skip encoded FQDN in DNS message.
 * Returns size in bytes of encoded FQDN, or negative error code.
 */
static int skip_fqdn(uint8_t *answer, int buf_sz)
{
	int i = 0;

	while (1) {
		if (i >= buf_sz) {
			return -EINVAL;
		}

		if (answer[i] == 0) {
			i += 1;
			break;
		} else if (answer[i] >= 0xc0) {
			i += 2;
			if (i > buf_sz) {
				return -EINVAL;
			}
			break;
		} else if (answer[i] < DNS_LABEL_MAX_SIZE) {
			i += answer[i] + 1;
		} else {
			return -EINVAL;
		}
	}

	return i;
}

int dns_unpack_answer(struct dns_msg_t *dns_msg, int dname_ptr, uint32_t *ttl)
{
	int dname_len;
	uint16_t rem_size;
	uint16_t pos;
	uint16_t len;
	uint8_t *answer;

	answer = dns_msg->msg + dns_msg->answer_offset;

	dname_len = skip_fqdn(answer,
			      dns_msg->msg_size - dns_msg->answer_offset);
	if (dname_len < 0) {
		return dname_len;
	}

	/*
	 * We need to be sure this buffer has enough space
	 * to contain the answer.
	 *
	 * size: dname_size + type + class + ttl + rdlength + rdata
	 *            2     +   2  +   2   +  4  +     2    +  ?
	 *
	 * So, answer size >= 12
	 *
	 * See RFC-1035 4.1.3. Resource record format
	 */
	rem_size = dns_msg->msg_size - dname_len;
	if (rem_size < 2 + 2 + 4 + 2) {
		return -EINVAL;
	}

	/* Only DNS_CLASS_IN answers. If mDNS is enabled, strip away the
	 * Cache-Flush bit (highest one).
	 */
	if ((dns_answer_class(dname_len, answer) &
	     (IS_ENABLED(CONFIG_MDNS_RESOLVER) ? 0x7fff : 0xffff))
							!= DNS_CLASS_IN) {
		return -EINVAL;
	}

	/* TTL value */
	*ttl = dns_answer_ttl(dname_len, answer);
	len = dns_answer_rdlength(dname_len, answer);
	pos = dns_msg->answer_offset + dname_len +
		DNS_COMMON_UINT_SIZE + /* class length */
		DNS_COMMON_UINT_SIZE + /* type length */
		DNS_TTL_LEN +
		DNS_RDLENGTH_LEN;

	switch (dns_answer_type(dname_len, answer)) {
	case DNS_RR_TYPE_A:
	case DNS_RR_TYPE_AAAA:
		set_dns_msg_response(dns_msg, DNS_RESPONSE_IP, pos, len);
		return 0;

	case DNS_RR_TYPE_CNAME:
		set_dns_msg_response(dns_msg, DNS_RESPONSE_CNAME_NO_IP,
				     pos, len);
		return 0;

	default:
		/* malformed dns answer */
		return -EINVAL;
	}

	return 0;
}

int dns_unpack_response_header(struct dns_msg_t *msg, int src_id)
{
	uint8_t *dns_header;
	uint16_t size;
	int qdcount;
	int ancount;
	int rc;

	dns_header = msg->msg;
	size = msg->msg_size;

	if (size < DNS_MSG_HEADER_SIZE) {
		return -ENOMEM;
	}

	if (dns_unpack_header_id(dns_header) != src_id) {
		return -EINVAL;
	}

	if (dns_header_qr(dns_header) != DNS_RESPONSE) {
		return -EINVAL;
	}

	if (dns_header_opcode(dns_header) != DNS_QUERY) {
		return -EINVAL;
	}

	if (dns_header_z(dns_header) != 0) {
		return -EINVAL;
	}

	rc = dns_header_rcode(dns_header);
	switch (rc) {
	case DNS_HEADER_NOERROR:
		break;
	default:
		return rc;

	}

	qdcount = dns_unpack_header_qdcount(dns_header);
	ancount = dns_unpack_header_ancount(dns_header);

	/* For mDNS (when src_id == 0) the query count is 0 so accept
	 * the packet in that case.
	 */
	if ((qdcount < 1 && src_id > 0) || ancount < 1) {
		return -EINVAL;
	}

	return 0;
}

static int dns_msg_pack_query_header(uint8_t *buf, uint16_t size, uint16_t id)
{
	uint16_t offset;

	if (size < DNS_MSG_HEADER_SIZE) {
		return -ENOMEM;
	}

	UNALIGNED_PUT(htons(id), (uint16_t *)(buf));

	/* RD = 1, TC = 0, AA = 0, Opcode = 0, QR = 0 <-> 0x01 (1B)
	 * RCode = 0, Z = 0, RA = 0		      <-> 0x00 (1B)
	 *
	 * QDCOUNT = 1				      <-> 0x0001 (2B)
	 */

	offset = DNS_HEADER_ID_LEN;
	/* Split the following assignements just in case we need to alter
	 * the flags in future releases
	 */
	*(buf + offset) = DNS_FLAGS1;		/* QR, Opcode, AA, TC and RD */
	*(buf + offset + 1) = DNS_FLAGS2;	/* RA, Z and RCODE */

	offset += DNS_HEADER_FLAGS_LEN;
	/* set question counter */
	UNALIGNED_PUT(htons(1), (uint16_t *)(buf + offset));

	offset += DNS_QDCOUNT_LEN;
	/* set answer and ns rr */
	UNALIGNED_PUT(0, (uint32_t *)(buf + offset));

	offset += DNS_ANCOUNT_LEN + DNS_NSCOUNT_LEN;
	/* set the additional records */
	UNALIGNED_PUT(0, (uint16_t *)(buf + offset));

	return 0;
}

int dns_msg_pack_query(uint8_t *buf, uint16_t *len, uint16_t size,
		       uint8_t *qname, uint16_t qname_len, uint16_t id,
		       enum dns_rr_type qtype)
{
	uint16_t msg_size;
	uint16_t offset;
	int rc;

	msg_size = DNS_MSG_HEADER_SIZE + DNS_QTYPE_LEN + DNS_QCLASS_LEN;
	if (msg_size + qname_len > size) {
		return -ENOMEM;
	}

	rc = dns_msg_pack_query_header(buf, size, id);
	if (rc != 0) {
		return rc;
	}

	offset = DNS_MSG_HEADER_SIZE;
	memcpy(buf + offset, qname, qname_len);

	offset += qname_len;

	/* QType */
	UNALIGNED_PUT(htons(qtype), (uint16_t *)(buf + offset + 0));
	offset += DNS_QTYPE_LEN;

	/* QClass */
	UNALIGNED_PUT(htons(DNS_CLASS_IN), (uint16_t *)(buf + offset));

	*len = offset + DNS_QCLASS_LEN;

	return 0;
}

static int dns_find_null(int *qname_size, uint8_t *buf, uint16_t size)
{
	*qname_size = 0;
	while (*qname_size < size) {
		if (buf[(*qname_size)++] == 0x00) {
			return 0;
		}
	}

	return -ENOMEM;
}

int dns_unpack_response_query(struct dns_msg_t *dns_msg)
{
	uint8_t *dns_query;
	uint8_t *buf;
	int remaining_size;
	int qname_size;
	int offset;
	int rc;

	dns_msg->query_offset = DNS_MSG_HEADER_SIZE;
	dns_query = dns_msg->msg + dns_msg->query_offset;
	remaining_size = dns_msg->msg_size - dns_msg->query_offset;

	rc = dns_find_null(&qname_size, dns_query, remaining_size);
	if (rc != 0) {
		return rc;
	}

	/* header already parsed + qname size */
	offset = dns_msg->query_offset + qname_size;

	/* 4 bytes more due to qtype and qclass */
	offset += DNS_QTYPE_LEN + DNS_QCLASS_LEN;
	if (offset >= dns_msg->msg_size) {
		return -ENOMEM;
	}

	buf = dns_query + qname_size;
	if (dns_unpack_query_qtype(buf) != DNS_RR_TYPE_A &&
	    dns_unpack_query_qtype(buf) != DNS_RR_TYPE_AAAA) {
		return -EINVAL;
	}

	if (dns_unpack_query_qclass(buf) != DNS_CLASS_IN) {
		return -EINVAL;
	}

	dns_msg->answer_offset = dns_msg->query_offset + qname_size +
				 DNS_QTYPE_LEN + DNS_QCLASS_LEN;

	return 0;
}

int dns_copy_qname(uint8_t *buf, uint16_t *len, uint16_t size,
		   struct dns_msg_t *dns_msg, uint16_t pos)
{
	uint16_t msg_size = dns_msg->msg_size;
	uint8_t *msg = dns_msg->msg;
	uint16_t lb_size;
	int rc = -EINVAL;

	*len = 0U;

	while (1) {
		if (pos >= msg_size) {
			rc = -ENOMEM;
			break;
		}

		lb_size = msg[pos];

		/* pointer */
		if (lb_size > DNS_LABEL_MAX_SIZE) {
			uint8_t mask = DNS_LABEL_MAX_SIZE;

			if (pos + 1 >= msg_size) {
				rc = -ENOMEM;
				break;
			}

			/* See: RFC 1035, 4.1.4. Message compression */
			pos = ((msg[pos] & mask) << 8) + msg[pos + 1];

			continue;
		}

		/* validate that the label (i.e. size + elements),
		 * fits the current msg buffer
		 */
		if (DNS_LABEL_LEN_SIZE + lb_size > size - *len) {
			rc = -ENOMEM;
			break;
		}

		/* copy the lb_size value and label elements */
		memcpy(buf + *len, msg + pos, DNS_LABEL_LEN_SIZE + lb_size);
		/* update destination buffer len */
		*len += DNS_LABEL_LEN_SIZE + lb_size;
		/* update msg ptr position */
		pos += DNS_LABEL_LEN_SIZE + lb_size;

		/* The domain name terminates with the zero length octet
		 * for the null label of the root
		 */
		if (lb_size == 0U) {
			rc = 0;
			break;
		}
	}

	return rc;
}

int mdns_unpack_query_header(struct dns_msg_t *msg, uint16_t *src_id)
{
	uint8_t *dns_header;
	uint16_t size;
	int qdcount;

	dns_header = msg->msg;
	size = msg->msg_size;

	if (size < DNS_MSG_HEADER_SIZE) {
		return -ENOMEM;
	}

	if (dns_header_qr(dns_header) != DNS_QUERY) {
		return -EINVAL;
	}

	if (dns_header_opcode(dns_header) != DNS_QUERY) {
		return -EINVAL;
	}

	if (dns_header_opcode(dns_header) != 0) {
		return -EINVAL;
	}

	if (dns_header_rcode(dns_header) != 0) {
		return -EINVAL;
	}

	qdcount = dns_unpack_header_qdcount(dns_header);
	if (qdcount < 1) {
		return -EINVAL;
	}

	if (src_id) {
		*src_id = dns_unpack_header_id(dns_header);
	}

	msg->query_offset = DNS_MSG_HEADER_SIZE;

	return qdcount;
}

/* Returns the length of the unpacked name */
static int dns_unpack_name(const uint8_t *msg, int maxlen, const uint8_t *src,
			   struct net_buf *buf, const uint8_t **eol)
{
	int dest_size = net_buf_tailroom(buf);
	const uint8_t *end_of_label = NULL;
	const uint8_t *curr_src = src;
	int loop_check = 0, len = -1;
	int label_len;
	int val;

	if (curr_src < msg || curr_src >= (msg + maxlen)) {
		return -EMSGSIZE;
	}

	while ((val = *curr_src++)) {
		if (val & NS_CMPRSFLGS) {
			/* Follow pointer */
			int pos;

			if (curr_src >= (msg + maxlen)) {
				return -EMSGSIZE;
			}

			if (len < 0) {
				len = curr_src - src + 1;
			}

			end_of_label = curr_src + 1;

			/* Strip compress bits from length calculation */
			pos = ((val & 0x3f) << 8) | (*curr_src & 0xff);

			curr_src = msg + pos;
			if (curr_src >= (msg + maxlen)) {
				return -EMSGSIZE;
			}

			loop_check += 2;
			if (loop_check >= maxlen) {
				return -EMSGSIZE;
			}
		} else {
			/* Max label length is 64 bytes (because 2 bits are
			 * used for pointer)
			 */
			label_len = val;
			if (label_len > 63) {
				return -EMSGSIZE;
			}

			if (((buf->data + label_len + 1) >=
			     (buf->data + dest_size)) ||
			    ((curr_src + label_len) >= (msg + maxlen))) {
				return -EMSGSIZE;
			}

			loop_check += label_len + 1;

			net_buf_add_u8(buf, '.');
			net_buf_add_mem(buf, curr_src, label_len);

			curr_src += label_len;
		}
	}

	buf->data[buf->len] = '\0';

	if (eol) {
		if (!end_of_label) {
			end_of_label = curr_src;
		}

		*eol = end_of_label;
	}

	return buf->len;
}

int dns_unpack_query(struct dns_msg_t *dns_msg, struct net_buf *buf,
		     enum dns_rr_type *qtype, enum dns_class *qclass)
{
	const uint8_t *end_of_label;
	uint8_t *dns_query;
	int remaining_size;
	int ret;
	int query_type, query_class;

	dns_query = dns_msg->msg + dns_msg->query_offset;
	remaining_size = dns_msg->msg_size - dns_msg->query_offset;

	ret = dns_unpack_name(dns_msg->msg, dns_msg->msg_size, dns_query,
			      buf, &end_of_label);
	if (ret < 0) {
		return ret;
	}

	query_type = dns_unpack_query_qtype(end_of_label);
	if (query_type != DNS_RR_TYPE_A && query_type != DNS_RR_TYPE_AAAA) {
		return -EINVAL;
	}

	query_class = dns_unpack_query_qclass(end_of_label);
	if (query_class != DNS_CLASS_IN) {
		return -EINVAL;
	}

	if (qtype) {
		*qtype = query_type;
	}

	if (qclass) {
		*qclass = query_class;
	}

	dns_msg->query_offset = end_of_label - dns_msg->msg + 2 + 2;

	return ret;
}
