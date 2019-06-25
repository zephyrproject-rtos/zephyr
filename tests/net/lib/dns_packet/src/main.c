/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <dns_pack.h>

#define MAX_BUF_SIZE	512
/* RFC 1035, 4.1.1. Header section format */
#define DNS_HEADER_SIZE	12

static u8_t buf[MAX_BUF_SIZE];
static u16_t buf_len;

static u8_t qname[MAX_BUF_SIZE];
static u16_t qname_len;


/* Domain: www.zephyrproject.org
 * Type: standard query (IPv4)
 * Transaction ID: 0xda0f
 * Recursion desired
 */
static u8_t query_ipv4[] = { 0xda, 0x0f, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77,
				0x0d, 0x7a, 0x65, 0x70, 0x68, 0x79, 0x72, 0x70,
				0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x03, 0x6f,
				0x72, 0x67, 0x00, 0x00, 0x01, 0x00, 0x01 };

#define DNAME1 "www.zephyrproject.org"

/* Domain: zephyr.local
 * Type: standard query (IPv6)
 * Recursion not desired
 */
static u8_t query_mdns[] = {
	0xda, 0x0f, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65, 0x70,
	0x68, 0x79, 0x72, 0x05, 0x6c, 0x6f, 0x63, 0x61,
	0x6c, 0x00, 0x00, 0x01, 0x00, 0x01,
};

#define ZEPHYR_LOCAL "zephyr.local"

static u16_t tid1 = 0xda0f;

