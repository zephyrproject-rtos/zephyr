/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _DNS_PACK_H_
#define _DNS_PACK_H_

#include <misc/byteorder.h>

#include <stdint.h>
#include <stddef.h>
#include <errno.h>

/**
 * @brief dns_msg_t
 *
 * @details		Structure that points to the buffer containing the
 *			DNS message. It also contains some decodified
 *			message's properties that can not be recovered easily:
 *
 *			- cname_offset
 *
 *			- query_offset
 *
 *			- answer_offset:
 *
 *			- response_type
 *			  It indicates the response's content type. It could be
 *			  an IP address, a CNAME with IP (two answers), a CNAME
 *			  with no IP address. See enum dns_response_type for
 *			  more details.
 *
 *			- response_position: this is an offset. It holds the
 *			  starting byte of the field containing the desired
 *			  info. For example an IPv4 address.
 *
 *			- response_length: this is an offset. It holds the
 *			  response's length.
 */
struct dns_msg_t {
	uint8_t *msg;
	size_t msg_size;

	int response_type;
	int response_position;
	int response_length;

	int cname_offset;
	int query_offset;
	int answer_offset;
};

#define DNS_MSG_INIT(b, s)	{.msg = b, .msg_size = s,	\
				 .response_type = -EINVAL}


enum dns_rr_type {
	DNS_RR_TYPE_INVALID = 0,
	DNS_RR_TYPE_A	= 1,		/* IPv4  */
	DNS_RR_TYPE_CNAME = 5,		/* CNAME */
	DNS_RR_TYPE_AAAA = 28		/* IPv6  */
};

enum dns_response_type {
	DNS_RESPONSE_INVALID = -EINVAL,
	DNS_RESPONSE_IP,
	DNS_RESPONSE_CNAME_WITH_IP,
	DNS_RESPONSE_CNAME_NO_IP
};

enum dns_class {
	DNS_CLASS_INVALID = 0,
	DNS_CLASS_IN,
};

enum dns_msg_type {
	DNS_QUERY = 0,
	DNS_RESPONSE
};

enum dns_header_rcode {
	DNS_HEADER_NOERROR = 0,
	DNS_HEADER_FORMATERROR,
	DNS_HEADER_SERVERFAILURE,
	DNS_HEADER_NAMEERROR,
	DNS_HEADER_NOTIMPLEMENTED,
	DNS_HEADER_REFUSED
};

/* See 4.1.1 Header section format
 * DNS Message Header is always 12 bytes
 */
#define DNS_MSG_HEADER_SIZE	12

/** It returns the ID field in the DNS msg header	*/
static inline int dns_header_id(uint8_t *header)
{
	return sys_cpu_to_be16(*(uint16_t *)header);
}

/* inline unpack routines are used to unpack data from network
 * order to cpu. Similar routines without the unpack prefix are
 * used for cpu to network order.
 */
static inline int dns_unpack_header_id(uint8_t *header)
{
	return sys_be16_to_cpu(*(uint16_t *)header);
}

/** It returns the QR field in the DNS msg header	*/
static inline int dns_header_qr(uint8_t *header)
{
	return ((*(header + 2)) & 0x80) ? 1 : 0;
}

/** It returns the OPCODE field in the DNS msg header	*/
static inline int dns_header_opcode(uint8_t *header)
{
	return ((*(header + 2)) & 0x70) >> 1;
}

/** It returns the AA field in the DNS msg header	*/
static inline int dns_header_aa(uint8_t *header)
{
	return ((*(header + 2)) & 0x04) ? 1 : 0;
}

/** It returns the TC field in the DNS msg header	*/
static inline int dns_header_tc(uint8_t *header)
{
	return ((*(header + 2)) & 0x02) ? 1 : 0;
}

/** It returns the RD field in the DNS msg header	*/
static inline int dns_header_rd(uint8_t *header)
{
	return ((*(header + 2)) & 0x01) ? 1 : 0;
}

/** It returns the RA field in the DNS msg header	*/
static inline int dns_header_ra(uint8_t *header)
{
	return ((*(header + 3)) & 0x80) >> 7;
}

/** It returns the Z field in the DNS msg header	*/
static inline int dns_header_z(uint8_t *header)
{
	return ((*(header + 3)) & 0x70) >> 4;
}

/** It returns the RCODE field in the DNS msg header	*/
static inline int dns_header_rcode(uint8_t *header)
{
	return ((*(header + 3)) & 0x0F);
}

/** It returns the QDCOUNT field in the DNS msg header	*/
static inline int dns_header_qdcount(uint8_t *header)
{
	return sys_cpu_to_be16(*(uint16_t *)(header + 4));
}

static inline int dns_unpack_header_qdcount(uint8_t *header)
{
	return sys_be16_to_cpu(*(uint16_t *)(header + 4));
}

/** It returns the ANCOUNT field in the DNS msg header	*/
static inline int dns_header_ancount(uint8_t *header)
{
	return sys_cpu_to_be16(*(uint16_t *)(header + 6));
}

static inline int dns_unpack_header_ancount(uint8_t *header)
{
	return sys_be16_to_cpu(*(uint16_t *)(header + 6));
}

/** It returns the NSCOUNT field in the DNS msg header	*/
static inline int dns_header_nscount(uint8_t *header)
{
	return sys_cpu_to_be16(*(uint16_t *)(header + 8));
}

