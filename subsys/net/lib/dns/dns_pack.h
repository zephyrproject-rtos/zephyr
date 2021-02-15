/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DNS_PACK_H_
#define _DNS_PACK_H_

#include <net/net_ip.h>
#include <net/buf.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>

/* See RFC 1035, 4.1.1 Header section format
 * DNS Message Header is always 12 bytes
 */
#define DNS_MSG_HEADER_SIZE	12

/* This is the label's length octet, see 4.1.2. Question section format */
#define DNS_LABEL_LEN_SIZE	1
#define DNS_POINTER_SIZE	2
#define DNS_LABEL_MIN_SIZE	1
#define DNS_LABEL_MAX_SIZE	63
#define DNS_NAME_MAX_SIZE       255
#define DNS_ANSWER_MIN_SIZE	12
#define DNS_COMMON_UINT_SIZE	2

#define DNS_HEADER_ID_LEN	2
#define DNS_HEADER_FLAGS_LEN	2
#define DNS_QTYPE_LEN		2
#define DNS_QCLASS_LEN		2
#define DNS_QDCOUNT_LEN		2
#define DNS_ANCOUNT_LEN		2
#define DNS_NSCOUNT_LEN		2
#define DNS_ARCOUNT_LEN		2
#define DNS_TTL_LEN		4
#define DNS_RDLENGTH_LEN	2

#define NS_CMPRSFLGS    0xc0   /* DNS name compression */

/* RFC 1035 '4.1.1. Header section format' defines the following flags:
 * QR, Opcode, AA, TC, RD, RA, Z and RCODE.
 * This implementation only uses RD (Recursion Desired).
 */
#define DNS_RECURSION		1

/* These two defines represent the 3rd and 4th bytes of the DNS msg header.
 * See RFC 1035, 4.1.1. Header section format.
 */
#define DNS_FLAGS1		DNS_RECURSION	/* QR, Opcode, AA, and TC = 0 */
#define DNS_FLAGS2		0		/* RA, Z and RCODE = 0 */

/**
 * DNS message structure for DNS responses
 *
 * Structure that points to the buffer containing the DNS message. It also
 * contains some decodified message's properties that can not be recovered
 * easily:
 * - cname_offset
 * - query_offset
 * - answer_offset:
 *     + response_type: It indicates the response's content type. It could be
 *       an IP address, a CNAME with IP (two answers), a CNAME with no IP
 *       address. See enum dns_response_type for more details.
 *     + response_position: this is an offset. It holds the starting byte of
 *       the field containing the desired info. For example an IPv4 address.
 *     + response_length: this is an offset. It holds the response's length.
 */
struct dns_msg_t {
	uint8_t *msg;

	int response_type;
	uint16_t response_position;
	uint16_t response_length;

	uint16_t query_offset;
	uint16_t answer_offset;
	uint16_t msg_size;
};

#define DNS_MSG_INIT(b, s)	{.msg = b, .msg_size = s,	\
				 .response_type = -EINVAL}