static int eval_query(const char *dname, u16_t tid, enum dns_rr_type type,
		      u8_t *expected, u16_t expected_len)
{
	u8_t *question;
	int rc;

	rc = dns_msg_pack_qname(&qname_len, qname, MAX_BUF_SIZE, dname);
	if (rc != 0) {
		goto lb_exit;
	}

	rc = dns_msg_pack_query(buf, &buf_len, MAX_BUF_SIZE, qname, qname_len,
				tid, type);
	if (rc != 0) {
		goto lb_exit;
	}

	if (dns_unpack_header_id(buf) != tid) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* This is a query */
	if (dns_header_qr(buf) != DNS_QUERY) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* This is a query (standard query) */
	if (dns_header_opcode(buf) != DNS_QUERY) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Authoritative Answer must be 0 for a Query */
	if (dns_header_aa(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* TrunCation is always 0 */
	if (dns_header_tc(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Recursion Desired is always 1 */
	if (dns_header_rd(buf) != 1) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Recursion Available is always 0 */
	if (dns_header_ra(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Z is always 0 */
	if (dns_header_z(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Response code must be 0 (no error) */
	if (dns_header_rcode(buf) != DNS_HEADER_NOERROR) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Question counter must be 1 */
	if (dns_header_qdcount(buf) != 1) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Answer counter must be 0 */
	if (dns_header_ancount(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Name server resource records counter must be 0 */
	if (dns_header_nscount(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* Additional records counter must be 0 */
	if (dns_header_arcount(buf) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

	question = buf + DNS_HEADER_SIZE;

	/* QClass */
	if (dns_unpack_query_qclass(question + qname_len) != DNS_CLASS_IN) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* QType */
	if (dns_unpack_query_qtype(question + qname_len) != type) {
		rc = -EINVAL;
		goto lb_exit;
	}

	/* compare with the expected result */
	if (buf_len != expected_len) {
		rc = -EINVAL;
		goto lb_exit;
	}

	if (memcmp(expected, buf, buf_len) != 0) {
		rc = -EINVAL;
		goto lb_exit;
	}

lb_exit:
	return rc;
}

/* The DNS response min size is computed as follows:
 * (hdr size) + (question min size) +  (RR min size)
 */
#define RESPONSE_MIN_SIZE	(DNS_HEADER_SIZE + 6 + 14)

/* DNS QNAME size here is 2 because we use DNS pointers */
#define NAME_PTR_SIZE	2
/* DNS integer size */
#define INT_SIZE	2
/* DNS answer TTL size */
#define ANS_TTL_SIZE	4

struct dns_response_test {
	/* domain name: example.com */
	const char *dname;

	/* expected result */
	u8_t *res;
	/* expected result length */
	u16_t res_len;

	/* transaction id */
	u16_t tid;
	/* A, AAAA */
	u8_t answer_type;
	/* answer counter */
	u8_t ancount;
	/* answer TTL */
	u32_t ttl;
	/* recursion available */
	u8_t ra;
	/* recursion desired */
	u8_t rd;
	/* data len */
	u8_t rdlen;
	/* data */
	const u8_t *rdata;
};

/* This routine evaluates DNS responses with one RR, and assumes that the
 * RR's name points to the DNS question's qname.
 */
static int eval_response1(struct dns_response_test *resp, bool unpack_answer)
{
	u8_t *ptr = resp->res;
	u16_t offset;
	int  rc;

	if (resp->res_len < RESPONSE_MIN_SIZE) {
		rc = __LINE__;
		goto lb_exit;
	}

	if (dns_unpack_header_id(resp->res) != resp->tid) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* This is a response */
	if (dns_header_qr(resp->res) != DNS_RESPONSE) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* For the DNS response, this value is standard query */
	if (dns_header_opcode(resp->res) != DNS_QUERY) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Authoritative Answer */
	if (dns_header_aa(resp->res) != 0) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* TrunCation is always 0 */
	if (dns_header_tc(resp->res) != 0) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Recursion Desired */
	if (dns_header_rd(resp->res) != resp->rd) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Recursion Available */
	if (dns_header_ra(resp->res) != resp->ra) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Z is always 0 */
	if (dns_header_z(resp->res) != 0) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Response code must be 0 (no error) */
	if (dns_header_rcode(resp->res) != DNS_HEADER_NOERROR) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Question counter must be 1 */
	if (dns_header_qdcount(resp->res) != 1) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Answer counter */
	if (dns_header_ancount(resp->res) != resp->ancount) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Name server resource records counter must be 0 */
	if (dns_header_nscount(resp->res) != 0) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* Additional records counter must be 0 */
	if (dns_header_arcount(resp->res) != 0) {
		rc = __LINE__;
		goto lb_exit;
	}

	rc = dns_msg_pack_qname(&qname_len, qname, MAX_BUF_SIZE, resp->dname);
	if (rc != 0) {
		goto lb_exit;
	}

	offset = DNS_HEADER_SIZE;

	/* DNS header + qname + qtype (int size) + qclass (int size) */
	if (offset + qname_len + 2 * INT_SIZE >= resp->res_len) {
		rc = __LINE__;
		goto lb_exit;
	}

	if (memcmp(qname, resp->res + offset, qname_len) != 0) {
		rc = __LINE__;
		goto lb_exit;
	}

	offset += qname_len;

	if (dns_unpack_query_qtype(resp->res + offset) != resp->answer_type) {
		rc = __LINE__;
		goto lb_exit;
	}

	if (dns_unpack_query_qclass(resp->res + offset) != DNS_CLASS_IN) {
		rc = __LINE__;
		goto lb_exit;
	}

	/* qtype and qclass */
	offset += INT_SIZE + INT_SIZE;

	if (unpack_answer) {
		u32_t ttl;
		struct dns_msg_t msg;

		msg.msg = resp->res;
		msg.msg_size = resp->res_len;
		msg.answer_offset = offset;

		if (dns_unpack_answer(&msg, DNS_ANSWER_MIN_SIZE, &ttl) < 0) {
			rc = __LINE__;
			goto lb_exit;
		}

		offset = msg.response_position;
	} else {
		/* 0xc0 and 0x0c are derived from RFC 1035 4.1.4 Message
		 * compression. 0x0c is the DNS Header Size (fixed size) and
		 * 0xc0 is the DNS pointer marker.
		 */
		if (resp->res[offset + 0] != 0xc0 ||
		    resp->res[offset + 1] != 0x0c) {
			rc = __LINE__;
			goto lb_exit;
		}

		/* simplify the following lines by applying the offset here */
		resp->res += offset;
		offset = NAME_PTR_SIZE;

		if (dns_answer_type(NAME_PTR_SIZE,
				    resp->res) != resp->answer_type) {
			rc = __LINE__;
			goto lb_exit;
		}

		offset += INT_SIZE;

		if (dns_answer_class(NAME_PTR_SIZE,
				     resp->res) != DNS_CLASS_IN) {
			rc = __LINE__;
			goto lb_exit;
		}

		offset += INT_SIZE;

		if (dns_answer_ttl(NAME_PTR_SIZE, resp->res) != resp->ttl) {
			rc = __LINE__;
			goto lb_exit;
		}

		offset += ANS_TTL_SIZE;

		if (dns_answer_rdlength(NAME_PTR_SIZE,
					resp->res) != resp->rdlen) {
			rc = __LINE__;
			goto lb_exit;
		}

		offset += INT_SIZE;
	}

	if (resp->rdlen + offset > resp->res_len) {
		rc = __LINE__;
		goto lb_exit;
	}

	if (memcmp(resp->res + offset, resp->rdata, resp->rdlen) != 0) {
		rc = __LINE__;
	}

lb_exit:
	resp->res = ptr;

	return -rc;
}


void test_dns_query(void)
{
	int rc;

	rc = eval_query(DNAME1, tid1, DNS_RR_TYPE_A,
			query_ipv4, sizeof(query_ipv4));
	zassert_equal(rc, 0, "Query test failed for domain: "DNAME1);

	rc = eval_query(NULL, tid1, DNS_RR_TYPE_A,
			query_ipv4, sizeof(query_ipv4));
	zassert_not_equal(rc, 0, "Query test with invalid domain name failed");

	rc = eval_query(DNAME1, tid1, DNS_RR_TYPE_AAAA,
			query_ipv4, sizeof(query_ipv4));
	zassert_not_equal(rc, 0, "Query test for IPv4 with RR type AAAA failed");

	rc = eval_query(DNAME1, tid1 + 1, DNS_RR_TYPE_A,
			query_ipv4, sizeof(query_ipv4));
	zassert_not_equal(rc, 0, "Query test with invalid ID failed");
}

/* DNS response for www.zephyrproject.org with the following parameters:
 * Transaction ID: 0xb041
 * Answer type: RR A
 * Answer counter: 1
 * TTL: 3028
 * Recursion Available: 1
 * RD len: 4 (IPv4 Address)
 * RData: 140.211.169.8
 */
static u8_t resp_ipv4[] = { 0xb0, 0x41, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01,
			       0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77,
			       0x0d, 0x7a, 0x65, 0x70, 0x68, 0x79, 0x72, 0x70,
			       0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x03, 0x6f,
			       0x72, 0x67, 0x00, 0x00, 0x01, 0x00, 0x01, 0xc0,
			       0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x0b,
			       0xd4, 0x00, 0x04, 0x8c, 0xd3, 0xa9, 0x08 };

static const u8_t resp_ipv4_addr[] = {140, 211, 169, 8};

void test_dns_response(void)
{
	struct dns_response_test test = {
		.dname = DNAME1,
		.res = resp_ipv4,
		.res_len = sizeof(resp_ipv4),
		.tid = 0xb041,
		.answer_type = DNS_RR_TYPE_A,
		.ancount = 1,
		.ttl = 3028,
		.ra = 1,
		.rd = 1,
		.rdlen = 4, /* IPv4 test */
		.rdata = resp_ipv4_addr
	};
	struct dns_response_test test1, test2;
	int rc;

	memcpy(&test1, &test, sizeof(test1));
	rc = eval_response1(&test1, false);
	zassert_equal(rc, 0,
		      "Response test failed for domain: " DNAME1
		      " at line %d", -rc);

	/* Test also using dns_unpack_answer() API */
	memcpy(&test2, &test, sizeof(test2));
	rc = eval_response1(&test2, true);
	zassert_equal(rc, 0,
		      "Response test 2 failed for domain: " DNAME1
		      " at line %d", -rc);
}

/* Domain: www.wireshark.org
 * Type: standard query (IPv4)
 * Transaction ID: 0x2121
 * Answer is for a.www.wireshark.org for testing purposes.
 */
char answer_ipv4[] = {
	0x21, 0x21, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77,
	0x09, 0x77, 0x69, 0x72, 0x65, 0x73, 0x68, 0x61,
	0x72, 0x6b, 0x03, 0x6f, 0x72, 0x67, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x01, 0x61, 0xc0, 0x0c, 0x00, 0x01, 0x00,
	0x01, 0x00, 0x00, 0x02, 0x58, 0x00, 0x04, 0xae,
	0x89, 0x2a, 0x41
};

#define DNAME2 "www.wireshark.org"

static const u8_t answer_ipv4_addr[] = { 174, 137, 42, 65 };

void test_dns_response2(void)
{
	struct dns_response_test test1 = {
		.dname = DNAME2,
		.res = answer_ipv4,
		.res_len = sizeof(answer_ipv4),
		.tid = 0x2121,
		.answer_type = DNS_RR_TYPE_A,
		.ancount = 1,
		.ttl = 600,
		.ra = 1,
		.rd = 1,
		.rdlen = 4, /* IPv4 test */
		.rdata = answer_ipv4_addr
	};
	int rc;

	/* Test also using dns_unpack_answer() API */
	rc = eval_response1(&test1, true);
	zassert_equal(rc, 0,
		      "Response test 2 failed for domain: " DNAME2
		      " at line %d", -rc);
}

void test_mdns_query(void)
{
	int rc;

	rc = eval_query(ZEPHYR_LOCAL, tid1, DNS_RR_TYPE_A,
			query_mdns, sizeof(query_mdns));
	zassert_equal(rc, 0, "Query test failed for domain: " ZEPHYR_LOCAL);
}

/* DNS response for zephyr.local with the following parameters:
 * Transaction ID: 0xf2b6
 * Answer type: RR AAAA
 * Answer counter: 1
 * TTL: 30
 * Recursion Available: 0
 * RD len: 16 (IPv6 Address)
 * RData: fe80:0000:0000:0000:0200:5eff:fe00:5337
 */
static u8_t resp_ipv6[] = {
	0xf2, 0xb6, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x06, 0x7a, 0x65, 0x70,
	0x68, 0x79, 0x72, 0x05, 0x6c, 0x6f, 0x63, 0x61,
	0x6c, 0x00, 0x00, 0x1c, 0x00, 0x01, 0x06, 0x7a,
	0x65, 0x70, 0x68, 0x79, 0x72, 0x05, 0x6c, 0x6f,
	0x63, 0x61, 0x6c, 0x00, 0x00, 0x1c, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x1e, 0x00, 0x10, 0xfe, 0x80,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x5e, 0xff, 0xfe, 0x00, 0x53, 0x37,
};

static const u8_t resp_ipv6_addr[] = {
	0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x5e, 0xff, 0xfe, 0x00, 0x53, 0x37
};

void test_mdns_response(void)
{
	struct dns_response_test test1 = {
		.dname = ZEPHYR_LOCAL,
		.res = resp_ipv6,
		.res_len = sizeof(resp_ipv6),
		.tid = 0xf2b6,
		.answer_type = DNS_RR_TYPE_AAAA,
		.ancount = 1,
		.ttl = 30,
		.ra = 0,
		.rd = 0,
		.rdlen = 16, /* IPv6 test */
		.rdata = resp_ipv6_addr,
	};
	int rc;

	rc = eval_response1(&test1, true);
	zassert_equal(rc, 0,
		      "Response test failed for domain: " ZEPHYR_LOCAL
		      " at line %d", -rc);
}

void test_main(void)
{
	ztest_test_suite(dns_tests,
			 ztest_unit_test(test_dns_query),
			 ztest_unit_test(test_dns_response),
			 ztest_unit_test(test_dns_response2),
			 ztest_unit_test(test_mdns_query),
			 ztest_unit_test(test_mdns_response)
		);

	ztest_run_test_suite(dns_tests);

	/* TODO:
	 *	1) add malformed DNS data
	 *	2) add validations against buffer overflows
	 *	3) add debug info to detect the exit point (or split the tests)
	 *	4) add test data with CNAME and more RR
	 */
}