/** It returns the ARCOUNT field in the DNS msg header	*/
static inline int dns_header_arcount(uint8_t *header)
{
	return sys_cpu_to_be16(*(uint16_t *)(header + 10));
}

static inline int dns_query_qtype(uint8_t *question)
{
	return sys_cpu_to_be16(*((uint16_t *)(question + 0)));
}

static inline int dns_unpack_query_qtype(uint8_t *question)
{
	return sys_be16_to_cpu(*((uint16_t *)(question + 0)));
}

static inline int dns_query_qclass(uint8_t *question)
{
	return sys_cpu_to_be16(*((uint16_t *)(question + 2)));
}

static inline int dns_unpack_query_qclass(uint8_t *question)
{
	return sys_be16_to_cpu(*((uint16_t *)(question + 2)));
}

static inline int dns_response_type(int dname_size, uint8_t *answer)
{
	/** Future versions must consider byte 0
	 * 4.1.3. Resource record format
	 * *(answer + dname_size + 0);
	 */
	return *(answer + dname_size + 1);
}

static inline int dns_answer_class(int dname_size, uint8_t *answer)
{
	/** Future versions must consider byte 2
	 * 4.1.3. Resource record format
	 * *(answer + dname_size + 2);
	 */
	return *(answer + dname_size + 3);
}

static inline int dns_answer_ttl(int dname_size, uint8_t *answer)
{
	return sys_cpu_to_be32(*(uint32_t *)(answer + dname_size + 4));
}

static inline int dns_answer_rdlength(int dname_size, uint8_t *answer)
{
	return sys_cpu_to_be16(*(uint16_t *)(answer + dname_size + 8));
}

static inline int dns_unpack_answer_rdlength(int dname_size, uint8_t *answer)
{
	return sys_be16_to_cpu(*(uint16_t *)(answer + dname_size + 8));
}

/**
 * @brief dns_msg_pack_qname	Packs a QNAME
 * @param len			Bytes used by this function
 * @param buf			Buffer
 * @param sizeof		Buffer's size
 * @param domain_name		Something like www.example.com
 * @return			0 on success
 * @return			-ENOMEM if there is no enough space to store
 *				the resultant QNAME
 */
int dns_msg_pack_qname(int *len, uint8_t *buf, int size, char *domain_name);


/**
 * @brief dns_unpack_answer	Unpacks an answer message
 * @param dns_msg		Structure
 * @param dname_ptr		An index to the previous CNAME. For example
 *				for the first answer, ptr must be 0x0c, the
 *				DNAME at the question.
 * @return			0 on success
 * @return			-ENOMEM on error
 */
int dns_unpack_answer(struct dns_msg_t *dns_msg, int dname_ptr);

/**
 * @brief dns_unpack_response_header
 *
 * @details		Unpacks the header's response.
 *
 * @param msg		Structure containing the response.
 *
 * @param src_id	Transaction id, it must match the id
 *			used in the query datagram sent to the
 *			DNS server.
 * @return		0 on success
 *
 * @return		-ENOMEM if the buffer in msg has no
 *			enough space to store the header.
 *			The header is always 12 bytes length.
 *
 * @return		-EINVAL:
 *			  * if the src_id does not match the
 *			  header's id.
 *			  * if the header's QR value is
 *			  not DNS_RESPONSE.
 *			  * if the header's OPCODE value is not
 *			  DNS_QUERY.
 *			  * if the header's Z value is not 0.
 *			  * if the question counter is not 1 or
 *			  the answer counter is less than 1.
 *
 *			RFC 1035 RCODEs (> 0):
 *
 *			  1 Format error
 *			  2 Server failure
 *			  3 Name Error
 *			  4 Not Implemented
 *			  5 Refused
 *
 */
int dns_unpack_response_header(struct dns_msg_t *msg, int src_id);

/**
 * @brief dns_msg_pack_query	Packs the query message
 * @param [out] buf		Buffer that will contain the resultant query
 * @param [out] len		Number of bytes used to encode the query
 * @param [in] size		Buffer size
 * @param [in] domain_name	Something like: www.example.com
 * @param [in] id		Transaction Identifier
 * @param [in] qtype		Query type: AA, AAAA. See enum dns_rr_type
 * @return			0 on success
 * @return			On error, a negative value is returned. See:
 *				- dns_msg_pack_query_header
 *				- dns_msg_pack_qname
 */
int dns_msg_pack_query(uint8_t *buf, size_t *len, size_t size,
		       char *domain_name, uint16_t id, enum dns_rr_type qtype);

/**
 * @brief dns_unpack_response_query
 *
 * @details		Unpacks the response's query. RFC 1035 states that the
 *			response's query comes after the first 12 bytes,
 *			i.e. afther the message's header.
 *
 *			This function computes the answer_offset field.
 *
 * @param dns_msg	Structure containing the message.
 *
 * @return		0 on success
 * @return		-ENOMEM:
 *			  * if the null label is not found after
 *			  traversing the buffer.
 *			  * if QCLASS and QTYPE are not found.
 * @return		-EINVAL:
 *			  * if QTYPE is not "A" (IPv4) or "AAAA" (IPv6).
 *			  * if QCLASS is not "IN".
 *
 */
int dns_unpack_response_query(struct dns_msg_t *dns_msg);

#endif