enum dns_rr_type {
	DNS_RR_TYPE_INVALID = 0,
	DNS_RR_TYPE_A	= 1,		/* IPv4  */
	DNS_RR_TYPE_CNAME = 5,		/* CNAME */
	DNS_RR_TYPE_PTR = 12,		/* PTR   */
	DNS_RR_TYPE_TXT = 16,		/* TXT   */
	DNS_RR_TYPE_AAAA = 28,		/* IPv6  */
	DNS_RR_TYPE_SRV = 33,		/* SRV   */
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
	DNS_CLASS_FLUSH = BIT(15)
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

struct dns_header {
	/** Transaction ID */
	uint16_t id;
	/**
	 * | Name | Bit Position | Width | Description |
	 * |------|--------------|-------|-------------|
	 * | RCODE | 0 | 4 | Response / Error code |
	 * | CD | 4 | 1 | |
	 * | AD | 5 | 1 | Authenticated Data. 0 := Unacceptable, 1 := Acceptable |
	 * | Z | 6 | 1 | Reserved (WZ/RAZ) |
	 * | RA | 7 | 1 | Recursion Available. 0 := Unavailable, 1 := Available |
	 * | RD | 8 | 1 | Recursion Desired. 0 := No Recursion, 1 := Recursion |
	 * | TC | 9 | 1 | 0 := Not Truncated, 1 := Truncated |
	 * | AA | 10 | 1 | Answer Authenticated / Answer Authoritative. 0 := Not Authenticated, 1 := Authenticated|
	 * | Opcode | 11 | 4 | See @ref dns_opcode |
	 * | QR | 15 | 1 | 0 := Query, 1 := Response |
	 */
	uint16_t flags;
	/** Query count */
	uint16_t qdcount;
	/** Answer count */
	uint16_t ancount;
	/** Authority count */
	uint16_t nscount;
	/** Additional information count */
	uint16_t arcount;
	/** Flexible array member for records */
	uint8_t data[];
} __packed;

struct dns_query {
	uint16_t type;
	uint16_t class_;
} __packed;

struct dns_rr {
	uint16_t type;
	uint16_t class_;
	uint32_t ttl;
	uint16_t rdlength;
	uint8_t rdata[];
} __packed;

struct dns_srv_rdata {
	uint16_t priority;
	uint16_t weight;
	uint16_t port;
} __packed;

struct dns_a_rdata {
	uint32_t address;
} __packed;

struct dns_aaaa_rdata {
	uint8_t address[16];
} __packed;

/** It returns the ID field in the DNS msg header	*/
static inline int dns_header_id(uint8_t *header)
{
	return htons(UNALIGNED_GET((uint16_t *)(header)));
}

/* inline unpack routines are used to unpack data from network
 * order to cpu. Similar routines without the unpack prefix are
 * used for cpu to network order.
 */
static inline int dns_unpack_header_id(uint8_t *header)
{
	return ntohs(UNALIGNED_GET((uint16_t *)(header)));
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
	return htons(UNALIGNED_GET((uint16_t *)(header + 4)));
}

static inline int dns_unpack_header_qdcount(uint8_t *header)
{
	return ntohs(UNALIGNED_GET((uint16_t *)(header + 4)));
}

/** It returns the ANCOUNT field in the DNS msg header	*/
static inline int dns_header_ancount(uint8_t *header)
{
	return htons(UNALIGNED_GET((uint16_t *)(header + 6)));
}

static inline int dns_unpack_header_ancount(uint8_t *header)
{
	return ntohs(UNALIGNED_GET((uint16_t *)(header + 6)));
}

/** It returns the NSCOUNT field in the DNS msg header	*/
static inline int dns_header_nscount(uint8_t *header)
{
	return htons(UNALIGNED_GET((uint16_t *)(header + 8)));
}

/** It returns the ARCOUNT field in the DNS msg header	*/
static inline int dns_header_arcount(uint8_t *header)
{
	return htons(UNALIGNED_GET((uint16_t *)(header + 10)));
}

static inline int dns_query_qtype(uint8_t *question)
{
	return htons(UNALIGNED_GET((uint16_t *)(question + 0)));
}

static inline int dns_unpack_query_qtype(const uint8_t *question)
{
	return ntohs(UNALIGNED_GET((uint16_t *)(question + 0)));
}

static inline int dns_query_qclass(uint8_t *question)
{
	return htons(UNALIGNED_GET((uint16_t *)(question + 2)));
}

static inline int dns_unpack_query_qclass(const uint8_t *question)
{
	return ntohs(UNALIGNED_GET((uint16_t *)(question + 2)));
}

static inline int dns_answer_type(uint16_t dname_size, uint8_t *answer)
{
	/* 4.1.3. Resource record format */
	return ntohs(UNALIGNED_GET((uint16_t *)(answer + dname_size + 0)));
}

static inline int dns_answer_class(uint16_t dname_size, uint8_t *answer)
{
	/* 4.1.3. Resource record format */
	return ntohs(UNALIGNED_GET((uint16_t *)(answer + dname_size + 2)));
}

static inline int dns_answer_ttl(uint16_t dname_size, uint8_t *answer)
{
	return ntohl(UNALIGNED_GET((uint32_t *)(answer + dname_size + 4)));
}

static inline int dns_answer_rdlength(uint16_t dname_size,
					     uint8_t *answer)
{
	return ntohs(UNALIGNED_GET((uint16_t *)(answer + dname_size + 8)));
}

/**
 * @brief Packs a QNAME
 *
 * @param len Bytes used by this function
 * @param buf Buffer
 * @param sizeof Buffer's size
 * @param domain_name Something like www.example.com
 * @retval 0 on success
 * @retval -ENOMEM if there is no enough space to store the resultant QNAME
 * @retval -EINVAL if an invalid parameter was passed as an argument
 */
int dns_msg_pack_qname(uint16_t *len, uint8_t *buf, uint16_t size,
		       const char *domain_name);

/**
 * @brief Unpacks an answer message
 *
 * @param dns_msg Structure
 * @param dname_ptr An index to the previous CNAME. For example for the
 *        first answer, ptr must be 0x0c, the DNAME at the question.
 * @param ttl TTL answer parameter.
 * @retval 0 on success
 * @retval -ENOMEM on error
 */
int dns_unpack_answer(struct dns_msg_t *dns_msg, int dname_ptr, uint32_t *ttl);

/**
 * @brief Unpacks the header's response.
 *
 * @param msg Structure containing the response.
 * @param src_id Transaction id, it must match the id used in the query
 *        datagram sent to the DNS server.
 * @retval 0 on success
 * @retval -ENOMEM if the buffer in msg has no enough space to store the header.
 *         The header is always 12 bytes length.
 * @retval -EINVAL if the src_id does not match the header's id, or if the
 *         header's QR value is not DNS_RESPONSE or if the header's OPCODE
 *         value is not DNS_QUERY, or if the header's Z value is not 0 or if
 *         the question counter is not 1 or the answer counter is less than 1.
 * @retval RFC 1035 RCODEs (> 0) 1 Format error, 2 Server failure, 3 Name Error,
 *         4 Not Implemented and 5 Refused.
 */
int dns_unpack_response_header(struct dns_msg_t *msg, int src_id);

/**
 * @brief Packs the query message
 *
 * @param buf Buffer that will contain the resultant query
 * @param len Number of bytes used to encode the query
 * @param size Buffer size
 * @param qname Domain name represented as a sequence of labels.
 *        See RFC 1035, 4.1.2. Question section format.
 * @param qname_len Number of octets in qname.
 * @param id Transaction Identifier
 * @param qtype Query type: AA, AAAA. See enum dns_rr_type
 * @retval 0 on success
 * @retval On error, a negative value is returned.
 *         See: dns_msg_pack_query_header and  dns_msg_pack_qname.
 */
int dns_msg_pack_query(uint8_t *buf, uint16_t *len, uint16_t size,
		       uint8_t *qname, uint16_t qname_len, uint16_t id,
		       enum dns_rr_type qtype);

/**
 * @brief Unpacks the response's query.
 *
 * @details RFC 1035 states that the response's query comes after the first
 *          12 bytes i.e., after the message's header. This function computes
 *          the answer_offset field.
 *
 * @param dns_msg Structure containing the message.
 * @retval 0 on success
 * @retval -ENOMEM if the null label is not found after traversing the buffer
 *         or if QCLASS and QTYPE are not found.
 * @retval -EINVAL if QTYPE is not "A" (IPv4) or "AAAA" (IPv6) or if QCLASS
 *         is not "IN".
 */
int dns_unpack_response_query(struct dns_msg_t *dns_msg);

/**
 * @brief Copies the qname from dns_msg to buf
 *
 * @details This routine implements the algorithm described in RFC 1035, 4.1.4.
 *          Message compression to copy the qname (perhaps containing pointers
 *          with offset) to the linear buffer buf. Pointers are removed and
 *          only the "true" labels are copied.
 *
 * @param buf Output buffer
 * @param len Output buffer's length
 * @param size Output buffer's size
 * @param dns_msg Structure containing the message
 * @param pos QNAME's position in dns_msg->msg
 * @retval 0 on success
 * @retval -EINVAL if an invalid parameter was passed as an argument
 * @retval -ENOMEM if the label's size is corrupted
 */
int dns_copy_qname(uint8_t *buf, uint16_t *len, uint16_t size,
		   struct dns_msg_t *dns_msg, uint16_t pos);

/**
 * @brief Unpacks the mDNS query. This is special version for multicast DNS
 *        as it skips checks to various fields as described in RFC 6762
 *        chapter 18.
 *
 * @param msg Structure containing the response.
 * @param src_id Transaction id, this is returned to the caller.
 * @retval 0 on success, <0 if error
 * @retval -ENOMEM if the buffer in msg has no enough space to store the header.
 *         The header is always 12 bytes length.
 * @retval -EINVAL if the src_id does not match the header's id, or if the
 *         header's QR value is not DNS_RESPONSE or if the header's OPCODE
 *         value is not DNS_QUERY, or if the header's Z value is not 0 or if
 *         the question counter is not 1 or the answer counter is less than 1.
 * @retval RFC 1035 RCODEs (> 0) 1 Format error, 2 Server failure, 3 Name Error,
 *         4 Not Implemented and 5 Refused.
 */
int mdns_unpack_query_header(struct dns_msg_t *msg, uint16_t *src_id);

static inline int llmnr_unpack_query_header(struct dns_msg_t *msg,
					    uint16_t *src_id)
{
	return mdns_unpack_query_header(msg, src_id);
}

/**
 * @brief Unpacks the query.
 *
 * @param dns_msg Structure containing the message.
 * @param buf Result buf
 * @param qtype Query type is returned to caller
 * @param qclass Query class is returned to caller
 * @retval 0 on success
 * @retval -ENOMEM if the null label is not found after traversing the buffer
 *         or if QCLASS and QTYPE are not found.
 * @retval -EINVAL if QTYPE is not "A" (IPv4) or "AAAA" (IPv6) or if QCLASS
 *         is not "IN".
 */
int dns_unpack_query(struct dns_msg_t *dns_msg, struct net_buf *buf,
		     enum dns_rr_type *qtype,
		     enum dns_class *qclass);

#endif
