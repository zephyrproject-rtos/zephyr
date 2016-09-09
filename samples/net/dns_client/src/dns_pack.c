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

#include "dns_pack.h"

#include <string.h>

static inline size_t dns_strlen(char *str)
{
	if (str == NULL) {
		return 0;
	}
	return strlen(str);
}

int dns_msg_pack_qname(int *len, uint8_t *buf, int size, char *domain_name)
{
	int lb_index;
	int lb_start;
	int lb_size;
	size_t i;

	lb_start = 0;
	lb_index = 1;
	lb_size = 0;

	/* traverse the domain name str, including the null-terminator :) */
	for (i = 0; i < dns_strlen(domain_name) + 1; i++) {
		if (lb_index >= size) {
			return -ENOMEM;
		}

		switch (domain_name[i]) {
		default:
			buf[lb_index] = domain_name[i];
			lb_size += 1;
			break;
		case '.':
			buf[lb_start] = lb_size;
			lb_size = 0;
			lb_start = lb_index;
			break;
		case '\0':
			buf[lb_start] = lb_size;
			buf[lb_index] = 0;
			break;
		}
		lb_index += 1;
	}

	*len = lb_index;
	return 0;
}

int dns_unpack_answer(struct dns_msg_t *dns_msg, int dname_ptr)
{
	uint8_t *answer;
	int buf_size;
	int ptr;

	answer = dns_msg->msg + dns_msg->answer_offset;

	/* See RFC-1035 to find out why 63	*/
	if (answer[0] < 63) {
		return -ENOMEM;
	}

	/* Recovery of the pointer value	*/
	ptr = (((answer[0] & 63) << 8) + answer[1]);
	if (ptr != dname_ptr) {
		return -ENOMEM;
	}

	/*
	 * We need to be sure this buffer has enough space
	 * to parse the answer.
	 *
	 * size: dname_size + type + class + ttl + rdlength + rdata
	 *            2     +   2  +   2   +  4  +     2    +  ?
	 *
	 * So, answer size >= 12
	 *
	 * See RFC-1035 4.1.3. Resource record format
	 */
	buf_size = dns_msg->msg_size - dns_msg->answer_offset;
	if (buf_size < 12) {
		return -ENOMEM;
	}

	/* Only answers of type Internet.
	 * Here we use 2 as an offset because a ptr uses only 2 bytes.
	 */
	if (dns_answer_class(2, answer) != DNS_CLASS_IN) {
		return -EINVAL;
	}

	switch (dns_response_type(2, answer)) {
	case DNS_RR_TYPE_A:
	case DNS_RR_TYPE_AAAA:
		dns_msg->response_type = DNS_RESPONSE_IP;
		dns_msg->response_position = dns_msg->answer_offset + 12;
		dns_msg->response_length = dns_unpack_answer_rdlength(2,
								      answer);

		return 0;

	case DNS_RR_TYPE_CNAME:

		dns_msg->response_type = DNS_RESPONSE_CNAME_NO_IP;
		dns_msg->response_position = dns_msg->answer_offset + 12;
		dns_msg->response_length = dns_unpack_answer_rdlength(2,
								      answer);

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
	int size;
	int qdcount;
	int ancount;
	int rcode;

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

	rcode = dns_header_rcode(dns_header);
	switch (rcode) {
	case DNS_HEADER_NOERROR:
		break;
	default:
		return rcode;

	}

	qdcount = dns_unpack_header_qdcount(dns_header);
	ancount = dns_unpack_header_ancount(dns_header);
	if (qdcount < 1 || ancount < 1) {
		return -EINVAL;
	}

	return 0;
}

static int dns_msg_pack_query_header(uint8_t *buf, int size, uint16_t id)
{
	if (size < DNS_MSG_HEADER_SIZE) {
		return -ENOMEM;
	}

	*(uint16_t *)(buf +  0) = sys_cpu_to_be16(id);

	/* RD = 1, TC = 0, AA = 0, Opcode = 0, QR = 0 <-> 0x01 (1B)
	 * RCode = 0, Z = 0, RA = 0		      <-> 0x00 (1B)
	 *
	 * QDCOUNT = 1				      <-> 0x0001 (2B)
	 */

	*(buf + 2) = 0x01;
	*(buf + 3) = 0x00;
	*(uint16_t *)(buf + 4) = sys_cpu_to_be16(1);
	*(uint32_t *)(buf +  6) = 0;	/* ANCOUNT and NSCOUNT = 0	*/
	*(uint16_t *)(buf +  10) = 0;   /* ARCOUNT = 0			*/

	return 0;
}

int dns_msg_pack_query(uint8_t *buf, size_t *len, size_t size,
		       char *domain_name, uint16_t id, enum dns_rr_type qtype)
{
	int qname_len;
	int offset;
	int rc;

	rc = dns_msg_pack_query_header(buf, size, id);
	if (rc != 0) {
		return rc;
	}

	offset = DNS_MSG_HEADER_SIZE;

	rc = dns_msg_pack_qname(&qname_len, buf + offset,
				size - offset, domain_name);
	if (rc != 0) {
		return rc;
	}

	offset += qname_len;

	/* 4 bytes required: QType: (2), QClass (2)	*/
	if (offset + 4 > size) {
		return rc;
	}
	/* QType */
	*(uint16_t *)(buf + offset + 0) = sys_cpu_to_be16(qtype);
	/* QClass */
	*(uint16_t *)(buf + offset + 2) = sys_cpu_to_be16(DNS_CLASS_IN);

	*len = offset + 4;

	return 0;
}

int dns_find_null(int *qname_size, uint8_t *buf, int size)
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
	/* 4 bytes more for qtype and qclass */
	offset += 4;
	if (offset > dns_msg->msg_size) {
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

	dns_msg->answer_offset = dns_msg->query_offset + qname_size + 4;

	return 0;
}
